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

# Subscriber tests
add_executable(subscriber_tests
    test_subscriber.cpp
)

target_link_libraries(subscriber_tests
    PRIVATE
        counter::counter
        GTest::gtest_main
)

# Pass the counter_sub binary path to the test so it can launch it as a subprocess
target_compile_definitions(subscriber_tests
    PRIVATE
        COUNTER_SUB_BIN="$<TARGET_FILE:counter_sub>"
)

# Discover tests for CTest
include(GoogleTest)
gtest_discover_tests(publisher_tests
    PROPERTIES
        LABELS "unit"
)
gtest_discover_tests(subscriber_tests
    PROPERTIES
        LABELS "unit"
)

# Integration tests (peer direct + router topology)
add_executable(integration_tests
    test_integration.cpp
)

target_link_libraries(integration_tests
    PRIVATE
        counter::counter
        GTest::gtest_main
)

gtest_discover_tests(integration_tests
    PROPERTIES
        LABELS "integration"
)
