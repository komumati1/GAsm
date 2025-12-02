cmake_minimum_required(VERSION 3.26)
project(gasm_python LANGUAGES CXX)

# Dependencies
include(FetchContent)

FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
        xbyak
        GIT_REPOSITORY https://github.com/herumi/xbyak.git
        GIT_TAG v7.30
)
FetchContent_MakeAvailable(xbyak)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)

if(NOT DEFINED EXT_SUFFIX)
    message(FATAL_ERROR "EXT_SUFFIX is not defined")
endif()

# Add the parent gasm library
#add_subdirectory(.. ${CMAKE_CURRENT_BINARY_DIR}/gasm_build)

Python3_add_library(gasm_python MODULE
        GasmPython.cpp
        ../src/GAsmParser.cpp
        ../src/GAsm.cpp
        ../src/GP.cpp
        ../src/Entry.cpp
        ../src/Hist.cpp
        ../src/functions.cpp
        ../src/GAsmInterpreter.cpp
        ../src/GAsmCompiler.cpp
)

target_compile_features(gasm_python PUBLIC cxx_std_23)

target_link_libraries(gasm_python PRIVATE
#        gasm
        Python3::Module
        nlohmann_json::nlohmann_json
)

target_include_directories(gasm_python PRIVATE
        ../include
        ${xbyak_SOURCE_DIR}/xbyak
        ${Python3_INCLUDE_DIRS}
)

set_target_properties(gasm_python PROPERTIES
        OUTPUT_NAME "gasm"
        PREFIX ""
        SUFFIX "${EXT_SUFFIX}"
)

target_include_directories(gasm_python PRIVATE
        ${Python3_INCLUDE_DIRS}
        ${json_SOURCE_DIR}/single_include
        ../include
)
