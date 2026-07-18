idf_component_register(
    SRCS main.c
    INCLUDE_DIRS "."
    REQUIRES movement-driver mqtt-aws wifi-driver
	PRIV_REQUIRES driver esp_driver_spi
    )
