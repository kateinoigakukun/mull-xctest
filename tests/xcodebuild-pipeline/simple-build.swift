// RUN: export MULL_XCTEST_ARGS="--linker clang --linker-flavor=clang"
// RUN: xcodebuild -project %S/../Inputs/Project/Project.xcodeproj -scheme DynamicFramework -destination "%iossim-dest" build-for-testing %xcodebuild-opts
