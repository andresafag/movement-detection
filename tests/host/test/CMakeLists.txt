cmake_minimum_required(VERSION 3.10)
project(SimpleTenTest C)

# 1. Configurar el estándar de C
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 2. Buscar la librería "Check" instalada en el sistema
find_library(CHECK_LIB NAMES check)

if(NOT CHECK_LIB)
    message(FATAL_ERROR "¡No se encontró la librería Check! Por favor, instálala primero.")
endif()

# 3. Definir el ejecutable y sus archivos fuentes (.c)
add_executable(ejecutable_pruebas math_utils.c test_simple.c)

# 4. Vincular la librería Check al ejecutable
target_link_libraries(ejecutable_pruebas PRIVATE ${CHECK_LIB})

# 5. Agregar librerías auxiliares solo si estás en Linux (para evitar fallos en GitHub Actions)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_library(SUBUNIT_LIB NAMES subunit)
    if(SUBUNIT_LIB)
        target_link_libraries(ejecutable_pruebas PRIVATE ${SUBUNIT_LIB} m pthread rt)
    endif()
endif()
