# Notes - packaging and installation.
#
# This project does not contain the application: it takes step 08, builds it,
# installs it where a system expects it, and packs it with CPack.
#
#   cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr
#   cmake --build build
#   cd build && cpack -G DEB
#   sudo apt install ./notes_1.0.0_amd64.deb
#
# See README.md for what each line does and how to check the result.

cmake_minimum_required(VERSION 3.20)
project(notes VERSION 1.0.0 LANGUAGES CXX
	DESCRIPTION "Notes - a small text editor written with Raven")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# GNUInstallDirs gives the conventional folders: bin, share, share/doc...
# They follow CMAKE_INSTALL_PREFIX, which the packaging run sets to /usr.
include(GNUInstallDirs)

find_package(raven REQUIRED)

# ---------------------------------------------------------------------------
#  Build
# ---------------------------------------------------------------------------

set(NOTES_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../08-capstone-notes")

add_executable(notes "${NOTES_SOURCE}/main.cpp")
target_link_libraries(notes PRIVATE raven::raven)
# Where the installed program finds notes.xml at run time. This is the whole
# point of the step: a build-tree path (".") cannot work once installed.
target_compile_definitions(notes PRIVATE
	RAVEN_TUTORIAL_DIR="${CMAKE_INSTALL_FULL_DATADIR}/notes")

# ---------------------------------------------------------------------------
#  Install: what "make install" and the packages contain
# ---------------------------------------------------------------------------

install(TARGETS notes RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES "${NOTES_SOURCE}/notes.xml"
	DESTINATION ${CMAKE_INSTALL_DATADIR}/notes)
install(FILES notes.desktop
	DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

# ---------------------------------------------------------------------------
#  CPack: the same install tree as an archive, and as a .deb
# ---------------------------------------------------------------------------

set(CPACK_PACKAGE_NAME "notes")
set(CPACK_PACKAGE_VENDOR "Raven tutorials")
set(CPACK_PACKAGE_CONTACT "Raven tutorials <you@example.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
	"Notes - a small text editor written with Raven")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "notes")

# An archive works everywhere; a .deb is added on Linux.
set(CPACK_GENERATOR "TGZ")
if(UNIX AND NOT APPLE)
	set(CPACK_GENERATOR "TGZ;DEB")

	set(CPACK_DEBIAN_PACKAGE_NAME "notes")
	set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
	set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Raven tutorials <you@example.com>")
	set(CPACK_DEBIAN_PACKAGE_SECTION "editors")
	set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
	# Lets CPack read the libraries the program links and write libc6, libx11-6...
	set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()

include(CPack)
