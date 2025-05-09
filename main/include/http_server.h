#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include "cJSON.h"
#include "esp_http_server.h"

#include "include/sensors.h"

#define MAX_BUFFER_SIZE 1024
#define MAX_FILEPATH_SIZE 128

esp_err_t http_server_init(sensor_handles_t *sensor_handles);

#endif // __HTTP_SERVER_H__