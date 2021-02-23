llvm_map_components_to_libnames(llvm_libs support object)
add_executable(mull-xctest mull-xctest.cpp)
target_link_libraries(mull-xctest mull ebc json11 MullVersion ${llvm_libs})
