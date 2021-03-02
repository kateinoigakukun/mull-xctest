import XCTest
@testable import DynamicFramework

class DynamicFrameworkTests: XCTestCase {
    func testIsValid() {
        XCTAssertFalse(isValid(age: 20))
    }
}
