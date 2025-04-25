cmake_minimum_required(VERSION 3.30)
project(chip8emu)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(${PROJECT_NAME})

find_package(SDL2 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        glad::glad
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.inl"
)

target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

#set_property(GLOBAL PROPERTY USE_FOLDERS ON)