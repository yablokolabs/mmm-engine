cmake_minimum_required(VERSION 3.27)

project(mmm_engine CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-O3 -march=native -ffast-math")

find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        SYSTEM
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

add_library(mmm_core STATIC
    src/adstock.cpp
    src/saturation.cpp
    src/channels.cpp
    src/trend.cpp
    src/seasonality.cpp
    src/mmm.cpp
    src/json_io.cpp
    src/optimizer.cpp
    src/linalg.cpp
    src/data_io.cpp
    src/fit.cpp
)
target_include_directories(mmm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(mmm_core PUBLIC nlohmann_json::nlohmann_json)

add_executable(mmm src/cli.cpp)
target_link_libraries(mmm PRIVATE mmm_core)

enable_testing()
add_executable(mmm_tests tests/test_mmm.cpp)
target_link_libraries(mmm_tests PRIVATE mmm_core)
target_compile_definitions(mmm_tests PRIVATE MMM_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME mmm_tests COMMAND mmm_tests)
