cmake_minimum_required(VERSION 3.14)

project(MullXCTest
  VERSION
    0.1.0
  LANGUAGES
    C CXX Swift
)

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  set(CMAKE_INSTALL_RPATH @executable_path/../lib/mull-support)
endif()

set(PATH_TO_LLVM "" CACHE PATH "Path to installed LLVM or LLVM source tree")

find_package(LLVM REQUIRED CONFIG PATHS ${PATH_TO_LLVM} NO_DEFAULT_PATH)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

find_package(SwiftSyntaxParser REQUIRED)
include(SwiftSyntax)

set(MULL_XCTEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MULL_XCTEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/MullXCTest)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_subdirectory(lib)
add_subdirectory(vendor)
add_subdirectory(tools)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
endif()

if(BUILD_TESTING)
  add_subdirectory(tests)
  add_subdirectory(unittests)
endif()
