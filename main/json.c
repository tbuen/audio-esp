#include "esp_ota_ops.h"
#include "cJSON.h"

#include "audio.h"
#include "json.h"

char *json_get_info(void) {
    const esp_app_desc_t *app = esp_ota_get_app_description();
    char *string;

    cJSON *resp = cJSON_CreateObject();

    cJSON_AddStringToObject(resp, "project", app->project_name);
    cJSON_AddStringToObject(resp, "version", app->version);
    cJSON_AddStringToObject(resp, "esp-idf", app->idf_ver);

    string = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);

    return string;
}

char *json_get_tracks(void) {
    const audio_track *track = audio_get_tracks();
    char *string;

    cJSON *resp = cJSON_CreateArray();

    while (track) {
        cJSON *t = cJSON_CreateObject();
        cJSON_AddStringToObject(t, "filename", track->filename);
        cJSON_AddItemToArray(resp, t);
        track = track->next;
    }

    string = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);

    audio_release_tracks();

    return string;
}

bool json_post_play(const char *content, char **response) {
    bool valid = false;

    cJSON *req = cJSON_Parse(content);

    if (req) {
        cJSON *filename = cJSON_GetObjectItemCaseSensitive(req, "filename");
        if (cJSON_IsString(filename) && filename->valuestring) {
            valid = true;

            bool status = audio_play(filename->valuestring);

            cJSON *resp = cJSON_CreateObject();

            cJSON_AddBoolToObject(resp, "status", status);

            *response = cJSON_PrintUnformatted(resp);
            cJSON_Delete(resp);
        }
        cJSON_Delete(req);
    }

    return valid;
}

void json_free(void *obj) {
    cJSON_free(obj);
}
