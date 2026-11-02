# Raven tutorials - builds every step of the course.
#
#   cmake -S . -B build
#   cmake --build build
#   ctest --test-dir build --output-on-failure
#
# Needs Raven installed (the raven-dev package, or `cmake --install` of a
# source build): find_package finds ravenConfig.cmake.

cmake_minimum_required(VERSION 3.20)
project(raven_tutorials LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

find_package(raven REQUIRED)

# One executable per tutorial, and one headless test per executable.
function(raven_tutorial name)
	add_executable(${name} ${name}/main.cpp)
	target_link_libraries(${name} PRIVATE raven::raven)
	# Where the .xml files of the step live: the program reads them at run time.
	target_compile_definitions(${name} PRIVATE
		RAVEN_TUTORIAL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/${name}")
	add_test(NAME ${name} COMMAND ${name} --test)
endfunction()

raven_tutorial(01-first-window)
raven_tutorial(02-layout)
raven_tutorial(03-controls)
raven_tutorial(04-actions-and-shortcuts)
raven_tutorial(05-xml-interface)
raven_tutorial(06-menus)
raven_tutorial(07-text-editor)
raven_tutorial(08-capstone-notes)
