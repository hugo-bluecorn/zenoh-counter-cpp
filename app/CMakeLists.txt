# Executable targets

add_executable(counter_pub
    main.cpp
)
target_link_libraries(counter_pub
    PRIVATE
        counter::counter
)

add_executable(counter_sub
    counter_sub.cpp
)
target_link_libraries(counter_sub
    PRIVATE
        zenohcpp
)

add_executable(counter_router
    counter_router.cpp
)
target_link_libraries(counter_router
    PRIVATE
        zenohcpp
)
