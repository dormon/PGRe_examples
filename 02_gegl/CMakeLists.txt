cmake_minimum_required(VERSION 3.15)

project(02_gegl)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1 ) 

add_subdirectory(libs/SDL2-2.0.16)
add_subdirectory(libs/geGL)

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} SDL2-static SDL2::SDL2main geGL::geGL)

add_custom_target(run ./${PROJECT_NAME})
