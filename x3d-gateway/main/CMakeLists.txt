set(SOURCES main.c sx1231.c wifi.c rfm.c mqtt.c led.c ../../x3d-lib/x3d.c)
idf_component_register(SRCS ${SOURCES} INCLUDE_DIRS "." "../../x3d-lib")
