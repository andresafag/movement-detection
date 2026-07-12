cmake_minimum_required(VERSION 3.16)
project(sensor_access_host_tests C)

# ---------------------------------------------------------------------------
# Host-based unit tests for the ESP32 firmware components.
#
# This build compiles the components with their *real* GPIO/FreeRTOS code
# linked against host stubs (tests/host/), then runs them on Linux/macOS/Windows
# under a standard C test runner.  No ESP32 toolchain required.
#
# Run with:
#   cmake -S tests/host -B tests/host/build
#   cmake --build tests/host/build
#   ctest --test-dir tests/host/build --output-on-failure
# ---------------------------------------------------------------------------

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Strict warnings — same flags the firmware cares about.
add_compile_options(
    -Wall
    -Wextra
    -Werror=implicit-function-declaration
    -Werror=return-type
    -Wno-unused-parameter
)

if(MSVC)
    add_compile_options(/W4)
endif()

# ---------------------------------------------------------------------------
# Host stubs — fake implementations of ESP-IDF / FreeRTOS functions that
# the firmware calls.  These live in tests/host/ and are linked in below.
# ---------------------------------------------------------------------------
add_library(host_stubs STATIC
    test.c
)
target_include_directories(host_stubs PUBLIC include)

# ---------------------------------------------------------------------------
# Components under test — the REAL C source compiled in.
# We must compile them with the host stub headers on the include path.
# ---------------------------------------------------------------------------
add_library(movement_driver STATIC
    ../../components/movement-driver/movement-driver.c
)
target_include_directories(movement_driver PUBLIC
    ../../components/movement-driver/include
)
target_link_libraries(movement_driver PUBLIC host_stubs)

add_library(mqtt_aws STATIC
    ../../components/mqtt-aws/mqtt-aws.c
)
target_include_directories(mqtt_aws PUBLIC
    ../../components/mqtt-aws/include
)
target_link_libraries(mqtt_aws PUBLIC host_stubs)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()

add_subdirectory(test)
