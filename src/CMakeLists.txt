# Library target
add_library(counter_lib
    publisher.cpp
)

# Public headers for consumers
target_include_directories(counter_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# zenoh-cpp (provides zenoh-c, include paths, and ZENOHCXX_ZENOHC define)
target_link_libraries(counter_lib
    PUBLIC
        zenohcpp
)

# Alias for consistent usage
add_library(counter::counter ALIAS counter_lib)
