add_executable(gasm_example main.cpp)

target_link_libraries(gasm_example PRIVATE
        gasm
        jit_moves.cpp
        tests.cpp
)
