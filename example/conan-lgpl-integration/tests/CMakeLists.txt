find_package(GTest REQUIRED CONFIG)

add_executable(myapp_tests test_compression.cpp)

target_link_libraries(myapp_tests PRIVATE
    compression
    GTest::gtest
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(myapp_tests)
