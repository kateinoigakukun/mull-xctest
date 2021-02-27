get_filename_component(swiftc_bin "${CMAKE_Swift_COMPILER}/../swiftc" ABSOLUTE)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/SwiftASTMutationFilter.swift.o
  DEPENDS
    SwiftASTMutationFilter.swift
    SwiftSyntax::SwiftSyntax
  COMMAND
    ${swiftc_bin} -emit-object
      -o ${CMAKE_CURRENT_BINARY_DIR}/SwiftASTMutationFilter.swift.o
      -I $<TARGET_PROPERTY:SwiftSyntax::SwiftSyntax,INTERFACE_INCLUDE_DIRECTORIES>
      -sdk ${CMAKE_OSX_SYSROOT}
      ${CMAKE_CURRENT_SOURCE_DIR}/SwiftASTMutationFilter.swift)

add_custom_target(mullSwiftFilterCore
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/SwiftASTMutationFilter.swift.o)


add_library(mullSwiftFilter
  SwiftASTMutationFilter.cpp
  SwiftRawSyntax.h)

target_link_libraries(mullSwiftFilter
  SwiftSyntax::SwiftSyntax
  SwiftSyntaxParser::SwiftSyntaxParser)

# Workaround: CMake produces "order-only phony target" when depending on swift library,
# but it produces "multiple rules generate" error. So link using target_link_options 
# instead of target_link_libraries.
# And Swift on Darwin can't produce static library with -g option because dsymutil doesn't
# accept archives, so use object library instead of static library.
# $<OBJECTS:mullSwiftFilterCore>
add_dependencies(mullSwiftFilter mullSwiftFilterCore)
target_link_options(mullSwiftFilter
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/SwiftASTMutationFilter.swift.o)
