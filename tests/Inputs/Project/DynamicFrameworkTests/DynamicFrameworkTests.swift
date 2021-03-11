import XCTest
@testable import DynamicFramework

class DynamicFrameworkTests: XCTestCase {
    func testIsValid() {
        #if !NO_MUTANT
        XCTAssertTrue(isValid(age: 30))
        #endif
    }
}
