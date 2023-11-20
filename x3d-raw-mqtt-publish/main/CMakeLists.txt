set(SOURCES main.c sx1231.c wifi.c rfm.c mqtt.c)
idf_component_register(SRCS ${SOURCES} INCLUDE_DIRS ".")
