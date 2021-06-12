set(headers
    ${MULL_XCTEST_INCLUDE_DIR}/SwiftSupport/SyntaxMutationFinder.h
    ${MULL_XCTEST_INCLUDE_DIR}/SwiftSupport/RuntimeFunctionFilter.h
)

add_library(mullSwiftSupport
  SyntaxMutationFinder.cpp
  RuntimeFunctionFilter.cpp
  ${headers})

target_link_libraries(mullSwiftSupport
  SwiftSyntax::SwiftSyntax
  SwiftSyntaxParser::SwiftSyntaxParser)

add_subdirectory(core)
add_dependencies(mullSwiftSupport mullSwiftSupportCore)
link_support_core(mullSwiftSupport)
