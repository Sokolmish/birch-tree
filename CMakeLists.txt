cmake_minimum_required(VERSION 3.16)
project(birch-tree VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

set (SRC_DIR ${PROJECT_SOURCE_DIR}/src)

add_executable(birch-tree
        ${SRC_DIR}/main.cpp)

find_package(fmt REQUIRED)
target_include_directories(birch-tree PRIVATE ${FMT_INCLUDE_DIRS})
target_link_libraries(birch-tree fmt)
