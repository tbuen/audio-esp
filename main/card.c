#include <errno.h>
#include <esp_log.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/param.h>
#include <unistd.h>

#include "rpc_types.h"
#include "card.h"

/***************************
***** CONSTANTS ************
***************************/

/***************************
***** MACROS ***************
***************************/

#define TAG "card"

#define LOGE(...) ESP_LOGE(TAG, __VA_ARGS__)
#define LOGW(...) ESP_LOGW(TAG, __VA_ARGS__)
#define LOGI(...) ESP_LOGI(TAG, __VA_ARGS__)
#define LOGD(...) ESP_LOGD(TAG, __VA_ARGS__)

/***************************
***** TYPES ****************
***************************/

/***************************
***** LOCAL FUNCTIONS ******
***************************/

/***************************
***** LOCAL VARIABLES ******
***************************/

/***************************
***** PUBLIC FUNCTIONS *****
***************************/

bool card_get_directory_entries(const char *path, dir_entries_t *entries) {
    bool ret = true;
    memset(entries, 0, sizeof(dir_entries_t));
    entry_t **next_dir_ptr = &entries->dirs;
    entry_t **next_track_ptr = &entries->tracks;
    DIR *dir = opendir(path);
    if (dir) {
        while (true) {
            errno = 0;
            struct dirent *de = readdir(dir);
            if (errno) {
                LOGW("could not read directory %s", path);
                ret = false;
                break;
            }
            if (de) {
                if (de->d_type == DT_REG) {
                    if (!strcmp(".ogg", &de->d_name[strlen(de->d_name) - 4])) {
                        entry_t *entry = calloc(1, sizeof(entry_t));
                        entry->name = strdup(de->d_name);
                        *next_track_ptr = entry;
                        next_track_ptr = &entry->next;
                    } else if (!strcmp("cover.jpg", de->d_name)) {
                        entries->cover = strdup(de->d_name);
                    }
                }
                if (de->d_type == DT_DIR) {
                    entry_t *entry = calloc(1, sizeof(entry_t));
                    entry->name = strdup(de->d_name);
                    *next_dir_ptr = entry;
                    next_dir_ptr = &entry->next;
                }
            } else {
                break;
            }
        }
        closedir(dir);
    } else {
        LOGW("could not open directory %s", path);
        ret = false;
    }
    return ret;
}

uint8_t card_get_track_info(const char *filename, track_info_t *info) {
    uint8_t ret = RPC_ERROR_NO_ERROR;
    if (strcmp(".ogg", &filename[strlen(filename) - 4])) {
        ret = RPC_ERROR_NO_TRACK;
    } else {
        errno = 0;
        int fd = open(filename, O_RDONLY, 0);
        if (fd >= 0) {
            int n;
            uint8_t buffer[1024];
            uint64_t length = 0;
            uint32_t rate = 0;
            if ((n = read(fd, buffer, sizeof(buffer))) == sizeof(buffer)) {
                LOGD("first buffer read");
                uint8_t *ptr = memmem(buffer, sizeof(buffer), "\x01vorbis", 7);
                if (ptr) {
                    LOGD("found 1st header");
                    ptr += 7 + 5;
                    rate = ptr[0] + (ptr[1] << 8) + (ptr[2] << 16) + (ptr[3] << 24);
                    LOGD("rate: %d", rate);
                }
                ptr = memmem(buffer, sizeof(buffer), "\x03vorbis", 7);
                if (ptr) {
                    LOGD("found 2nd header");
                    ptr += 7;
                    uint32_t len = ptr[0] + (ptr[1] << 8) + (ptr[2] << 16) + (ptr[3] << 24);
                    ptr += 4 + len;
                    uint32_t num = ptr[0] + (ptr[1] << 8) + (ptr[2] << 16) + (ptr[3] << 24);
                    ptr += 4;
                    LOGD("found %d comments", num);
                    for (uint8_t c = 0; c < num; ++c) {
                        len = ptr[0] + (ptr[1] << 8) + (ptr[2] << 16) + (ptr[3] << 24);
                        ptr += 4;
                        if (memcmp(ptr, "GENRE=", 6) == 0) {
                            info->genre = calloc(1, len - 6 + 1);
                            memcpy(info->genre, &ptr[6], len - 6);
                        } else if (memcmp(ptr, "ARTIST=", 7) == 0) {
                            info->artist = calloc(1, len - 7 + 1);
                            memcpy(info->artist, &ptr[7], len - 7);
                        } else if (memcmp(ptr, "ALBUM=", 6) == 0) {
                            info->album = calloc(1, len - 6 + 1);
                            memcpy(info->album, &ptr[6], len - 6);
                        } else if (memcmp(ptr, "TITLE=", 6) == 0) {
                            info->title = calloc(1, len - 6 + 1);
                            memcpy(info->title, &ptr[6], len - 6);
                        } else if (memcmp(ptr, "DATE=", 5) == 0) {
                            char buf[10] = { 0 };
                            memcpy(buf, &ptr[5], MIN(sizeof(buf) - 1, len - 5));
                            info->date = strtoul(buf, NULL, 10);
                        } else if (memcmp(ptr, "TRACKNUMBER=", 12) == 0) {
                            char buf[10] = { 0 };
                            memcpy(buf, &ptr[12], MIN(sizeof(buf) - 1, len - 12));
                            info->track = strtoul(buf, NULL, 10);
                        }
                        ptr += len;
                    }
                }
            }
            lseek(fd, -sizeof(buffer), SEEK_END);
            while (true) {
                if ((n = read(fd, buffer, sizeof(buffer))) == sizeof(buffer)) {
                    LOGD("last buffer read");
                    uint8_t *ptr = memmem(buffer, sizeof(buffer), "OggS\x00\x04", 6);
                    if (ptr) {
                        LOGD("found page header");
                        length = (uint64_t)ptr[6] + ((uint64_t)ptr[7] << 8) +
                            ((uint64_t)ptr[8] << 16) + ((uint64_t)ptr[9] << 24) +
                            ((uint64_t)ptr[10] << 32) + ((uint64_t)ptr[11] << 40) +
                            ((uint64_t)ptr[12] << 48) + ((uint64_t)ptr[13] << 56);
                        break;
                    } else {
                        lseek(fd, - 2 * sizeof(buffer) + 32, SEEK_CUR);
                    }
                } else {
                    LOGW("could not read, n: %d", n);
                    break;
                }
            }
            if (length && rate) {
                info->duration = length / rate;
            }
            if (n < 0) {
                LOGE("error reading file!");
                ret = RPC_ERROR_IO_ERROR;
            }
            close(fd);
        } else {
            LOGE("error opening file %s: %d", filename, errno);
            ret = RPC_ERROR_FILE_NOT_FOUND;
        }
    }
    return ret;
}

/***************************
***** LOCAL FUNCTIONS ******
***************************/
