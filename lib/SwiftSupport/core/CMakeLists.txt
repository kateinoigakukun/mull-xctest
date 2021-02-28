
if(CMAKE_GENERATOR STREQUAL "Xcode")
  add_library(mullSwiftSupportCore SwiftASTMutationFilter.swift)
  target_compile_options(mullSwiftSupportCore
    PRIVATE -I $<TARGET_PROPERTY:SwiftSyntax::SwiftSyntax,INTERFACE_INCLUDE_DIRECTORIES>)
  add_dependencies(mullSwiftSupportCore SwiftSyntax::SwiftSyntax)
else()
  # Swift on Darwin can't produce static library with -g option because dsymutil doesn't
  # accept archives, so use object library instead of static library.
  # And CMake doesn't support object library for Swift with Ninja
  get_filename_component(swiftc_bin "${CMAKE_Swift_COMPILER}/../swiftc" ABSOLUTE)
  get_filename_component(swift_lib "${CMAKE_Swift_COMPILER}/../../lib/swift/macosx" ABSOLUTE)

  set(swift_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/SwiftASTMutationFilter.swift)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/SwiftASTMutationFilter.swift.o
    DEPENDS
      ${swift_sources}
      SwiftSyntax::SwiftSyntax
    COMMAND
      ${swiftc_bin} -emit-object
        -parse-as-library
        -module-name MullSwiftSupportCore
        -o ${CMAKE_CURRENT_BINARY_DIR}/SwiftASTMutationFilter.swift.o
        -runtime-compatibility-version 5.1 $<$<CONFIG:Debug>:-g>
        -I $<TARGET_PROPERTY:SwiftSyntax::SwiftSyntax,INTERFACE_INCLUDE_DIRECTORIES>
        -sdk ${CMAKE_OSX_SYSROOT}
        ${swift_sources})
  add_custom_target(mullSwiftSupportCore_obj
    SOURCES ${CMAKE_CURRENT_BINARY_DIR}/SwiftASTMutationFilter.swift.o)
  add_library(mullSwiftSupportCore STATIC IMPORTED GLOBAL)
  add_dependencies(mullSwiftSupportCore mullSwiftSupportCore_obj)
  target_link_options(mullSwiftSupportCore
    INTERFACE
      ${CMAKE_CURRENT_BINARY_DIR}/SwiftASTMutationFilter.swift.o
      -lswiftCore -L/usr/lib/swift
      -L${swift_lib})
endif()

macro(link_support_core tgt)
  if(CMAKE_GENERATOR STREQUAL "Xcode")
    target_link_libraries("${tgt}" mullSwiftSupportCore)
  else()
    target_link_options("${tgt}"
      PUBLIC $<TARGET_PROPERTY:mullSwiftSupportCore,INTERFACE_LINK_OPTIONS>)
  endif()
endmacro()
