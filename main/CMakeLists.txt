# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS main.c
    INCLUDE_DIRS "."
    REQUIRES movement-driver mqtt-aws
	PRIV_REQUIRES driver esp_driver_spi
    )
