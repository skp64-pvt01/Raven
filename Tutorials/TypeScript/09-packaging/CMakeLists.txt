# Notes (TypeScript) - packaging and installation.
#
# Nothing is compiled: CMake installs the sources, the interface file, the Raven
# library and a launcher, and CPack packs that tree.
#
#   export RAVEN_LIBRARY=/path/to/libraven.so    # the library ships in the package
#   cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr
#   cd build && cpack -G DEB
#   sudo apt install ./notes_1.0.0_amd64.deb
#
# See README.md for what each line does and how to check the result.

cmake_minimum_required(VERSION 3.20)
project(notes VERSION 1.0.0 LANGUAGES NONE
	DESCRIPTION "Notes - a small text editor written with Raven (TypeScript)")

# No target architecture is known (LANGUAGES NONE), so CMake cannot guess the
# library folder. Say it once; only bin/, share/ and lib/ are used below.
set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "")
include(GNUInstallDirs)

set(COURSE "${CMAKE_CURRENT_SOURCE_DIR}/..")

# ---------------------------------------------------------------------------
#  Install
# ---------------------------------------------------------------------------

# The launcher: a shell script that starts deno with the right permissions.
install(PROGRAMS notes DESTINATION ${CMAKE_INSTALL_BINDIR})

# The modules the application imports. main.ts sits one folder deeper, so its
# "../raven.ts" and "../tutorial.ts" find these files.
install(FILES
	"${COURSE}/raven.ts"
	"${COURSE}/tutorial.ts"
	DESTINATION ${CMAKE_INSTALL_DATADIR}/notes)
install(FILES
	"${COURSE}/08-capstone-notes/main.ts"
	"${COURSE}/08-capstone-notes/notes.xml"
	DESTINATION ${CMAKE_INSTALL_DATADIR}/notes/app)

install(FILES notes.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

# The FFI binding needs the shared library. Shipping it means the package works
# on a machine where Raven is not installed.
if(DEFINED ENV{RAVEN_LIBRARY})
	# RAVEN_LIBRARY often points at a symlink (libraven.so -> libraven.so.0 ->
	# libraven.so.0.2.0). REALPATH copies the library itself, not the link.
	get_filename_component(RAVEN_LIBRARY_REAL "$ENV{RAVEN_LIBRARY}" REALPATH)
	install(FILES "${RAVEN_LIBRARY_REAL}"
		DESTINATION ${CMAKE_INSTALL_LIBDIR} RENAME libraven.so)
else()
	message(WARNING "RAVEN_LIBRARY is not set: the package will not include libraven.so")
endif()

# ---------------------------------------------------------------------------
#  CPack
# ---------------------------------------------------------------------------

set(CPACK_PACKAGE_NAME "notes")
set(CPACK_PACKAGE_VENDOR "Raven tutorials")
set(CPACK_PACKAGE_CONTACT "Raven tutorials <you@example.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
	"Notes - a small text editor written with Raven (TypeScript)")
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
	# A distribution would package deno; no repository provides it today, so it
	# is a recommendation rather than a dependency.
	set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "deno")
endif()

include(CPack)
