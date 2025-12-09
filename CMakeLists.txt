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
# Boost arbitrary precision
# ----------------------------
foreach(boost_lib IN ITEMS
        config
        assert
        core
        static_assert
        integer
        throw_exception
        predef
        type_traits
        mpl
        rational
        multiprecision
)
    FetchContent_Declare(
            boost_${boost_lib}
            GIT_REPOSITORY https://github.com/boostorg/${boost_lib}.git
            GIT_TAG boost-1.84.0               # choose a version you want
    )
    FetchContent_MakeAvailable(boost_${boost_lib})
endforeach()

# Add include dirs for all Boost modules
foreach(boost_lib IN ITEMS
        config
        assert
        core
        static_assert
        integer
        throw_exception
        predef
        type_traits
        mpl
        rational
        multiprecision
)
    include_directories(
            ${boost_${boost_lib}_SOURCE_DIR}/include
    )
endforeach()

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
    set(Python3_ROOT_DIR "C:/Users/mateu/AppData/Local/Programs/Python/Python311") # TODO your own interpreter
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
            gasm/src/Individual.cpp
            gasm/include/Individual.h
            gasm/src/Runner.cpp
            gasm/include/Runner.h
            gasm/include/utils.h
            gasm/python/HistPython.cpp
            gasm/python/HistPython.h
            gasm/python/GasmModule.cpp
            gasm/python/GasmModule.h
            gasm/python/IndividualPython.cpp
            gasm/python/IndividualPython.h
            gasm/python/utils.cpp
            gasm/python/utils.h
            gasm/python/EntryPython.cpp
            gasm/python/EntryPython.h
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
