# Notes - the starter project of the Raven C++ tutorials.
#
# This folder builds on its own, because it is meant to be copied:
#
#   cmake -S . -B build
#   cmake --build build
#   ./build/notes            a window
#   ./build/notes --test     no window, no GPU
#
# Needs Raven installed (the raven-dev package, or `cmake --install` of a
# source build).

cmake_minimum_required(VERSION 3.20)
project(notes LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raven REQUIRED)

add_executable(notes main.cpp)
target_link_libraries(notes PRIVATE raven::raven)
# The XML is read at run time: keep the source folder as the base path.
target_compile_definitions(notes PRIVATE RAVEN_TUTORIAL_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

# When you distribute the application, add the usual rules and point the XML
# path at the installed copy, for example:
#
#   include(GNUInstallDirs)
#   install(TARGETS notes RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
#   install(FILES notes.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/notes)
