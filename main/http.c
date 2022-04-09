#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"

#include "http.h"

static esp_err_t get_info_handler(httpd_req_t *req);

static const char *TAG = "http";

static httpd_handle_t server = NULL;
static char resp[256];

static httpd_uri_t get_info = {
    .uri = "/info",
    .method = HTTP_GET,
    .handler = &get_info_handler,
    .user_ctx = NULL
};

void http_init(void) {
    if (server) return;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &get_info);
    }

    if (server) {
        ESP_LOGI(TAG, "started");
    } else {
        ESP_LOGE(TAG, "failed to start");
    }
}

static esp_err_t get_info_handler(httpd_req_t *req) {
    const esp_app_desc_t *app_desc = esp_ota_get_app_description();

    sprintf(resp, "{\n    \"project\": \"%s\",\n    \"version\": \"%s\",\n    \"esp-idf\": \"%s\"\n}\n", app_desc->project_name, app_desc->version, app_desc->idf_ver);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);

    return ESP_OK;
}
