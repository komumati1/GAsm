add_executable(gasm_example
        main.cpp
        tests.cpp
)

target_link_libraries(gasm_example PRIVATE
        gasm
)

target_include_directories(gasm_example PUBLIC
        .
)
