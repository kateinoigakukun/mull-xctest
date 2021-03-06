llvm_map_components_to_libnames(llvm_libs support)
add_executable(mull-dump-mutants
  mull-dump-mutants.cpp)
target_link_libraries(mull-dump-mutants mull MullVersion mullXCTest ${llvm_libs})
