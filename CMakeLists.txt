cmake_minimum_required(VERSION 3.26)
project(GAsm)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.12.0   # or latest
)

FetchContent_MakeAvailable(json)

FetchContent_Declare(
        xbyak
        GIT_REPOSITORY https://github.com/herumi/xbyak.git
        GIT_TAG        v7.30   # or latest
)

FetchContent_MakeAvailable(xbyak)

#find_package(Sleef REQUIRED) # if package available
## or FetchContent for the source
#target_link_libraries(GAsm PRIVATE Sleef::sleef)

add_executable(GAsm
        main.cpp
        src/GAsmParser.cpp
        include/GAsmParser.h
        src/GAsm.cpp
        include/GAsm.h
        src/GP.cpp
        include/GP.h
        src/Entry.cpp
        include/Entry.h
        src/Hist.cpp
        include/Hist.h
        src/functions.cpp
        include/functions.h
        src/GAsmInterpreter.cpp
        include/GAsmInterpreter.h
        src/jit_moves.cpp
        include/jit_moves.h
        src/GAsmCompiler.cpp
        src/GAsmCompiler.cpp
        src/tests.cpp
        include/fib_learn.h)

target_link_libraries(GAsm PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(GAsm PRIVATE ${xbyak_SOURCE_DIR}/xbyak)

target_include_directories(GAsm PRIVATE include)
