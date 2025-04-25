#include <stdio.h>
#include "aht20.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

const static char *TAG = "weather-station";

typedef struct {
    aht20_dev_handle_t aht20_handle;
} sensor_handles_t;

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

static esp_err_t root_get_handler(httpd_req_t *req)
{
    const char* resp = "Hello, World!";
    httpd_resp_sendstr(req, resp);
    ESP_LOGI(TAG, "/ : %s", resp);
    return ESP_OK;
}

static esp_err_t temp_get_handler(httpd_req_t *req)
{
    float temp, hum;

    sensor_handles_t *sensor_handles = (sensor_handles_t *)req->user_ctx;

    ESP_ERROR_CHECK(aht20_read_float(sensor_handles->aht20_handle, &temp, &hum));

    httpd_resp_set_type(req, "application/json");

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "temperature", temp);
    cJSON_AddNumberToObject(root, "humidity", hum);
    const char *temp_info = cJSON_Print(root);

    httpd_resp_sendstr(req, temp_info);
    ESP_LOGI(TAG, "/temperature: %s", temp_info);

    free((void *)temp_info);
    cJSON_Delete(root);

    return ESP_OK;
}

esp_err_t start_http_server(sensor_handles_t *sensor_handles)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    //config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server");
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t root_uri_get = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = root_get_handler,
        .user_ctx  = NULL,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root_uri_get));

    httpd_uri_t temp_uri_get = {
        .uri       = "/temperature",
        .method    = HTTP_GET,
        .handler   = temp_get_handler,
        .user_ctx  = sensor_handles,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &temp_uri_get));

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

    // Initialize I2C bus
    const i2c_master_bus_handle_t bus_handle = initializeI2CBus();
    ESP_LOGI(TAG, "I2C bus initialized");

    // Initialize AHT20 sensor
    const aht20_dev_handle_t aht20_handle = initializeAHT20(bus_handle);
    ESP_LOGI(TAG, "AHT20 device added");

    sensor_handles_t *sensor_handles = malloc(sizeof(sensor_handles));
    if (sensor_handles == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for sensor handles");
        return;
    }
    sensor_handles->aht20_handle = aht20_handle;

    ESP_ERROR_CHECK(start_http_server(sensor_handles));
}
