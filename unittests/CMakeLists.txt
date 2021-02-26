include(GoogleTest)

set(mull_xctest_unittests_sources
  )

add_executable(mull-tests EXCLUDE_FROM_ALL ${mull_xctest_unittests_sources})
gtest_discover_tests(mull-tests)
target_link_libraries(mull-tests
  mullXCTest
  google-test)
