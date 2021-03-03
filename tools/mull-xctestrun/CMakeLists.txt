llvm_map_components_to_libnames(llvm_libs support object)
add_executable(mull-xctestrun
  mull-xctestrun.cpp
  XCTestRunInvocation.cpp
  XCTestRunInvocation.h
  XCTestRunFile.cpp
  XCTestRunFile.h)
target_link_libraries(mull-xctestrun
  mullXCTest mull MullVersion ebc json11 ${llvm_libs})
