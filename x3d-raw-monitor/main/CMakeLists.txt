set(SOURCES main.c sx1231.c rfm.c)
idf_component_register(SRCS ${SOURCES} INCLUDE_DIRS ".")
