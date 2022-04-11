#include "esp_ota_ops.h"
#include "cJSON.h"

#include "audio.h"
#include "json.h"

char *json_get_info(void) {
    const esp_app_desc_t *app = esp_ota_get_app_description();
    char *string;

    cJSON *info = cJSON_CreateObject();

    cJSON_AddStringToObject(info, "project", app->project_name);
    cJSON_AddStringToObject(info, "version", app->version);
    cJSON_AddStringToObject(info, "esp-idf", app->idf_ver);

    string = cJSON_PrintUnformatted(info);
    cJSON_Delete(info);

    return string;
}

char *json_get_tracks(void) {
    const audio_track *track = audio_get_tracks();
    char *string;

    cJSON *tracks = cJSON_CreateArray();

    while (track) {
        cJSON *t = cJSON_CreateObject();
        cJSON_AddStringToObject(t, "filename", track->filename);
        cJSON_AddItemToArray(tracks, t);
        track = track->next;
    }

    string = cJSON_PrintUnformatted(tracks);
    cJSON_Delete(tracks);
    audio_release_tracks();

    return string;
}

void json_free(void *obj) {
    cJSON_free(obj);
}
