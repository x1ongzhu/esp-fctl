/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_chip_info.h"
#include "esp_random.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include "esp_wifi.h"

static const char *REST_TAG = "esp-rest";
void set_fan_speed(int speed);
esp_err_t read_fan_speed(int32_t *fan_speed);
esp_err_t write_fan_speed(int32_t fan_speed);
void led_set_color(uint32_t hue);
extern int rpm;
uint16_t wifi_scan(wifi_ap_record_t *ap_info, int size);
void config_sta(char *ssid, char *password);
esp_err_t read_str(char *key, char *value);
esp_err_t write_str(char *key, char *value);

#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(REST_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context
{
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html"))
    {
        type = "text/html";
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".js"))
    {
        type = "application/javascript";
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".css"))
    {
        type = "text/css";
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".png"))
    {
        type = "image/png";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".ico"))
    {
        type = "image/x-icon";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".svg"))
    {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];

    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/')
    {
        strlcat(filepath, "/index.html", sizeof(filepath));
    }
    else
    {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1)
    {
        strlcpy(filepath, rest_context->base_path, sizeof(filepath));
        strlcat(filepath, "/index.html", sizeof(filepath));
        fd = open(filepath, O_RDONLY, 0);
        if (fd == -1)
        {
            ESP_LOGE(REST_TAG, "Failed to open file : %s", filepath);
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
            return ESP_FAIL;
        }
    }

    set_content_type_from_file(req, filepath);

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do
    {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1)
        {
            ESP_LOGE(REST_TAG, "Failed to read file : %s", filepath);
        }
        else if (read_bytes > 0)
        {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK)
            {
                close(fd);
                ESP_LOGE(REST_TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(REST_TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t rpm_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "rpm", rpm);
    const char *json_str = cJSON_Print(root);
    httpd_resp_sendstr(req, json_str);
    free((void *)json_str);
    cJSON_Delete(root);

    return ESP_OK;
}

static esp_err_t fan_speed_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    int32_t fan_speed = 0;
    esp_err_t err = read_fan_speed(&fan_speed);
    if (ESP_OK != err)
    {
        cJSON_AddNumberToObject(root, "speed", 0);
    }
    else
    {
        cJSON_AddNumberToObject(root, "speed", fan_speed);
    }
    const char *json_str = cJSON_Print(root);
    httpd_resp_sendstr(req, json_str);
    free((void *)json_str);
    cJSON_Delete(root);

    return ESP_OK;
}

static esp_err_t fan_speed_put_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0)
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    int speed = cJSON_GetObjectItem(root, "speed")->valueint;
    ESP_LOGI(REST_TAG, "Fan control: speed = %d", speed);
    ESP_ERROR_CHECK(write_fan_speed(speed));
    cJSON_Delete(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\": \"ok\"}");
    set_fan_speed(speed);
    led_set_color(240 * speed / 100);
    return ESP_OK;
}

static esp_err_t name_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    char name[128];
    esp_err_t err = read_str("name", &name);
    if (ESP_OK != err)
    {
        cJSON_AddStringToObject(root, "name", "fctl");
    }
    else
    {
        cJSON_AddStringToObject(root, "name", name);
    }
    const char *json_str = cJSON_Print(root);
    httpd_resp_sendstr(req, json_str);
    free((void *)json_str);
    cJSON_Delete(root);

    return ESP_OK;
}

static esp_err_t name_put_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0)
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    char *name = cJSON_GetObjectItem(root, "name")->valuestring;
    ESP_LOGI(REST_TAG, "set name = %s", name);
    ESP_ERROR_CHECK(write_str("name", name));
    cJSON_Delete(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\": \"ok\"}");
    return ESP_OK;
}

static esp_err_t mode_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    cJSON_AddNumberToObject(root, "mode", mode == WIFI_MODE_STA ? 0 : 1);
    const char *json_str = cJSON_Print(root);
    httpd_resp_sendstr(req, json_str);
    free((void *)json_str);
    cJSON_Delete(root);

    return ESP_OK;
}

static esp_err_t wifi_scan_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateArray();

    wifi_ap_record_t ap_info[10];
    memset(ap_info, 0, sizeof(ap_info));
    uint16_t ap_count = wifi_scan(&ap_info, 10);
    for (int i = 0; (i < 10) && (i < ap_count); i++)
    {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "ssid", (char *)(ap_info[i].ssid));
        cJSON_AddNumberToObject(item, "rssi", ap_info[i].rssi);
        cJSON_AddNumberToObject(item, "channel", ap_info[i].primary);
        cJSON_AddItemToArray(root, item);
    }
    const char *json_str = cJSON_Print(root);
    httpd_resp_sendstr(req, json_str);
    free((void *)json_str);
    cJSON_Delete(root);

    return ESP_OK;
}

static esp_err_t wifi_sta_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0)
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    char *ssid = cJSON_GetObjectItem(root, "ssid")->valuestring;
    char *password = cJSON_GetObjectItem(root, "password")->valuestring;
    ESP_LOGI(REST_TAG, "ssid = %s, password = %s", ssid, password);
    cJSON_Delete(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\": \"ok\"}");
    config_sta(ssid, password);
    return ESP_OK;
}

esp_err_t start_rest_server(const char *base_path)
{
    REST_CHECK(base_path, "wrong base path", err);
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    REST_CHECK(rest_context, "No memory for rest context", err);
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(REST_TAG, "Starting HTTP Server");
    REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

    httpd_uri_t rpm_get_uri = {
        .uri = "/api/rpm/get",
        .method = HTTP_GET,
        .handler = rpm_get_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &rpm_get_uri);

    httpd_uri_t fan_speed_get_uri = {
        .uri = "/api/fan/speed",
        .method = HTTP_GET,
        .handler = fan_speed_get_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &fan_speed_get_uri);

    httpd_uri_t fan_speed_put_uri = {
        .uri = "/api/fan/speed",
        .method = HTTP_PUT,
        .handler = fan_speed_put_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &fan_speed_put_uri);

    httpd_uri_t wifi_scan_get_uri = {
        .uri = "/api/wifi/scan",
        .method = HTTP_GET,
        .handler = wifi_scan_get_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &wifi_scan_get_uri);

    httpd_uri_t wifi_sta_post_uri = {
        .uri = "/api/wifi/sta",
        .method = HTTP_POST,
        .handler = wifi_sta_post_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &wifi_sta_post_uri);

    httpd_uri_t name_get_uri = {
        .uri = "/api/name",
        .method = HTTP_GET,
        .handler = name_get_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &name_get_uri);

    httpd_uri_t name_put_uri = {
        .uri = "/api/name",
        .method = HTTP_PUT,
        .handler = name_put_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &name_put_uri);

    httpd_uri_t mode_get_uri = {
        .uri = "/api/mode/get",
        .method = HTTP_GET,
        .handler = mode_get_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &mode_get_uri);

    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &common_get_uri);

    return ESP_OK;
err_start:
    free(rest_context);
err:
    return ESP_FAIL;
}
