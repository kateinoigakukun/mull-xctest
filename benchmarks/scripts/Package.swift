// swift-tools-version:5.3

import PackageDescription

let package = Package(
    name: "Scripts",
    targets: [
        .target(name: "diff", dependencies: ["Parser"]),
        .target(name: "Parser", dependencies: []),
    ]
)
