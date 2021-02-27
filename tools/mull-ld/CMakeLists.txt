llvm_map_components_to_libnames(llvm_libs all)
add_executable(mull-ld
  mull-ld.cpp
  LinkerInvocation.cpp
  LinkerInvocation.h)

target_link_libraries(mull-ld
  mullXCTest mullSwiftFilter
  mull MullVersion ebc json11
  ${llvm_libs})
add_custom_target(ld ALL
  COMMAND ${CMAKE_COMMAND}
    -E create_symlink $<TARGET_FILE:mull-ld> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ld
  DEPENDS mull-ld)
