#include <stdio.h>
#include <fcntl.h>
#include "aht20.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

const static char *TAG = "weather-station";

typedef struct {
    aht20_dev_handle_t aht20_handle;
} sensor_handles_t;

#define MAX_BUFFER_SIZE 1024
#define MAX_FILEPATH_SIZE 128

i2c_master_bus_handle_t initializeI2CBus()
{
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = -1,
        .sda_io_num = GPIO_NUM_1,
        .scl_io_num = GPIO_NUM_4,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    return bus_handle;
}

aht20_dev_handle_t initializeAHT20(i2c_master_bus_handle_t bus_handle)
{
    const i2c_aht20_config_t aht20_config = {
        .i2c_config.device_address = AHT20_ADDRESS_0,
        .i2c_config.scl_speed_hz = 100000,
        .i2c_timeout = 100,
    };
    aht20_dev_handle_t aht20_handle;

    ESP_ERROR_CHECK(aht20_new_sensor(bus_handle, &aht20_config, &aht20_handle));

    return aht20_handle;
}

sensor_handles_t* start_sensors()
{
    // Initialize I2C bus
    const i2c_master_bus_handle_t bus_handle = initializeI2CBus();
    ESP_LOGI(TAG, "I2C bus initialized");

    // Initialize AHT20 sensor
    const aht20_dev_handle_t aht20_handle = initializeAHT20(bus_handle);
    ESP_LOGI(TAG, "AHT20 device added");

    sensor_handles_t *sensor_handles = malloc(sizeof(sensor_handles));
    if (sensor_handles == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for sensor handles");
        return NULL;
    }

    sensor_handles->aht20_handle = aht20_handle;
    return sensor_handles;
}

esp_err_t init_fs()
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/www",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ESP_OK;
}

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

    httpd_resp_set_type(req, "application/json");

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "temperature", temp);
    cJSON_AddNumberToObject(root, "humidity", hum);
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

esp_err_t start_http_server(sensor_handles_t *sensor_handles)
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

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Weather Station...");

    // Initialize NVS, TCP/IP, and event loop
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // Initialize Wi-Fi
    ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "Wi-Fi connected");

    // Initialize all sensors
    sensor_handles_t *sensor_handles = start_sensors();
    ESP_LOGI(TAG, "Sensors initialized");

    ESP_ERROR_CHECK(init_fs());
    ESP_LOGI(TAG, "SPIFFS initialized");

    ESP_ERROR_CHECK(start_http_server(sensor_handles));
}
