import SwiftSyntax

enum SyntaxMutatorKind: Int {
    // FIXME: These cases are copied from mull::MutationKind.
    // Use more extensible way
    case NegateMutator,
    RemoveVoidFunctionMutator,
    ReplaceCallMutator,
    ScalarValueMutator,

    CXX_AddToSub,
    CXX_AddAssignToSubAssign,
    CXX_PreIncToPreDec,
    CXX_PostIncToPostDec,

    CXX_SubToAdd,
    CXX_SubAssignToAddAssign,
    CXX_PreDecToPreInc,
    CXX_PostDecToPostInc,

    CXX_MulToDiv,
    CXX_MulAssignToDivAssign,

    CXX_DivToMul,
    CXX_DivAssignToMulAssign,

    CXX_RemToDiv,
    CXX_RemAssignToDivAssign,

    CXX_BitwiseNotToNoop,
    CXX_UnaryMinusToNoop,

    CXX_LShiftToRShift,
    CXX_LShiftAssignToRShiftAssign,

    CXX_RShiftToLShift,
    CXX_RShiftAssignToLShiftAssign,

    CXX_Logical_AndToOr,
    CXX_Logical_OrToAnd,

    CXX_Bitwise_OrToAnd,
    CXX_Bitwise_OrAssignToAndAssign,
    CXX_Bitwise_AndToOr,
    CXX_Bitwise_AndAssignToOrAssign,
    CXX_Bitwise_XorToOr,
    CXX_Bitwise_XorAssignToOrAssign,

    CXX_LessThanToLessOrEqual,
    CXX_LessOrEqualToLessThan,
    CXX_GreaterThanToGreaterOrEqual,
    CXX_GreaterOrEqualToGreaterThan,

    CXX_GreaterThanToLessOrEqual,
    CXX_GreaterOrEqualToLessThan,
    CXX_LessThanToGreaterOrEqual,
    CXX_LessOrEqualToGreaterThan,

    CXX_EqualToNotEqual,
    CXX_NotEqualToEqual,

    CXX_AssignConst,
    CXX_InitConst,

    CXX_RemoveNegation,

    Swift_Logical_AndToOr,
    Swift_Logical_OrToAnd
}
typealias LineColumnHash = Int

struct SyntaxMutation {
    let line: Int
    let column: Int
    let kind: Set<SyntaxMutatorKind>
}

func lineColumnHash(line: Int, column: Int) -> LineColumnHash {
    // Use 'Cantor pairing function' to map (Int, Int) to Int
    return (line + column) * (line + column + 1) / 2 + line
}

class SourceUnitStorage {
    let converter: SourceLocationConverter
    private var storage: [LineColumnHash: SyntaxMutation] = [:]

    internal init(converter: SourceLocationConverter) {
        self.converter = converter
    }

    func hasMutation(line: Int, column: Int, mutation kind: SyntaxMutatorKind) -> Bool {
        let hash = lineColumnHash(line: line, column: column)
        guard let mutation = storage[hash] else { return false }
        return mutation.kind.contains(kind)
    }
    func save<S: SyntaxProtocol>(mutation: Set<SyntaxMutatorKind>, node: S) {
        let location = node.startLocation(converter: converter)
        guard let line = location.line, let column = location.column else {
            return
        }
        let hash = lineColumnHash(line: line, column: column)
        storage[hash] = SyntaxMutation(line: line, column: column, kind: mutation)
    }
}

let BINARY_MUTATIONS: [String: Set<SyntaxMutatorKind>] = [
    "+" : [.CXX_AddToSub],
    "-" : [.CXX_SubToAdd],
    "*" : [.CXX_MulToDiv],
    "/" : [.CXX_DivToMul],
    "%" : [.CXX_RemToDiv],

    "+=": [.CXX_AddAssignToSubAssign],
    "-=": [.CXX_SubAssignToAddAssign],
    "*=": [.CXX_MulAssignToDivAssign],
    "/=": [.CXX_DivAssignToMulAssign],
    "%=": [.CXX_RemAssignToDivAssign],

    "==": [.CXX_EqualToNotEqual],
    "!=": [.CXX_NotEqualToEqual],

    "<" : [
        .CXX_LessThanToGreaterOrEqual, // < -> >=
        .CXX_LessThanToLessOrEqual,    // < -> <=
    ],
    ">" : [
        .CXX_GreaterThanToLessOrEqual,    // > -> <=
        .CXX_GreaterThanToGreaterOrEqual, // > -> >=
    ],
    "<=": [
        .CXX_LessOrEqualToGreaterThan, // <= -> >
        .CXX_LessOrEqualToLessThan,    // <= -> <
    ],
    ">=": [
        .CXX_GreaterOrEqualToLessThan,    // >= -> <
        .CXX_GreaterOrEqualToGreaterThan, // >= -> >
    ],
    "&&": [
        .Swift_Logical_AndToOr, // && -> ||
    ]
]

class SourceUnitLocationIndexer: SyntaxAnyVisitor {
    let storage: SourceUnitStorage

    internal init(storage: SourceUnitStorage) {
        self.storage = storage
    }

    override func visitPost(_ node: TokenSyntax) {
        guard let parent = node.parent,
              parent.is(BinaryOperatorExprSyntax.self) else { return }
        switch node.tokenKind {
        case .spacedBinaryOperator(let op):
            guard let mutation = BINARY_MUTATIONS[op] else { break }
            storage.save(mutation: mutation, node: node)
        default:
            break
        }
    }
}

import struct Foundation.URL

enum ErrorCode: Int, Swift.Error {
    case invalidSourcePathChars = 1
    case syntaxParserError
}

@_cdecl("mullIndexSwiftSource")
func IndexSwiftSource(sourcePath: UnsafePointer<UInt8>,
                      errorCode: UnsafeMutablePointer<Int>) -> UnsafeMutableRawPointer? {
    guard let (sourcePath, _) = String.decodeCString(sourcePath, as: UTF8.self) else {
        errorCode.pointee = ErrorCode.invalidSourcePathChars.rawValue
        return nil
    }
    let sourceFile: SourceFileSyntax
    do {
        sourceFile = try SyntaxParser.parse(URL(fileURLWithPath: sourcePath))
    } catch {
        errorCode.pointee = ErrorCode.syntaxParserError.rawValue
        return nil
    }
    let converter = SourceLocationConverter(file: sourcePath, tree: sourceFile)
    let storage = SourceUnitStorage(converter: converter)
    let indexer = SourceUnitLocationIndexer(storage: storage)
    indexer.walk(sourceFile)

    return Unmanaged.passRetained(storage).toOpaque()
}

@_cdecl("mullHasSyntaxMutation")
func HasSyntaxMutation(storage: UnsafeRawPointer, line: Int, column: Int, rawMutatorKind: SyntaxMutatorKind.RawValue) -> Bool {
    let storage = Unmanaged<SourceUnitStorage>.fromOpaque(storage).takeUnretainedValue()
    let mutation = SyntaxMutatorKind(rawValue: rawMutatorKind)!
    return storage.hasMutation(line: line, column: column, mutation: mutation)
}
