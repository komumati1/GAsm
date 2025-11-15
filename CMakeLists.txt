cmake_minimum_required(VERSION 3.26)
project(GAsm)

set(CMAKE_CXX_STANDARD 23)

add_executable(GAsm main.cpp
        GAsmParser.cpp
        GAsmParser.h
        GAsm.cpp
        GAsm.h
        GP.cpp
        GP.h)
