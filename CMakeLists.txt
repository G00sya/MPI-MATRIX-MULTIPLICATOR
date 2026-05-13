cmake_minimum_required(VERSION 3.20)

project(
    mpi_matrix_multiplier
    VERSION 1.0.0
    DESCRIPTION "MPI-based matrix multiplication with benchmark support"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(FMT_DOC OFF CACHE BOOL "" FORCE)
set(FMT_TEST OFF CACHE BOOL "" FORCE)
set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/fmt EXCLUDE_FROM_ALL)

add_library(project_mpi INTERFACE)

if(WIN32)
    find_package(MSMPI REQUIRED)
    target_link_libraries(project_mpi INTERFACE MSMPI::MSMPI)
else()
    find_package(MPI REQUIRED COMPONENTS CXX)
    target_link_libraries(project_mpi INTERFACE MPI::MPI_CXX)
endif()

add_executable(mpi_matrix_multiplier
    src/main.cpp
    src/program_options.cpp
    src/matrix.cpp
    src/distributed_multiplier.cpp
    src/benchmark.cpp
    include/program_options.hpp
    include/matrix.hpp
    include/distributed_multiplier.hpp
    include/benchmark.hpp
)

target_include_directories(mpi_matrix_multiplier
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(mpi_matrix_multiplier
    PRIVATE
        fmt::fmt
        project_mpi
)

if(MSVC)
    target_compile_options(mpi_matrix_multiplier PRIVATE /W4 /permissive-)
else()
    target_compile_options(mpi_matrix_multiplier PRIVATE -Wall -Wextra -Wpedantic)
endif()
