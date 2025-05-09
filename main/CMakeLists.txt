idf_component_register(SRCS "main.c" "sensors.c"
                    INCLUDE_DIRS ".")

set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../front")
spiffs_create_partition_image(www ${WEB_SRC_DIR} FLASH_IN_PROJECT)