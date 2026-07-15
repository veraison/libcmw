cmake_minimum_required(VERSION 3.16)

add_executable(cmw_tests
    test_collection.cpp
    test_indicator.cpp
    test_monad.cpp
    test_sniff.cpp
    test_tn.cpp
    test_cmw.cpp
)

target_link_libraries(cmw_tests
    PRIVATE
        cmw
        GTest::gtest_main
)

target_include_directories(cmw_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(cmw_tests PRIVATE
    CMW_TESTDATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/testdata"
)

include(GoogleTest)
gtest_discover_tests(cmw_tests)

add_executable(cmw_interop interop_cxx.cpp)
target_link_libraries(cmw_interop PRIVATE cmw)
