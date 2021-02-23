llvm_map_components_to_libnames(llvm_libs support object)
add_executable(mull-ld mull-ld.cpp)
target_link_libraries(mull-ld mull ebc json11 MullVersion mullXCTest ${llvm_libs})
