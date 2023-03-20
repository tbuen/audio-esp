#include "esp_http_server.h"
#include "esp_log.h"
#include "unistd.h"

#include "message.h"
#include "json.h"
#include "http.h"

#define HTTP_MAX_SOCKETS 4

static void http_close_fn(httpd_handle_t hd, int sockfd);
static esp_err_t websocket_handler(httpd_req_t *req);

static const char *TAG = "http";

static httpd_handle_t server = NULL;
static QueueHandle_t queue;

static const httpd_uri_t websocket = {
    .uri = "/websocket",
    .method = HTTP_GET,
    .handler = &websocket_handler,
    .user_ctx = NULL,
    .is_websocket = true,
    .handle_ws_control_frames = false
};

void http_init(QueueHandle_t q) {
    queue = q;
}

void http_start(void) {
    if (server) return;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = HTTP_MAX_SOCKETS;
    config.close_fn = &http_close_fn;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &websocket);
    }

    if (server) {
        ESP_LOGI(TAG, "started");
    } else {
        ESP_LOGE(TAG, "failed to start");
    }
}

void http_stop(void) {
    if (!server) return;

    size_t fds = HTTP_MAX_SOCKETS;
    int client_fds[HTTP_MAX_SOCKETS];
    ESP_ERROR_CHECK(httpd_get_client_list(server, &fds, client_fds));

    for (size_t i = 0; i < fds; ++i) {
        if (httpd_ws_get_fd_info(server, client_fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET) {
            httpd_ws_frame_t ws_pkt = {
                .final = true,
                .fragmented = false,
                .type = HTTPD_WS_TYPE_CLOSE,
                .payload = NULL,
                .len = 0
            };
            httpd_ws_send_data(server, client_fds[i], &ws_pkt);
        }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    httpd_stop(server);
    server = NULL;

    ESP_LOGI(TAG, "stopped");
}

size_t http_get_number_of_clients(void) {
    if (!server) return 0;

    size_t fds = HTTP_MAX_SOCKETS;
    int client_fds[HTTP_MAX_SOCKETS];
    ESP_ERROR_CHECK(httpd_get_client_list(server, &fds, client_fds));
    ESP_LOGI(TAG, "open sockets: %d", fds);
    return fds;
}

bool http_send(int sockfd, char *text) {
    if (!server) return false;
    if (httpd_ws_get_fd_info(server, sockfd) != HTTPD_WS_CLIENT_WEBSOCKET) return false;

    httpd_ws_frame_t ws_pkt = {
        .final = true,
        .fragmented = false,
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t*)text,
        .len = strlen(text)
    };
    return httpd_ws_send_data(server, sockfd, &ws_pkt) == ESP_OK;
}

static void http_close_fn(httpd_handle_t hd, int sockfd) {
    ESP_LOGI(TAG, "close socket %d", sockfd);
    close(sockfd);
    message_t msg = { BASE_HTTP, EVENT_HTTP_DISCONNECT, NULL };
    xQueueSendToBack(queue, &msg, 0);
}

static esp_err_t websocket_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "new connection, socket %d", httpd_req_to_sockfd(req));
        message_t msg = { BASE_HTTP, EVENT_HTTP_CONNECT, NULL };
        xQueueSendToBack(queue, &msg, 0);
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "packet type: %d len: %d", ws_pkt.type, ws_pkt.len);
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
        if (ws_pkt.len) {
            buf = calloc(1, ws_pkt.len + 1);
            if (buf == NULL) {
                ESP_LOGE(TAG, "Failed to calloc memory for buf");
                return ESP_ERR_NO_MEM;
            }
            ws_pkt.payload = buf;
            ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
                free(buf);
                return ret;
            }

            msg_http_recv_t *msg_data = calloc(1, sizeof(msg_http_recv_t));
            msg_data->com_ctx = calloc(1, sizeof(com_ctx_t));
            msg_data->com_ctx->sockfd = httpd_req_to_sockfd(req);
            msg_data->text = (char*)buf;
            message_t msg = { BASE_HTTP, EVENT_HTTP_RECV, msg_data };
            xQueueSendToBack(queue, &msg, 0);
        }
    }
    return ret;
}
