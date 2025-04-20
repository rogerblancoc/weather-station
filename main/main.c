#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "aht20.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

const static char *TAG = "weather-station";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Weather Station...");

    // Initialize I2C bus
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

    ESP_LOGI(TAG, "I2C bus initialized");

    // Initialize AHT20
    const i2c_aht20_config_t aht20_config = {
        .i2c_config.device_address = AHT20_ADDRESS_0,
        .i2c_config.scl_speed_hz = 100000,
        .i2c_timeout = 100,
    };
    aht20_dev_handle_t aht20_handle;

    ESP_ERROR_CHECK(aht20_new_sensor(bus_handle, &aht20_config, &aht20_handle));

    ESP_LOGI(TAG, "AHT20 device added");

    float temp, hum;

    int counter = 0;
    while(counter < 10) {
        // Read temperature and humidity
        ESP_ERROR_CHECK(aht20_read_float(aht20_handle, &temp, &hum));
        ESP_LOGI(TAG, "[%d] Temperature: %2.2f C; Humidity: %2.2f", counter, temp, hum);

        vTaskDelay(pdMS_TO_TICKS(2000));
        counter++;
    }
}