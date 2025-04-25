#include <stdio.h>
#include "aht20.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

const static char *TAG = "weather-station";

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

    float temp, hum;

    while(true) {
        // Read temperature and humidity
        ESP_ERROR_CHECK(aht20_read_float(aht20_handle, &temp, &hum));
        ESP_LOGI(TAG, "Temperature: %2.2f C; Humidity: %2.2f", temp, hum);
        // Wait for 2 seconds before the next reading
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
