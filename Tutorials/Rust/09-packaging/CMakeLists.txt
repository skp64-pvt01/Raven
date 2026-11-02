# Notes (Rust) - packaging and installation.
#
# CMake does not build the Rust program: it drives cargo, then installs and
# packs the result with CPack. The application stays a normal cargo project.
#
#   export RAVEN_LIB_DIR=/usr/lib RAVEN_STATIC=1     # a binary with no libraven.so
#   cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr
#   cmake --build build
#   cd build && cpack -G DEB
#   sudo apt install ./notes_1.0.0_amd64.deb
#
# See README.md for what each line does and how to check the result.

cmake_minimum_required(VERSION 3.20)
project(notes VERSION 1.0.0 LANGUAGES NONE
	DESCRIPTION "Notes - a small text editor written with Raven")

# No target architecture is known (LANGUAGES NONE), so CMake cannot guess the
# library folder. Say it once; only bin/ and share/ are used below.
set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "")
include(GNUInstallDirs)

find_program(CARGO cargo REQUIRED)

# ---------------------------------------------------------------------------
#  Build: cargo, invoked by CMake so that `cmake --build` does everything
# ---------------------------------------------------------------------------

set(RUST_COURSE "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(RUST_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/cargo")
set(NOTES_BINARY "${RUST_TARGET_DIR}/release/08-capstone-notes")

add_custom_command(
	OUTPUT "${NOTES_BINARY}"
	COMMAND ${CMAKE_COMMAND} -E env
		"CARGO_TARGET_DIR=${RUST_TARGET_DIR}"
		# Compiled into the binary with option_env! in main.rs.
		"NOTES_DATA_DIR=${CMAKE_INSTALL_FULL_DATADIR}/notes"
		"${CARGO}" build --release --manifest-path "${RUST_COURSE}/Cargo.toml"
			-p step-08-capstone-notes
	DEPENDS
		"${RUST_COURSE}/08-capstone-notes/src/main.rs"
		"${RUST_COURSE}/08-capstone-notes/notes.xml"
	WORKING_DIRECTORY "${RUST_COURSE}"
	COMMENT "cargo build --release -p step-08-capstone-notes"
	VERBATIM)

add_custom_target(notes ALL DEPENDS "${NOTES_BINARY}")

# ---------------------------------------------------------------------------
#  Install
# ---------------------------------------------------------------------------

install(PROGRAMS "${NOTES_BINARY}"
	DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME notes)
install(FILES "${RUST_COURSE}/08-capstone-notes/notes.xml"
	DESTINATION ${CMAKE_INSTALL_DATADIR}/notes)
install(FILES notes.desktop
	DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

# ---------------------------------------------------------------------------
#  CPack
# ---------------------------------------------------------------------------

set(CPACK_PACKAGE_NAME "notes")
set(CPACK_PACKAGE_VENDOR "Raven tutorials")
set(CPACK_PACKAGE_CONTACT "Raven tutorials <you@example.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
	"Notes - a small text editor written with Raven (Rust)")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "notes")

set(CPACK_GENERATOR "TGZ")
if(UNIX AND NOT APPLE)
	set(CPACK_GENERATOR "TGZ;DEB")

	set(CPACK_DEBIAN_PACKAGE_NAME "notes")
	set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
	set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Raven tutorials <you@example.com>")
	set(CPACK_DEBIAN_PACKAGE_SECTION "editors")
	set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
	# Only the system libraries are left: libraven is linked statically.
	set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()

include(CPack)
