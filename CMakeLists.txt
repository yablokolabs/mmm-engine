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

add_executable(mmm
    src/cli.cpp
    src/adstock.cpp
    src/saturation.cpp
    src/channels.cpp
    src/trend.cpp
    src/seasonality.cpp
    src/mmm.cpp
    src/json_io.cpp
    src/optimizer.cpp
)

target_link_libraries(mmm nlohmann_json::nlohmann_json)
target_include_directories(mmm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
