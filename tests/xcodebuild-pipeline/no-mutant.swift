// RUN: %empty-directory(%t)
// RUN: export MULL_XCTEST_ARGS="-Xmull --linker=clang -Xmull --linker-flavor=clang"
// RUN: xcodebuild -project %S/../Inputs/Project/Project.xcodeproj -scheme DynamicFramework -destination "%iossim-dest" -derivedDataPath %t build-for-testing %xcodebuild-opts OTHER_LDFLAGS="$MULL_XCTEST_ARGS" SWIFT_ACTIVE_COMPILATION_CONDITIONS=NO_MUTANT
// RUN: xcodebuild test-without-building -destination "%iossim-dest" -xctestrun %t/Build/Products/DynamicFramework_%xctestrun-suffix.xctestrun


// RUN: mull-xctestrun --test-target DynamicFrameworkTests -Xxcodebuild -destination -Xxcodebuild "%iossim-dest" %t/Build/Products/DynamicFramework_%xctestrun-suffix.xctestrun
// RUN-CHECK-NOT: Baseline
