#include "esp_http_server.h"
#include "esp_log.h"

#include "json.h"
#include "http.h"

static esp_err_t get_info_handler(httpd_req_t *req);
static esp_err_t get_tracks_handler(httpd_req_t *req);
static esp_err_t post_play_handler(httpd_req_t *req);

static const char *TAG = "http";

static httpd_handle_t server = NULL;

static httpd_uri_t get_info = {
    .uri = "/info",
    .method = HTTP_GET,
    .handler = &get_info_handler,
    .user_ctx = NULL
};

static httpd_uri_t get_tracks = {
    .uri = "/tracks",
    .method = HTTP_GET,
    .handler = &get_tracks_handler,
    .user_ctx = NULL
};

static httpd_uri_t post_play = {
    .uri = "/play",
    .method = HTTP_POST,
    .handler = &post_play_handler,
    .user_ctx = NULL
};

void http_start(void) {
    if (server) return;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &get_info);
        httpd_register_uri_handler(server, &get_tracks);
        httpd_register_uri_handler(server, &post_play);
    }

    if (server) {
        ESP_LOGI(TAG, "started");
    } else {
        ESP_LOGE(TAG, "failed to start");
    }
}

void http_stop(void) {
    if (!server) return;

    httpd_stop(server);
    server = NULL;
}

static esp_err_t get_info_handler(httpd_req_t *req) {
    char *resp = json_get_info();

    if (!resp) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, NULL);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_sendstr(req, resp);

    json_free(resp);

    return ESP_OK;
}

static esp_err_t get_tracks_handler(httpd_req_t *req) {
    char *resp = json_get_tracks();

    if (!resp) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, NULL);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_sendstr(req, resp);

    json_free(resp);

    return ESP_OK;
}

static esp_err_t post_play_handler(httpd_req_t *req) {
    char *content = calloc(1, req->content_len + 1);

    int ret = httpd_req_recv(req, content, req->content_len);
    if (ret < req->content_len) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_err(req, HTTPD_408_REQ_TIMEOUT, NULL);
        }
        free(content);
        return ESP_FAIL;
    }

    char *resp = NULL;

    if (!json_post_play(content, &resp)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, resp);
        free(content);
        return ESP_FAIL;
    }

    if (resp) {
        httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    }
    httpd_resp_sendstr(req, resp);

    if (resp) json_free(resp);
    free(content);

    return ESP_OK;
}
