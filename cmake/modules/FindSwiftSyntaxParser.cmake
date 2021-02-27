function(find_toolchain_tool result_var_name toolchain tool)
  execute_process(
      COMMAND "xcrun" "--toolchain" "${toolchain}" "--find" "${tool}"
      OUTPUT_VARIABLE tool_path
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  set("${result_var_name}" "${tool_path}" PARENT_SCOPE)
endfunction()

find_toolchain_tool(swiftc_path "XcodeDefault" swiftc)
get_filename_component(swift_lib "${swiftc_path}/../../lib/swift" REALPATH)

find_path(SwiftSyntaxParser_INCLUDE_DIR
  NAMES "SwiftSyntaxParser.h"
  PATHS "${swift_lib}/_InternalSwiftSyntaxParser")

find_library(SwiftSyntaxParser_LIBRARY
  NAMES "_InternalSwiftSyntaxParser"
  PATHS "${swift_lib}/macosx")
mark_as_advanced(
  SwiftSyntaxParser_INCLUDE_DIR
  SwiftSyntaxParser_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SwiftSyntaxParser
  REQUIRED_VARS
    SwiftSyntaxParser_INCLUDE_DIR
    SwiftSyntaxParser_LIBRARY)

if(SwiftSyntaxParser_FOUND AND NOT TARGET SwiftSyntaxParser::SwiftSyntaxParser)
  add_library(SwiftSyntaxParser::SwiftSyntaxParser UNKNOWN IMPORTED)
  set_target_properties(SwiftSyntaxParser::SwiftSyntaxParser PROPERTIES
    IMPORTED_LOCATION "${SwiftSyntaxParser_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SwiftSyntaxParser_INCLUDE_DIR}")
endif()
