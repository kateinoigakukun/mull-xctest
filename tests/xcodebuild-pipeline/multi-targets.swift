// RUN: %empty-directory(%t)
// RUN: export MULL_XCTEST_ARGS="-Xmull --linker=clang -Xmull --linker-flavor=clang"
// RUN: xcodebuild -project %S/../Inputs/Project/Project.xcodeproj -scheme MultiTargets -destination "%iossim-dest" -derivedDataPath %t build-for-testing %xcodebuild-opts OTHER_LDFLAGS="$MULL_XCTEST_ARGS"
// RUN: xcodebuild test-without-building -destination "%iossim-dest" -xctestrun %t/Build/Products/MultiTargets_iphonesimulator14.4-x86_64.xctestrun

// RUN: mull-xctestrun -Xxcodebuild -destination -Xxcodebuild "platform=iOS Simulator,name=iPhone 8" %t/Build/Products/MultiTargets_iphonesimulator14.4-x86_64.xctestrun | FileCheck %s --check-prefix=RUN-CHECK
// RUN-CHECK-DAG: DynamicFramework/Library.swift:3:16: warning: Survived: Replaced > with >= [cxx_gt_to_ge]
// RUN-CHECK-DAG: DynamicFramework/Library.swift:3:16: warning: Survived: Replaced > with <= [cxx_gt_to_le]
// RUN-CHECK-DAG: StaticFramework/StaticFramework.swift:2:16: warning: Survived: Replaced > with >= [cxx_gt_to_ge]
// RUN-CHECK-DAG: StaticFramework/StaticFramework.swift:2:16: warning: Survived: Replaced > with <= [cxx_gt_to_le]
