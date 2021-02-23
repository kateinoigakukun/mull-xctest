llvm_map_components_to_libnames(llvm_libs support)
add_executable(mull-xctest mull-xctest.cpp)
target_link_libraries(mull-xctest mull MullXCTestVersion ${llvm_libs})
