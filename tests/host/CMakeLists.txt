cmake_minimum_required(VERSION 3.16)
project(sensor_access_host_tests C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CHECK REQUIRED check)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components/movement-driver/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components/mqtt-aws/include
)

add_library(host_stubs STATIC
    stubs/fake_esp_stubs.c
)

target_include_directories(host_stubs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs
)

# ==========================================
# Test: test_movement_driver
# ==========================================
add_executable(test_movement_driver
    test/test_movement_driver.c
    ../../components/movement-driver/movement-driver.c
)

target_link_libraries(test_movement_driver PRIVATE host_stubs ${CHECK_LIBRARIES})
target_include_directories(test_movement_driver PRIVATE ${CHECK_INCLUDE_DIRS})
# Agrega la ruta física de las librerías encontradas por PkgConfig (Soluciona LNK1104)
target_link_directories(test_movement_driver PRIVATE ${CHECK_LIBRARY_DIRS})

if(NOT MSVC)
    target_compile_options(test_movement_driver PRIVATE ${CHECK_CFLAGS_OTHER})
endif()

add_test(NAME test_movement_driver COMMAND test_movement_driver)

# ==========================================
# Test: test_mqtt_aws
# ==========================================
add_executable(test_mqtt_aws
    test/test_mqtt_aws.c
    ../../components/mqtt-aws/mqtt-aws.c
)

target_link_libraries(test_mqtt_aws PRIVATE host_stubs ${CHECK_LIBRARIES})
target_include_directories(test_mqtt_aws PRIVATE ${CHECK_INCLUDE_DIRS})
# Agrega la ruta física de las librerías encontradas por PkgConfig (Soluciona LNK1104)
target_link_directories(test_mqtt_aws PRIVATE ${CHECK_LIBRARY_DIRS})

if(NOT MSVC)
    target_compile_options(test_mqtt_aws PRIVATE ${CHECK_CFLAGS_OTHER})
endif()

add_test(NAME test_mqtt_aws COMMAND test_mqtt_aws)

# ==========================================
# Test: test_sensor_pipeline
# ==========================================
add_executable(test_sensor_pipeline
    test/test_sensor_pipeline.c
    ../../components/movement-driver/movement-driver.c
    ../../components/mqtt-aws/mqtt-aws.c
)

target_link_libraries(test_sensor_pipeline PRIVATE host_stubs ${CHECK_LIBRARIES})
target_include_directories(test_sensor_pipeline PRIVATE ${CHECK_INCLUDE_DIRS})
# Agrega la ruta física de las librerías encontradas por PkgConfig (Soluciona LNK1104)
target_link_directories(test_sensor_pipeline PRIVATE ${CHECK_LIBRARY_DIRS})

if(NOT MSVC)
    target_compile_options(test_sensor_pipeline PRIVATE ${CHECK_CFLAGS_OTHER})
endif()

add_test(NAME test_sensor_pipeline COMMAND test_sensor_pipeline)

enable_testing()
