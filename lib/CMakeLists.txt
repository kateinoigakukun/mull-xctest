set(mullXCTest_sources
    Tasks/ExtractRawEmbeddedFileFromBinary.cpp
)
set(mullXCTest_headers
    ${MULL_XCTEST_INCLUDE_DIR}/Tasks/ExtractRawEmbeddedFileFromBinary.h
)

add_library(mullXCTest STATIC
    ${mullXCTest_sources}
    ${mullXCTest_headers})

target_link_libraries(mullXCTest ebc)
