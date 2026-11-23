set(SOURCES
  main.c
  sx1231.c
  rfm.c
  ../../../x3d-lib-2/src/x3d_proto.c
  ../../../x3d-lib-2/src/x3d_pped.c
)
idf_component_register(
SRCS ${SOURCES}
INCLUDE_DIRS "." "../../../x3d-lib-2/src"
REQUIRES esp_driver_gpio esp_driver_spi esp_timer)
