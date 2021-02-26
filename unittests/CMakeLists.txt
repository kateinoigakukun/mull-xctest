include(GoogleTest)

set(mull_xctest_unittests_sources
  MutantSerializationTests.cpp)

add_executable(mullXCTest-tests EXCLUDE_FROM_ALL ${mull_xctest_unittests_sources})
gtest_discover_tests(mullXCTest-tests)
target_link_libraries(mullXCTest-tests
  mullXCTest
  google-test)
