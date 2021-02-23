include(ExternalProject)
set(PATH_TO_MULL "${CMAKE_CURRENT_SOURCE_DIR}/vendor/mull")
ExternalProject_Add(libmull
  SOURCE_DIR
    ${PATH_TO_MULL}
  CMAKE_ARGS
    -DCMAKE_INSTALL_LIBDIR=lib
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DPATH_TO_LLVM=${PATH_TO_LLVM}
    -DCMAKE_CXX_FLAGS=-fno-rtti)

ExternalProject_Get_property(libmull BINARY_DIR)
add_library(mull STATIC IMPORTED GLOBAL)
set_target_properties(mull
  PROPERTIES
    IMPORTED_LOCATION
      ${BINARY_DIR}/lib/libmull.a
    INTERFACE_INCLUDE_DIRECTORIES
      ${PATH_TO_MULL}/include)
