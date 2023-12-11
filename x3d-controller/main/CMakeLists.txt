set(SOURCES main.c sx1231.c wifi.c rfm.c mqtt.c led.c x3d_handler.c ../../x3d-lib/x3d.c)
idf_component_register(SRCS ${SOURCES} INCLUDE_DIRS "." "../../x3d-lib")
# requires min idf 5.2 but is still in dev
# nvs_create_partition_image(nvs ../nvs_data.csv FLASH_IN_PROJECT)
