#include "include/sensors.h"
#include "include/shared.h"

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

bmp390_handle_t initializeBMP390(i2c_master_bus_handle_t bus_handle)
{
    bmp390_handle_t bm390_handle;

    const bmp390_config_t bm390_config = I2C_BMP390_CONFIG_DEFAULT;

    ESP_ERROR_CHECK(bmp390_init(bus_handle, &bm390_config, &bm390_handle));

    return bm390_handle;
}

sensor_handles_t* sensors_init()
{
    // Initialize I2C bus
    const i2c_master_bus_handle_t bus_handle = initializeI2CBus();
    ESP_LOGI(TAG, "I2C bus initialized");

    // Initialize AHT20 sensor
    const aht20_dev_handle_t aht20_handle = initializeAHT20(bus_handle);
    ESP_LOGI(TAG, "AHT20 device added");

    const bmp390_handle_t bmp390_handle = initializeBMP390(bus_handle);
    ESP_LOGI(TAG, "BMP390 device added");

    sensor_handles_t *sensor_handles = malloc(sizeof(sensor_handles_t));
    if (sensor_handles == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for sensor handles");
        return NULL;
    }

    sensor_handles->aht20_handle = aht20_handle;
    sensor_handles->bmp390_handle = bmp390_handle;
    return sensor_handles;
}