cmake_minimum_required(VERSION 3.16)
project(birch-tree VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
    add_compile_options(-Wno-unknown-pragmas)
endif()

if (NOT NOT_USE_CONAN)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
endif()

set (SRC_DIR ${PROJECT_SOURCE_DIR}/src)
add_executable(birch-tree
        ${SRC_DIR}/main.cpp
        )

if (NOT_USE_CONAN)
    find_package(fmt REQUIRED)
    target_include_directories(birch-tree PRIVATE ${fmt_INCLUDE_DIRS})
    target_link_libraries(birch-tree ${fmt_LIBRARIES})
else()
    target_link_libraries(birch-tree ${CONAN_LIBS})
endif()
