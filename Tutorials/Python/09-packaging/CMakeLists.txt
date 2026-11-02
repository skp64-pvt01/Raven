# Notes (Python) - packaging and installation.
#
# Nothing is compiled: CMake installs the script, its data file and a launcher,
# and CPack packs that tree. `.py` files are not executables a system can run by
# name, so a two-line launcher in /usr/bin is what the desktop entry starts.
#
#   cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr
#   cd build && cpack -G DEB
#   sudo apt install ./notes_1.0.0_amd64.deb
#
# See README.md for what each line does and how to check the result.

cmake_minimum_required(VERSION 3.20)
project(notes VERSION 1.0.0 LANGUAGES NONE
	DESCRIPTION "Notes - a small text editor written with Raven (Python)")

# No target architecture is known (LANGUAGES NONE), so CMake cannot guess the
# library folder. Say it once; only bin/ and share/ are used below.
set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "")
include(GNUInstallDirs)

set(NOTES_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../08-capstone-notes")

# ---------------------------------------------------------------------------
#  Install
# ---------------------------------------------------------------------------

# The launcher: a shell script next to the real entry point.
install(PROGRAMS notes DESTINATION ${CMAKE_INSTALL_BINDIR})

# The application and the interface file, kept together: main.py reads
# notes.xml from its own folder.
install(FILES
	"${NOTES_SOURCE}/main.py"
	"${NOTES_SOURCE}/notes.xml"
	DESTINATION ${CMAKE_INSTALL_DATADIR}/notes)

install(FILES notes.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

# ---------------------------------------------------------------------------
#  CPack
# ---------------------------------------------------------------------------

set(CPACK_PACKAGE_NAME "notes")
set(CPACK_PACKAGE_VENDOR "Raven tutorials")
set(CPACK_PACKAGE_CONTACT "Raven tutorials <you@example.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
	"Notes - a small text editor written with Raven (Python)")
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
	# A Python application has no shared libraries to scan: name what it needs.
	# The `raven` module itself comes from pip or the Raven checkout; a .deb
	# cannot depend on it until a distribution package exists for it.
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "python3")
endif()

include(CPack)
