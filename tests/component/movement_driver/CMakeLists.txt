# Component test for the movement-driver.
#
# This is a *separate* ESP-IDF project that imports the production
# movement-driver as a component.  It is built and (optionally) flashed
# by idf.py.  Run with:
#
#   idf.py -C tests/component/movement_driver build
#   idf.py -C tests/component/movement_driver test       # runs on-device

cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../components"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(movement_driver_component_test)
