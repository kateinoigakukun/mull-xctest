import SwiftSyntax
import Mull_C

typealias SyntaxMutatorKind = CMutatorKind

class SourceUnitStorage {
    let inner: CASTMutationStorage
    let sourceFile: String
    let converter: SourceLocationConverter

    internal init(inner: CASTMutationStorage,
                  sourceFile: String,
                  converter: SourceLocationConverter) {
        self.inner = inner
        self.sourceFile = sourceFile
        self.converter = converter
    }
    func save<S: SyntaxProtocol>(mutation: Set<SyntaxMutatorKind>, node: S) {
        let location = node.startLocation(converter: converter)
        guard let line = location.line, let column = location.column else {
            return
        }
        sourceFile.withCString { sourceFilePtr in
            for kind in mutation {
                mull_ASTMutationStorage_saveMutation(
                    inner, sourceFilePtr, kind, Int32(line), Int32(column)
                )
            }
        }
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

    "==": [.Swift_EqualToNotEqual],
    "!=": [.Swift_NotEqualToEqual],

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

    override func visitPost(_ node: AssignmentExprSyntax) {
        let mutation = SyntaxMutatorKind.CXX_AssignConst
        storage.save(mutation: [mutation], node: node)
    }

    override func visitPost(_ node: FunctionCallExprSyntax) {
        let mutation = SyntaxMutatorKind.CXX_RemoveVoidCall
        storage.save(mutation: [mutation], node: node)
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

@_cdecl("mull_swift_IndexSource")
func IndexSwiftSource(sourcePath: UnsafePointer<UInt8>,
                      storagePtr: CASTMutationStorage,
                      errorCode: UnsafeMutablePointer<Int>) {
    guard let (sourcePath, _) = String.decodeCString(sourcePath, as: UTF8.self) else {
        errorCode.pointee = ErrorCode.invalidSourcePathChars.rawValue
        return
    }
    let sourceFile: SourceFileSyntax
    do {
        sourceFile = try SyntaxParser.parse(URL(fileURLWithPath: sourcePath))
    } catch {
        errorCode.pointee = ErrorCode.syntaxParserError.rawValue
        return
    }
    let converter = SourceLocationConverter(file: sourcePath, tree: sourceFile)
    let storage = SourceUnitStorage(inner: storagePtr, sourceFile: sourcePath,
                                    converter: converter)
    let indexer = SourceUnitLocationIndexer(storage: storage)
    indexer.walk(sourceFile)
}
