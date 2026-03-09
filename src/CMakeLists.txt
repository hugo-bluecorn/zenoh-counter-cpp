# Library target
add_library(mylib
    mylib.cpp
)

# Public headers for consumers
target_include_directories(mylib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# zenoh-cpp (provides zenoh-c, include paths, and ZENOHCXX_ZENOHC define)
target_link_libraries(mylib
    PUBLIC
        zenohcpp
)

# Alias for consistent usage
add_library(mylib::mylib ALIAS mylib)
