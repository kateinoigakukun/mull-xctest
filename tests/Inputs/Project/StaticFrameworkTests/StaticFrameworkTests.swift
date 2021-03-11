import XCTest
@testable import StaticFramework

class StaticFrameworkTests: XCTestCase {
    func testIsValid() {
        XCTAssertTrue(isValid(age: 30))
    }
}
