#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"

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
    };
    i2c_master_bus_handle_t bus_handle;

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    ESP_LOGI(TAG, "I2C bus initialized");

    // Initialize AHT20
    const i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x38,
        .scl_speed_hz = 100000,
    };
    i2c_master_dev_handle_t device_handle;

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &device_config, &device_handle));

    ESP_LOGI(TAG, "AHT20 device added");
}