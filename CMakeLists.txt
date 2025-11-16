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
        include/Hist.h)

target_link_libraries(GAsm PRIVATE nlohmann_json::nlohmann_json)

target_include_directories(GAsm PRIVATE include)
