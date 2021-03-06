// RUN: %empty-directory(%t)
// RUN: export MULL_XCTEST_ARGS="-Xmull --linker=clang -Xmull --linker-flavor=clang"
// RUN: xcodebuild -project %S/../Inputs/Project/Project.xcodeproj -scheme DynamicFramework -destination "%iossim-dest" -derivedDataPath %t build-for-testing %xcodebuild-opts OTHER_LDFLAGS="$MULL_XCTEST_ARGS"
// RUN: xcodebuild test-without-building -destination "%iossim-dest" -xctestrun %t/Build/Products/DynamicFramework_iphonesimulator14.4-x86_64.xctestrun
// RUN: mull-dump-mutants %t/Build/Products/Debug-iphonesimulator/DynamicFramework.framework/DynamicFramework | FileCheck %s --check-prefix=DUMP-CHECK

// DUMP-CHECK-DAG: Mutation Point: cxx_gt_to_ge {{.*}}/Library.swift:2:16
// DUMP-CHECK-DAG: Mutation Point: cxx_gt_to_le {{.*}}/Library.swift:2:16


// RUN: mull-xctestrun --test-target DynamicFrameworkTests -Xxcodebuild -destination -Xxcodebuild "platform=iOS Simulator,name=iPhone 8" %t/Build/Products/DynamicFramework_iphonesimulator14.4-x86_64.xctestrun | FileCheck %s --check-prefix=RUN-CHECK
// RUN-CHECK: Library.swift:2:16: warning: Survived: Replaced > with >= [cxx_gt_to_ge]
