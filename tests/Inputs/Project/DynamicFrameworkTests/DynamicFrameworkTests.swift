import XCTest
@testable import DynamicFramework

class DynamicFrameworkTests: XCTestCase {
    func testIsValid() {
        XCTAssertTrue(isValid(age: 30))
    }
}
