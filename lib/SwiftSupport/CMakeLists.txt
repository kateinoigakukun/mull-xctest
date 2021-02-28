set(headers
    ${MULL_XCTEST_INCLUDE_DIR}/SwiftSupport/SyntaxMutationFinder.h
    ${MULL_XCTEST_INCLUDE_DIR}/SwiftSupport/SyntaxMutationFilter.h
    ${MULL_XCTEST_INCLUDE_DIR}/SwiftSupport/SourceStorage.h
)

add_library(mullSwiftSupport
  SyntaxMutationFilter.cpp
  SyntaxMutationFinder.cpp
  SourceStorage.cpp
  ${headers})

target_link_libraries(mullSwiftSupport
  SwiftSyntax::SwiftSyntax
  SwiftSyntaxParser::SwiftSyntaxParser)

add_subdirectory(core)
add_dependencies(mullSwiftSupport mullSwiftSupportCore)
link_support_core(mullSwiftSupport)
