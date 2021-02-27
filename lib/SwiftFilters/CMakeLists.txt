add_library(mullSwiftFilter
  SwiftASTMutationFilter.cpp
  SwiftRawSyntax.h)

target_link_libraries(mullSwiftFilter
  SwiftSyntax::SwiftSyntax
  SwiftSyntaxParser::SwiftSyntaxParser)

add_subdirectory(core)
add_dependencies(mullSwiftFilter mullSwiftFilterCore)
