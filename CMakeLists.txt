cmake_minimum_required(VERSION 3.19)
set(CMAKE_TOOLCHAIN_FILE $ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")
project(sam_tuner)

find_package(yaml-cpp REQUIRED)
find_package(tabulate CONFIG REQUIRED)
find_package(argparse CONFIG REQUIRED)

add_executable(${PROJECT_NAME} run.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE yaml-cpp tabulate argparse)

install(FILES helper.h helper.tpp sam_tune_util.h DESTINATION include/${PROJECT_NAME})