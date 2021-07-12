public struct SourceLocation: Equatable, Comparable, CustomStringConvertible, Hashable {
    public var filePath: String
    public var line: UInt
    public var column: UInt
    /// Note: Ignores column because muter and mull report in different style for column
    public static func < (lhs: SourceLocation, rhs: SourceLocation) -> Bool {
        guard lhs.filePath == rhs.filePath else {
            return lhs.filePath < rhs.filePath
        }
        return lhs.line < rhs.line
    }
    public var description: String {
        "\(filePath):\(line)"
    }
}

public struct MullMutationPoint {
    public var mutator: String
    public var location: SourceLocation
}

public struct MuterMutationPoint: Decodable {
    public var mutationOperatorId: String
    public var location: SourceLocation

    enum TopKeys: CodingKey {
        case mutationOperatorId
        case position
        case filePath
    }
    enum PositionKeys: CodingKey {
        case line, column
    }

    public init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: TopKeys.self)
        self.mutationOperatorId = try container.decode(String.self, forKey: .mutationOperatorId)
        let filePath = try container.decode(String.self, forKey: .filePath)
        let position = try container.nestedContainer(keyedBy: PositionKeys.self,
                                                     forKey: .position)
        self.location = try SourceLocation(
            filePath: filePath,
            line: position.decode(UInt.self, forKey: .line),
            column: position.decode(UInt.self, forKey: .column)
        )
    }
}
