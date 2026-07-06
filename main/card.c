#include <errno.h>
#include <esp_log.h>
#include <dirent.h>

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
    entry_t **next_file_ptr = &entries->files;
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
                    entry_t *entry = calloc(1, sizeof(entry_t));
                    entry->name = strdup(de->d_name);
                    *next_file_ptr = entry;
                    next_file_ptr = &entry->next;
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

/***************************
***** LOCAL FUNCTIONS ******
***************************/
