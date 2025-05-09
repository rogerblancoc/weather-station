#include "esp_event.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "mdns.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "include/http_server.h"
#include "include/sensors.h"
#include "include/shared.h"

#define HOSTNAME "weather-station"

esp_err_t mdns_service_init()
{
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(HOSTNAME));
    ESP_ERROR_CHECK(mdns_instance_name_set("Weather Station"));
    ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));

    return ESP_OK;
}

esp_err_t fs_init()
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

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Weather Station...");

    // Initialize NVS, TCP/IP, and event loop
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(TAG, "NVS, TCP/IP, and event loop initialized");

    // Initialize mDNS
    ESP_ERROR_CHECK(mdns_service_init());
    ESP_LOGI(TAG, "mDNS initialized");

    // Initialize Wi-Fi
    ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "Wi-Fi initialized");

    // Initialize all sensors
    sensor_handles_t *sensor_handles = sensors_init();
    ESP_LOGI(TAG, "Sensors initialized");

    // Initialize SPIFFS
    ESP_ERROR_CHECK(fs_init());
    ESP_LOGI(TAG, "SPIFFS initialized");

    // Initialize HTTP server
    ESP_ERROR_CHECK(http_server_init(sensor_handles));
    ESP_LOGI(TAG, "HTTP Server initialized");
}
