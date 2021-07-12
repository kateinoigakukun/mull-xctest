import Foundation

public func parseMuterBenchmark(contents: Data) throws -> [MuterMutationPoint] {
    let decoder = JSONDecoder()
    decoder.keyDecodingStrategy = .useDefaultKeys
    struct TopLevel: Decodable {
        struct FileReport: Decodable {
            struct AppliedOperator: Decodable {
                var mutationPoint: MuterMutationPoint
            }
            var appliedOperators: [AppliedOperator]
        }
        var fileReports: [FileReport]
    }
    let top = try decoder.decode(TopLevel.self, from: contents)
    return top.fileReports.flatMap(\.appliedOperators).map(\.mutationPoint)
}

public enum Error: Swift.Error, LocalizedError {
    case invalidUTF8
    case invalidPrefix
}

public func parseMullBenchmark(contents: Data) throws -> [MullMutationPoint] {
    func parseSourceLocation<S: StringProtocol>(line: S) throws -> SourceLocation {
        let components = line.split(separator: ":")
        return SourceLocation(
            filePath: String(components[0]),
            line: UInt(components[1])!, column: UInt(components[2])!
        )
    }
    func parseLine<S: StringProtocol>(line: S) throws -> MullMutationPoint {
        let prefix = "Mutation Point: "
        guard line.hasPrefix(prefix) else {
            throw Error.invalidPrefix
        }
        var line = line.dropFirst(prefix.count)
        let mutator = line.prefix(while: { !$0.isWhitespace })
        line = line.dropFirst(mutator.count + 1)
        let location = try parseSourceLocation(line: line)
        return MullMutationPoint(
            mutator: String(mutator), location: location
        )
    }
    guard let contents = String(data: contents, encoding: .utf8) else {
        throw Error.invalidUTF8
    }
    let lines = contents.split(whereSeparator: \.isNewline)
    return try lines.map { try parseLine(line: $0) }
}
