set(mullXCTest_sources
    Tasks/ExtractEmbeddedFileTask.cpp
    Tasks/LoadBitcodeFromBufferTask.cpp
    Tasks/EmbedMutantInfoTask.cpp
    MutantSerialization.cpp
    XCTestRunFile.cpp
    XCResultFile.cpp
    MutationPipeline.cpp
)
set(mullXCTest_headers
    ${MULL_XCTEST_INCLUDE_DIR}/Tasks/ExtractEmbeddedFileTask.h
    ${MULL_XCTEST_INCLUDE_DIR}/Tasks/LoadBitcodeFromBufferTask.h
    ${MULL_XCTEST_INCLUDE_DIR}/Tasks/EmbedMutantInfoTask.h
    ${MULL_XCTEST_INCLUDE_DIR}/MutantSerialization.h
    ${MULL_XCTEST_INCLUDE_DIR}/PthreadTaskExecutor.h
    ${MULL_XCTEST_INCLUDE_DIR}/XCTestRunFile.h
    ${MULL_XCTEST_INCLUDE_DIR}/XCResultFile.h
    ${MULL_XCTEST_INCLUDE_DIR}/MutationPipeline.h
)

add_library(mullXCTest STATIC
    ${mullXCTest_sources}
    ${mullXCTest_headers})

target_link_libraries(mullXCTest mull ebc)

add_subdirectory(SwiftSupport)
