cmake_minimum_required(VERSION 3.16)
project(WebPi VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_library(GPIOD_LIBRARY gpiod)

if(NOT GPIOD_LIBRARY)
    message(WARNING "libgpiod not found. Skipping '${SOURCE_NAME}'.\n sudo apt install libgpiod-dev")
else()

set(GPIODWRAP "${PROJECT_SOURCE_DIR}")
set(GPIODWRAP_BIN "${GPIODWRAP}/bin")

add_library(gpiodWrap INTERFACE)

target_sources(gpiodWrap
    INTERFACE
    FILE_SET HEADERS
    TYPE HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES gpiodWrap.hpp
)

add_subdirectory(examples)

endif()
