# Test executable
add_executable(publisher_tests
    test_publisher.cpp
)

# Link against library and GoogleTest
target_link_libraries(publisher_tests
    PRIVATE
        counter::counter
        GTest::gtest_main
)

# Discover tests for CTest
include(GoogleTest)
gtest_discover_tests(publisher_tests
    PROPERTIES
        LABELS "unit"
)
