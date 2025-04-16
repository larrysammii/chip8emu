cmake_minimum_required(VERSION 3.30)
project(chip8emu)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(chip8emu)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.inl"
)

target_sources(chip8emu PRIVATE ${SOURCES})

target_include_directories(chip8emu PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

#set_property(GLOBAL PROPERTY USE_FOLDERS ON)