cmake_minimum_required(VERSION 3.20)
project(widebrim C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/madhatter/src" OR NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/madhatter/include")
    message(FATAL_ERROR
        "The madhatter submodule is missing. Run: git submodule update --init --recursive"
    )
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED sdl3)

file(GLOB MADHATTER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/madhatter/src/*.c"
)

add_library(madhatter STATIC ${MADHATTER_SOURCES})

target_include_directories(madhatter PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/madhatter/include
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/madhatter/include
    ${SDL3_INCLUDE_DIRS}
)

file(GLOB WIDEBRIM_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

set(WINDOW "sdl3" CACHE STRING "Window backend to build")
set_property(CACHE WINDOW PROPERTY STRINGS "sdl3")

if(WINDOW STREQUAL "sdl3")
    list(APPEND WIDEBRIM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/window/sdl.c"
    )
else()
    message(FATAL_ERROR "Unsupported window backend '${WINDOW}'. Supported values: sdl3")
endif()

set(RENDERER "sdl3" CACHE STRING "Renderer backend to build")
set_property(CACHE RENDERER PROPERTY STRINGS "sdl3")

if(RENDERER STREQUAL "sdl3")
    list(APPEND WIDEBRIM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/sdl.c"
    )
else()
    message(FATAL_ERROR "Unsupported renderer backend '${RENDERER}'. Supported values: sdl3")
endif()

add_executable(widebrim ${WIDEBRIM_SOURCES})

target_compile_definitions(widebrim PRIVATE "RENDERER=${RENDERER}" "WINDOW=${WINDOW}")

target_include_directories(widebrim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/madhatter/include
    ${SDL3_INCLUDE_DIRS}
)

target_link_libraries(widebrim PRIVATE
    ${SDL3_LINK_LIBRARIES}
    madhatter
)

# Work around pkg-config library ordering on some platforms.
if(APPLE)
    target_link_options(widebrim PRIVATE -Wl,-framework,CoreFoundation)
endif()
