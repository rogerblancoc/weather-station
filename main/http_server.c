//#include <stdio.h>
#include "fcntl.h"
#include "esp_vfs.h"

#include "include/http_server.h"
#include "include/shared.h"

void send_cors_headers(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
}

void set_content_type(httpd_req_t *req, const char *filepath)
{
    const char *type;
    if (strstr(filepath, ".html")) {
        type = "text/html";
    } else if (strstr(filepath, ".css")) {
        type = "text/css";
    } else if (strstr(filepath, ".js")) {
        type = "application/javascript";
    } else if (strstr(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (strstr(filepath, ".svg")) {
        type = "image/svg+xml";
    } else {
        type = "text/plain";
    }
    httpd_resp_set_type(req, type);
}

static esp_err_t hello_get_handler(httpd_req_t *req)
{
    send_cors_headers(req);

    httpd_resp_set_type(req, "application/json");

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "message", "Hello, World!");
    const char *message_info = cJSON_Print(root);

    httpd_resp_sendstr(req, message_info);
    ESP_LOGI(TAG, "/api/hello:\n%s", message_info);

    free((void *)message_info);
    cJSON_Delete(root);

    return ESP_OK;
}

static esp_err_t weather_get_handler(httpd_req_t *req)
{
    send_cors_headers(req);

    sensor_handles_t *sensor_handles = (sensor_handles_t *)req->user_ctx;

    float temp, hum;
    ESP_ERROR_CHECK(aht20_read_float(sensor_handles->aht20_handle, &temp, &hum));

    float _tmp, pres;
    ESP_ERROR_CHECK(bmp390_get_measurements(sensor_handles->bmp390_handle, &_tmp, &pres));
    pres /= 100;

    httpd_resp_set_type(req, "application/json");

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "temperature", temp);
    cJSON_AddNumberToObject(root, "humidity", hum);
    cJSON_AddNumberToObject(root, "pressure", pres);
    const char *temp_info = cJSON_Print(root);

    httpd_resp_sendstr(req, temp_info);
    ESP_LOGI(TAG, "/api/weather:\n%s", temp_info);

    free((void *)temp_info);
    cJSON_Delete(root);

    return ESP_OK;
}

static esp_err_t root_get_handler(httpd_req_t *req)
{
    send_cors_headers(req);

    char filepath[MAX_FILEPATH_SIZE] = "/www";

    if (strcmp(req->uri, "/") == 0) {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else if (strstr(req->uri, ".js")) {
        // If the request is for vendor files, append .gz for gzip compression
        if(strstr(req->uri, "/vendor/")) {
            strlcat(req->uri, ".gz", sizeof(req->uri));
            httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
        }
        strlcat(filepath, req->uri, sizeof(filepath));
    } else {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    ESP_LOGI(TAG, "%s", filepath);

    // Open the file
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        ESP_LOGE(TAG, "Failed to open file");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type(req, filepath);

    // Buffer for reading chunks
    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytes_read;

    do {
        bytes_read = read(fd, buffer, MAX_BUFFER_SIZE);
        if (bytes_read < 0) {
            ESP_LOGE(TAG, "Error reading file");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
            return ESP_FAIL;
        }

        ESP_ERROR_CHECK(httpd_resp_send_chunk(req, buffer, bytes_read));
    } while (bytes_read > 0);

    close(fd);
    ESP_LOGI(TAG, "File sending complete");
    // Respond with an empty chunk to signal HTTP response completion
    httpd_resp_send_chunk(req, NULL, 0);

    return ESP_OK;
}

esp_err_t http_server_init(sensor_handles_t *sensor_handles)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server");
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t hello_uri_get = {
        .uri       = "/api/hello",
        .method    = HTTP_GET,
        .handler   = hello_get_handler,
        .user_ctx  = NULL,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &hello_uri_get));

    httpd_uri_t weather_uri_get = {
        .uri       = "/api/weather",
        .method    = HTTP_GET,
        .handler   = weather_get_handler,
        .user_ctx  = sensor_handles,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &weather_uri_get));

    httpd_uri_t root_uri_get = {
        .uri       = "/*",
        .method    = HTTP_GET,
        .handler   = root_get_handler,
        .user_ctx  = NULL,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root_uri_get));

    ESP_LOGI(TAG, "HTTP Server started");

    return ESP_OK;
}