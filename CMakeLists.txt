#cmake_minimum_required(VERSION 3.26)
#project(gasm_python LANGUAGES CXX)
#
#set(CMAKE_CXX_STANDARD 23)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#
##set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
#
#include(FetchContent)
#
## -----------------------------------------------
## Fetch external dependencies
## -----------------------------------------------
## nlohmann/json
#FetchContent_Declare(
#        json
#        GIT_REPOSITORY https://github.com/nlohmann/json.git
#        GIT_TAG v3.12.0
#)
#FetchContent_MakeAvailable(json)
#
## Xbyak
#FetchContent_Declare(
#        xbyak
#        GIT_REPOSITORY https://github.com/herumi/xbyak.git
#        GIT_TAG v7.30
#)
#FetchContent_MakeAvailable(xbyak)
#
## -----------------------------------------------
## Build options
## -----------------------------------------------
#option(BUILD_PYTHON "Build Python extension module" OFF)
#option(BUILD_EXAMPLES "Build C++ example CLI" ON)
#
### -----------------------------------------------
### Core C++ library
### -----------------------------------------------
##add_subdirectory(gasm)
#
## -----------------------------------------------
## Python extension module
## -----------------------------------------------
#if (BUILD_PYTHON)
#    add_subdirectory(python)
#endif()
#
## -----------------------------------------------
## C++ example CLI
## -----------------------------------------------
#if (BUILD_EXAMPLES)
#    add_subdirectory(examples/cpp)
#endif()
#
## ----------------------------
## Python module
## ----------------------------
#find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
#
#if(NOT DEFINED EXT_SUFFIX)
#    message(FATAL_ERROR "EXT_SUFFIX is not defined")
#endif()

#Python3_add_library(gasm_python MODULE
#        gasm/python/GasmPython.cpp
#        gasm/src/GAsmParser.cpp
#        gasm/src/GAsm.cpp
#        gasm/src/GP.cpp
#        gasm/src/Entry.cpp
#        gasm/src/Hist.cpp
#        gasm/src/functions.cpp
#        gasm/src/GAsmInterpreter.cpp
#        gasm/src/GAsmCompiler.cpp
#        gasm/src/jit_moves.cpp
#        gasm/src/tests.cpp
#)
#
#target_link_libraries(gasm_python PRIVATE
#        Python3::Module
#        nlohmann_json::nlohmann_json
#)
#
#target_include_directories(gasm_python PRIVATE
#        gasm/include
#        gasm/python
#        ${xbyak_SOURCE_DIR}/xbyak
#        ${Python3_INCLUDE_DIRS}
#        ${json_SOURCE_DIR}/single_include
#)
#
#set_target_properties(gasm_python PROPERTIES
#        OUTPUT_NAME "gasm"
#        PREFIX ""
#        SUFFIX "${EXT_SUFFIX}"
#)

cmake_minimum_required(VERSION 3.26)
project(GAsm)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    # Tell the Windows SDK which architecture we are building for,
    # otherwise ntdef.h/winnt.h can emit "No Target Architecture".
    add_compile_definitions(_AMD64_)
endif()


# -----------------------------------------------
# Build options
# -----------------------------------------------
option(BUILD_PYTHON "Build Python extension module" OFF)
option(BUILD_EXAMPLES "Build C++ example CLI" ON)

# -----------------------------------------------
# Python package
# -----------------------------------------------
if(NOT BUILD_PYTHON)
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
    include_directories(${Python3_INCLUDE_DIRS})
endif()

include(FetchContent)

# ----------------------------
# nlohmann/json
# ----------------------------
FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.12.0
)
FetchContent_MakeAvailable(json)

# ----------------------------
# Xbyak
# ----------------------------
FetchContent_Declare(
        xbyak
        GIT_REPOSITORY https://github.com/herumi/xbyak.git
        GIT_TAG        v7.30
)
FetchContent_MakeAvailable(xbyak)

# ----------------------------
# Executable build
# ----------------------------
if (BUILD_EXAMPLES)
    add_subdirectory(gasm)
    add_subdirectory(examples/cpp)
endif()

# ----------------------------
# Python module
# ----------------------------
if (BUILD_PYTHON)
    set(Python3_ROOT_DIR "C:/Users/mateu/AppData/Local/Programs/Python/Python311")
    set(Python3_FIND_REGISTRY FIRST)
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

    if(NOT DEFINED EXT_SUFFIX)
        message(FATAL_ERROR "EXT_SUFFIX is not defined")
    endif()

    # Build a shared library for Python
    add_library(GasmPython MODULE
            gasm/python/GasmPython.cpp
            gasm/python/GasmPython.h
            gasm/src/GAsmParser.cpp
            gasm/include/GAsmParser.h
            gasm/src/GAsm.cpp
            gasm/include/GAsm.h
            gasm/src/GP.cpp
            gasm/include/GP.h
            gasm/src/Entry.cpp
            gasm/include/Entry.h
            gasm/src/Hist.cpp
            gasm/include/Hist.h
            gasm/src/functions.cpp
            gasm/include/functions.h
            gasm/src/GAsmInterpreter.cpp
            gasm/include/GAsmInterpreter.h
            gasm/src/GAsmCompiler.cpp
    )

    target_compile_options(GasmPython PRIVATE /std:c++20)


    # Fix name so Python can import it: gasm.pyd or gasm.so
    set_target_properties(GasmPython PROPERTIES
            PREFIX ""                # remove 'lib' prefix
            OUTPUT_NAME "gasm"       # Python module name
            SUFFIX "${EXT_SUFFIX}"   # Python pyd extension
    )

    target_include_directories(GasmPython PRIVATE
            gasm/include
            ${xbyak_SOURCE_DIR}/xbyak
            ${Python3_INCLUDE_DIRS}
    )

    target_link_libraries(GasmPython PRIVATE
            Python3::Python
            nlohmann_json::nlohmann_json
    )
endif()
