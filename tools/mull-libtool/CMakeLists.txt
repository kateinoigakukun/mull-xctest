add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Options.inc
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Options.td
  COMMAND "${PATH_TO_LLVM}/bin/llvm-tblgen"
    "-gen-opt-parser-defs" "-I${PATH_TO_LLVM}/include" "${CMAKE_CURRENT_SOURCE_DIR}/Options.td"
    "-o" "${CMAKE_CURRENT_BINARY_DIR}/Options.inc"
)

add_executable(mull-libtool
  mull-libtool.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/Options.inc)

target_include_directories(mull-libtool
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mull-libtool
  mullXCTest mullSwiftSupport
  mull MullVersion ebc json11)

install(TARGETS mull-libtool)
