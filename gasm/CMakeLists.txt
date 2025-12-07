add_library(gasm STATIC
        src/GAsmParser.cpp
        src/GAsm.cpp
        src/GP.cpp
        src/Entry.cpp
        src/Hist.cpp
        src/functions.cpp
        src/GAsmInterpreter.cpp
        src/GAsmCompiler.cpp
        src/Individual.cpp
        include/Individual.h
        src/Runner.cpp
        include/Runner.h
        include/utils.h
)

# Force C++23
#target_compile_features(gasm PUBLIC cxx_std_23)


target_include_directories(gasm PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${xbyak_SOURCE_DIR}/xbyak
)

target_link_libraries(gasm PUBLIC
        nlohmann_json::nlohmann_json
)
