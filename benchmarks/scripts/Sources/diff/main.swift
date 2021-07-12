import Parser
import Foundation

let args = CommandLine.arguments
let mullFile = args[1]
let muterFile = args[2]

let mullResult = try parseMullBenchmark(contents: Data(contentsOf: URL(fileURLWithPath: mullFile)))
let muterResult = try parseMuterBenchmark(contents: Data(contentsOf: URL(fileURLWithPath: muterFile)))

enum UniversalMutator: String, Equatable, Comparable {
    case RelationalOperatorReplacement
    case RemoveSideEffects
    case ChangeLogicalConnector

    static func < (lhs: UniversalMutator, rhs: UniversalMutator) -> Bool {
        return lhs.rawValue < rhs.rawValue
    }
}

struct MutationPoint: Equatable, Comparable, CustomStringConvertible, Hashable {
    let mutator: UniversalMutator
    let location: SourceLocation

    init?(mullPoint: MullMutationPoint) {
        let mutatorMap: [String: UniversalMutator?] = [
            "cxx_add_assign_to_sub_assign": nil,
            "cxx_add_to_sub": nil,
            "cxx_div_to_mul": nil,
            "cxx_ge_to_gt": .RelationalOperatorReplacement,
            "cxx_ge_to_lt": .RelationalOperatorReplacement,
            "cxx_gt_to_ge": .RelationalOperatorReplacement,
            "cxx_gt_to_le": .RelationalOperatorReplacement,
            "cxx_lt_to_ge": .RelationalOperatorReplacement,
            "cxx_lt_to_le": .RelationalOperatorReplacement,
            "cxx_mul_assign_to_div_assign": nil,
            "cxx_rem_to_div": nil,
            "cxx_remove_void_call": .RemoveSideEffects,
            "cxx_sub_assign_to_add_assign": nil,
            "cxx_sub_to_add": nil,
            "swift_eq_to_ne": nil,
            "swift_logical_and_to_or": .ChangeLogicalConnector,
        ]
        guard let knownMutator = mutatorMap[mullPoint.mutator] else {
            fatalError("unknown mutator \(mullPoint.mutator)")
        }
        guard let comparableMutator = knownMutator else {
            return nil
        }
        var location = mullPoint.location
        let prefixRange = mullPoint.location.filePath.range(of: "benchmarks/build")!
        let filePath = mullPoint.location.filePath.suffix(from: prefixRange.upperBound)
        location.filePath = String(filePath)
        self.mutator = comparableMutator
        self.location = location
    }

    init(muterPoint: MuterMutationPoint) {
        self.mutator = UniversalMutator(rawValue: muterPoint.mutationOperatorId)!
        let prefixRange = muterPoint.location.filePath.range(of: "TemporaryItems/")!
        let filePathString = String(muterPoint.location.filePath.suffix(from: prefixRange.upperBound))
        let startIndex = filePathString.firstIndex(of: "/")!
        let filePath = URL(fileURLWithPath: String(filePathString[startIndex...]))
        var location = muterPoint.location
        location.filePath = filePath.path
        self.location = location
    }

    static func < (lhs: MutationPoint, rhs: MutationPoint) -> Bool {
        guard lhs.location == rhs.location else {
            return lhs.location < rhs.location
        }
        return lhs.mutator < rhs.mutator
    }

    var description: String {
        return "\(mutator):\(location)"
    }
}

func makeTempFile() throws -> URL {
    var mktempPath: String {
        #if os(Linux)
        return "/bin/mktemp"
        #else
        return "/usr/bin/mktemp"
        #endif
    }
    var (path, _) = try Process.exec(bin: mktempPath, arguments: [])
    path = path.trimmingCharacters(in: .whitespacesAndNewlines)
    return URL(fileURLWithPath: path)
}

func diff(lhs: [MutationPoint], rhs: [MutationPoint]) throws {
    let lhs = Set(lhs)
    let rhs = Set(rhs)
    let added = lhs.subtracting(rhs)
    let deleted = rhs.subtracting(lhs)
    print("Equals = ", lhs.intersection(rhs).count)
    print(added.map(\.description).map { "+\($0)" }.joined(separator: "\n"))
    print(deleted.map(\.description).map { "-\($0)" }.joined(separator: "\n"))
//    let lhs = lhs.sorted().map(\.description).joined(separator: "\n")
//    let rhs = rhs.sorted().map(\.description).joined(separator: "\n")
//    let lhsFile = try makeTempFile()
//    let rhsFile = try makeTempFile()
//    try lhs.write(to: lhsFile, atomically: true, encoding: .utf8)
//    try rhs.write(to: rhsFile, atomically: true, encoding: .utf8)
//
//    print("/usr/bin/diff", lhsFile.path, rhsFile.path)
//    try Process.exec(bin: "/usr/bin/diff", arguments: [lhsFile.path, rhsFile.path])
}

try diff(lhs: mullResult.compactMap(MutationPoint.init(mullPoint: )),
         rhs: muterResult.map(MutationPoint.init(muterPoint: )))
