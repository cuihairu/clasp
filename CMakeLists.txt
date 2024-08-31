cmake_minimum_required(VERSION 3.14)
project(clasp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(clasp
        src/clasp.cpp
        src/command.cpp
        src/flag.cpp
)

target_include_directories(clasp PUBLIC include)

add_executable(basic_example examples/basic_example.cpp)
target_link_libraries(basic_example PRIVATE clasp)
