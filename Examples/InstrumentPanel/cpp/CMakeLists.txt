cmake_minimum_required(VERSION 3.20)
project(instrument_panel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raven REQUIRED)

# panel.cpp is the reusable pane; Examples/Workbench compiles the same file.
add_executable(instrument_panel main.cpp panel.cpp)
target_link_libraries(instrument_panel PRIVATE raven::raven)
