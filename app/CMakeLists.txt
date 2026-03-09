# Executable target
add_executable(counter_pub
    main.cpp
)

# Link against library
target_link_libraries(counter_pub
    PRIVATE
        counter::counter
)
