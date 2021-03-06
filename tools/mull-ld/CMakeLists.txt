file(DOWNLOAD
  "https://raw.githubusercontent.com/llvm/llvm-project/release/11.x/lld/MachO/Options.td"
  ${CMAKE_CURRENT_BINARY_DIR}/ld64/Options.td)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ld64/Options.inc
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ld64/Options.td
  COMMAND "${PATH_TO_LLVM}/bin/llvm-tblgen"
    "-gen-opt-parser-defs" "-I${PATH_TO_LLVM}/include" "${CMAKE_CURRENT_BINARY_DIR}/ld64/Options.td"
    "-o" "${CMAKE_CURRENT_BINARY_DIR}/ld64/Options.inc"
)

file(DOWNLOAD
  "https://raw.githubusercontent.com/llvm/llvm-project/release/11.x/clang/include/clang/Driver/Options.td"
  ${CMAKE_CURRENT_BINARY_DIR}/clang/Options.td)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/clang/Options.inc
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/clang/Options.td
  COMMAND "${PATH_TO_LLVM}/bin/llvm-tblgen"
    "-gen-opt-parser-defs" "-I${PATH_TO_LLVM}/include" "${CMAKE_CURRENT_BINARY_DIR}/clang/Options.td"
    "-o" "${CMAKE_CURRENT_BINARY_DIR}/clang/Options.inc"
)


add_executable(mull-ld
  mull-ld.cpp
  LinkerInvocation.cpp
  LinkerInvocation.h
  LinkerOptions.cpp
  LinkerOptions.h
  ${CMAKE_CURRENT_BINARY_DIR}/ld64/Options.inc
  ${CMAKE_CURRENT_BINARY_DIR}/clang/Options.inc)

target_include_directories(mull-ld
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mull-ld
  mullXCTest mullSwiftSupport
  mull MullVersion ebc json11)
add_custom_target(ld ALL
  COMMAND ${CMAKE_COMMAND}
    -E create_symlink $<TARGET_FILE:mull-ld> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ld
  DEPENDS mull-ld)

install(TARGETS mull-ld)

foreach(path ${CMAKE_MODULE_PATH})
  if(EXISTS ${path}/InstallSymlink.cmake)
    set(INSTALL_SYMLINK ${path}/InstallSymlink.cmake)
    break()
  endif()
endforeach()

install(SCRIPT ${INSTALL_SYMLINK}
  CODE "install_symlink(ld ../mull-ld bin/mull-tools)"
  COMPONENT ${component})

install(FILES $<TARGET_PROPERTY:SwiftSyntaxParser::SwiftSyntaxParser,IMPORTED_LOCATION>
DESTINATION lib/mull-support)
