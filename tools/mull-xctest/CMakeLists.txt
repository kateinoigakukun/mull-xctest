llvm_map_components_to_libnames(llvm_libs support object)
add_executable(mull-xctest
  mull-xctest.cpp
  XCTestInvocation.cpp
  XCTestInvocation.h)
target_link_libraries(mull-xctest mullXCTest mull ebc json11 MullVersion ${llvm_libs})

install(TARGETS mull-xctest)
