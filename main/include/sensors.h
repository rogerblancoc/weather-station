#ifndef __SENSORS_H__
#define __SENSORS_H__

#include "aht20.h"
#include "bmp390.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

typedef struct {
    aht20_dev_handle_t aht20_handle;
    bmp390_handle_t bmp390_handle;
} sensor_handles_t;

sensor_handles_t* sensors_init();

#endif // __SENSORS_H__