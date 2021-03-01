include(ExternalProject)

get_filename_component(swift_build "${CMAKE_Swift_COMPILER}/../swift-build" REALPATH)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(swift_build_type "debug")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(swift_build_type "release")
else()
  message(FATAL_ERROR "Unexpected build type: ${CMAKE_BUILD_TYPE}")
endif()

ExternalProject_Add(SwiftSyntax
  GIT_REPOSITORY    https://github.com/apple/swift-syntax.git
  GIT_TAG           0.50300.0
  CONFIGURE_COMMAND ""
  INSTALL_COMMAND   ""
  BUILD_COMMAND ${swift_build}
    "--product" "SwiftSyntax" "--configuration" ${swift_build_type}
    "--package-path" "<SOURCE_DIR>"
    "--build-path" "<BINARY_DIR>"
  BUILD_BYPRODUCTS <BINARY_DIR>/${swift_build_type}/libSwiftSyntax.a
)

ExternalProject_Get_property(SwiftSyntax BINARY_DIR)

add_custom_target(SwiftSyntax_ranlib
  COMMAND ${CMAKE_RANLIB} "${BINARY_DIR}/${swift_build_type}/libSwiftSyntax.a"
  DEPENDS SwiftSyntax)

add_library(SwiftSyntax::SwiftSyntax STATIC IMPORTED GLOBAL)
add_dependencies(SwiftSyntax::SwiftSyntax SwiftSyntax_ranlib)

execute_process(COMMAND
  ${CMAKE_COMMAND} "-E" "make_directory" "${BINARY_DIR}/${swift_build_type}")

set_target_properties(SwiftSyntax::SwiftSyntax PROPERTIES
  IMPORTED_LOCATION "${BINARY_DIR}/${swift_build_type}/libSwiftSyntax.a"
  INTERFACE_INCLUDE_DIRECTORIES "${BINARY_DIR}/${swift_build_type}")
