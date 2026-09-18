cmake_minimum_required(VERSION 3.20)
project(widebrim C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED sdl3)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    /usr/local/include
    ${SDL3_INCLUDE_DIRS}
)

add_executable(widebrim
    src/main.c
    src/madhatter_bridge.c
    src/engine.h
    src/renderer.c
    src/game_state.c
    src/mode.c
)

target_include_directories(widebrim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    /usr/local/include
    ${SDL3_INCLUDE_DIRS}
)

target_link_libraries(widebrim PRIVATE
    ${SDL3_LINK_LIBRARIES}
    /usr/local/lib/libmadhatter.a
)

# Work around pkg-config library ordering on some platforms.
if(APPLE)
    target_link_options(widebrim PRIVATE -Wl,-framework,CoreFoundation)
endif()
