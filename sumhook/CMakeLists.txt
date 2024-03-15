cmake_minimum_required(VERSION 3.12.0)

add_library(sumhook src/sumhook.cpp)
target_include_directories(sumhook PUBLIC include)
target_compile_features(sumhook PUBLIC cxx_std_20)

option(SUMHOOK_TEST "Build the tests" OFF)

if(SUMHOOK_TEST)
    add_executable(sumhook-test test/main.cpp)
    target_link_libraries(sumhook-test PRIVATE sumhook)

    find_package(zydis REQUIRED)
    target_link_libraries(sumhook-test PRIVATE Zydis::Zydis)
endif()
