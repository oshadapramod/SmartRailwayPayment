idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES i2c-lcd1602
                       REQUIRES esp32-smbus)
