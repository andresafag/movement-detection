# Integration test project for the sensor-access pipeline.
#
# This is a *separate* ESP-IDF project.  It uses the production
# components from /components and runs on real ESP32 hardware (or qemu).
#
#   idf.py -C tests/integration build
#   idf.py -C tests/integration flash monitor
#
# The Unity test harness reports results on the serial monitor.

cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/../components"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(sensor_access_integration_test)
