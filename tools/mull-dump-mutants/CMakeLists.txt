llvm_map_components_to_libnames(llvm_libs all)
add_executable(mull-dump-mutants
  mull-dump-mutants.cpp)
target_link_libraries(mull-dump-mutants mull ebc json11 MullVersion mullXCTest ${llvm_libs})
