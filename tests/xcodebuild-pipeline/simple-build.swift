// RUN: %empty-directory(%t)
// RUN: export MULL_XCTEST_ARGS="--linker clang --linker-flavor=clang"
// RUN: xcodebuild -project %S/../Inputs/Project/Project.xcodeproj -scheme DynamicFramework -destination "%iossim-dest" -derivedDataPath %t build-for-testing %xcodebuild-opts
// RUN: xcodebuild test-without-building -destination "%iossim-dest" -xctestrun %t/Build//Products/DynamicFramework_iphonesimulator14.4-x86_64.xctestrun
