set(SOURCES
  main.c
  sx1231.c
  rfm.c
)
idf_component_register(
SRCS ${SOURCES}
INCLUDE_DIRS "."
REQUIRES esp_driver_gpio esp_driver_spi esp_timer)
