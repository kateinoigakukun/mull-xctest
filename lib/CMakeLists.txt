set(mullXCTest_sources
    Tasks/ExtractEmbeddedFileTask.cpp
    Tasks/LoadBitcodeFromBufferTask.cpp
)
set(mullXCTest_headers
    ${MULL_XCTEST_INCLUDE_DIR}/Tasks/ExtractEmbeddedFileTask.h
    ${MULL_XCTEST_INCLUDE_DIR}/Tasks/LoadBitcodeFromBufferTask.h
)

add_library(mullXCTest STATIC
    ${mullXCTest_sources}
    ${mullXCTest_headers})

target_link_libraries(mullXCTest ebc)
