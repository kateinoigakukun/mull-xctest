
if(CMAKE_GENERATOR STREQUAL "Xcode")
  add_library(mullSwiftFilterCore SwiftASTMutationFilter.swift)
  target_compile_options(mullSwiftFilterCore
    PRIVATE -I $<TARGET_PROPERTY:SwiftSyntax::SwiftSyntax,INTERFACE_INCLUDE_DIRECTORIES>)
  add_dependencies(mullSwiftFilterCore SwiftSyntax::SwiftSyntax)
else()
  # Swift on Darwin can't produce static library with -g option because dsymutil doesn't
  # accept archives, so use object library instead of static library.
  # And CMake doesn't support object library for Swift with Ninja
  get_filename_component(swiftc_bin "${CMAKE_Swift_COMPILER}/../swiftc" ABSOLUTE)

  set(swift_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/SwiftASTMutationFilter.swift)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/SwiftASTMutationFilter.swift.o
    DEPENDS
      ${swift_sources}
      SwiftSyntax::SwiftSyntax
    COMMAND
      ${swiftc_bin} -emit-object
        -o ${CMAKE_CURRENT_BINARY_DIR}/SwiftASTMutationFilter.swift.o
        -I $<TARGET_PROPERTY:SwiftSyntax::SwiftSyntax,INTERFACE_INCLUDE_DIRECTORIES>
        -sdk ${CMAKE_OSX_SYSROOT}
        ${swift_sources})

  add_library(mullSwiftFilterCore STATIC IMPORTED GLOBAL)
  target_link_options(mullSwiftFilterCore
    INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/SwiftASTMutationFilter.swift.o)
endif()
