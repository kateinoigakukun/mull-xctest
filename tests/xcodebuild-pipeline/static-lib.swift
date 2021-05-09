// RUN: %empty-directory(%t)
// RUN: export MULL_XCTEST_ARGS="-Xmull --linker=clang -Xmull --linker-flavor=clang"
// RUN: xcodebuild -project %S/../Inputs/Project/Project.xcodeproj -scheme StaticFramework -destination "%iossim-dest" -derivedDataPath %t build-for-testing %xcodebuild-opts OTHER_LDFLAGS="$MULL_XCTEST_ARGS"
// RUN: xcodebuild test-without-building -destination "%iossim-dest" -xctestrun %t/Build/Products/StaticFramework_%xctestrun-suffix.xctestrun
// RUN: mull-dump-mutants %t/Build/Products/Debug-iphonesimulator/StaticFrameworkTests.xctest/StaticFrameworkTests | FileCheck %s --check-prefix=DUMP-CHECK

// DUMP-CHECK-DAG: Mutation Point: cxx_gt_to_ge {{.*}}/StaticFramework.swift:2:16
// DUMP-CHECK-DAG: Mutation Point: cxx_gt_to_le {{.*}}/StaticFramework.swift:2:16


// RUN: mull-xctestrun --test-target StaticFrameworkTests -Xxcodebuild -destination -Xxcodebuild "%iossim-dest" %t/Build/Products/StaticFramework_%xctestrun-suffix.xctestrun | FileCheck %s --check-prefix=RUN-CHECK
// RUN-CHECK: StaticFramework.swift:2:16: warning: Survived: Replaced > with >= [cxx_gt_to_ge]
