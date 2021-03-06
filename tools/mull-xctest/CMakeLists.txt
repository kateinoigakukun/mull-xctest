llvm_map_components_to_libnames(llvm_libs support)
add_executable(mull-xctest
  mull-xctest.cpp
  XCTestInvocation.cpp
  XCTestInvocation.h)
target_link_libraries(mull-xctest mullXCTest mull MullVersion ${llvm_libs})
