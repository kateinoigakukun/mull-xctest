cmake_minimum_required(VERSION 3.14)

project(MullXCTest
  VERSION
    0.1.0
  LANGUAGES
    C CXX
)

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PATH_TO_LLVM "" CACHE PATH "Path to installed LLVM or LLVM source tree")

find_package(LLVM REQUIRED CONFIG PATHS ${PATH_TO_LLVM} NO_DEFAULT_PATH)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(MULL_XCTEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/MullXCTest)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_subdirectory(lib)
add_subdirectory(vendor)
add_subdirectory(tools)
