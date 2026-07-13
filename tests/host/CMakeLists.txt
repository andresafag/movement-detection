cmake_minimum_required(VERSION 3.16)
project(sensor_access_host_tests C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ==========================================
# Configuración Unificada de dependencias
# ==========================================
if(WIN32)
    find_package(check CONFIG REQUIRED)
    
    if(TARGET check::check)
        set(CHECK_LIBS check::check)
    elseif(TARGET Check::check)
        set(CHECK_LIBS Check::check)
    else()
        set(CHECK_LIBS check)
    endif()
    
    # Forzamos la ruta absoluta del include usando el directorio normalizado de vcpkg
    file(TO_CMAKE_PATH "$ENV{VCPKG_ROOT_DIR}" VCPKG_ROOT_NORMALIZED)
    set(CHECK_INCLUDE_DIRS "${VCPKG_ROOT_NORMALIZED}/installed/x64-windows/include")
    
    # 🚀 SOLUCIÓN DEFINITIVA: Le decimos a Visual Studio EXACTAMENTE en qué carpetas buscar el archivo check.lib
    link_directories(
        "${VCPKG_ROOT_NORMALIZED}/installed/x64-windows/lib"
        "${VCPKG_ROOT_NORMALIZED}/installed/x64-windows/debug/lib"
    )
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CHECK REQUIRED check)
    set(CHECK_LIBS ${CHECK_STATIC_LIBRARIES})
endif()

include_directories(
    ${CHECK_INCLUDE_DIRS}
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
target_link_libraries(test_movement_driver PRIVATE host_stubs ${CHECK_LIBS})
add_test(NAME test_movement_driver COMMAND test_movement_driver)

# ==========================================
# Test: test_mqtt_aws
# ==========================================
add_executable(test_mqtt_aws
    test/test_mqtt_aws.c
    ../../components/mqtt-aws/mqtt-aws.c
)
target_link_libraries(test_mqtt_aws PRIVATE host_stubs ${CHECK_LIBS})
add_test(NAME test_mqtt_aws COMMAND test_mqtt_aws)

# ==========================================
# Test: test_sensor_pipeline
# ==========================================
add_executable(test_sensor_pipeline
    test/test_sensor_pipeline.c
    ../../components/movement-driver/movement-driver.c
    ../../components/mqtt-aws/mqtt-aws.c
)
target_link_libraries(test_sensor_pipeline PRIVATE host_stubs ${CHECK_LIBS})
add_test(NAME test_sensor_pipeline COMMAND test_sensor_pipeline)

enable_testing()
