# CPack External generator: turns the installed tree into one AppImage.
#
# CPack has no AppImage generator, so it installs into CPACK_TEMPORARY_DIRECTORY
# (bin/, share/, ...) and runs this file. The result is an AppDir containing the
# designer and the theme editor, packed with appimagetool.
#
# shortcut: only the two tools are in the AppImage (the library and its
# headers are what the DEB and the archives are for), and the desktop entry runs
# the designer; the theme editor is reachable as usr/bin/raven_theme_editor.

set(_appdir "${CPACK_PACKAGE_DIRECTORY}/AppDir")
file(REMOVE_RECURSE "${_appdir}")
file(MAKE_DIRECTORY "${_appdir}/usr/bin")

foreach(_app raven_designer raven_theme_editor)
	set(_bin "${CPACK_TEMPORARY_DIRECTORY}/bin/${_app}")
	if(NOT EXISTS "${_bin}")
		message(FATAL_ERROR "AppImage: ${_bin} is missing (configure with RAVEN_BUILD_TOOLS=ON and build first)")
	endif()
	file(COPY "${_bin}" DESTINATION "${_appdir}/usr/bin")
endforeach()

file(GLOB _icons "${CPACK_TEMPORARY_DIRECTORY}/share/icons/hicolor/*/apps/raven.png")
if(NOT _icons)
	message(FATAL_ERROR "AppImage: the raven.png icon is missing from the install tree")
endif()
list(GET _icons 0 _icon)
file(COPY "${_icon}" DESTINATION "${_appdir}")

set(_desktop "${CPACK_TEMPORARY_DIRECTORY}/share/applications/raven_designer.desktop")
if(NOT EXISTS "${_desktop}")
	message(FATAL_ERROR "AppImage: raven_designer.desktop is missing from the install tree")
endif()
file(COPY "${_desktop}" DESTINATION "${_appdir}")

# $0 is AppRun when the runtime starts it, so the default is the designer;
# through the symlink below, the name selects the other tool.
file(WRITE "${_appdir}/AppRun" "#!/bin/sh
HERE=\"\$(dirname \"\$(readlink -f \"\$0\")\")\"
APP=\"\${0##*/}\"
case \"\$APP\" in
	raven_theme_editor) ;;
	*) APP=raven_designer ;;
esac
exec \"\$HERE/usr/bin/\$APP\" \"\$@\"
")
execute_process(COMMAND chmod +x "${_appdir}/AppRun")
execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink AppRun "${_appdir}/raven_theme_editor"
	RESULT_VARIABLE _rc)
if(NOT _rc EQUAL 0)
	message(FATAL_ERROR "AppImage: could not create the raven_theme_editor symlink")
endif()

if(NOT CPACK_APPIMAGE_TOOL OR NOT EXISTS "${CPACK_APPIMAGE_TOOL}")
	message(FATAL_ERROR "AppImage: appimagetool not found (configure with -DRAVEN_APPIMAGETOOL=/path/to/appimagetool)")
endif()

set(_out "${CPACK_PACKAGE_DIRECTORY}/${CPACK_APPIMAGE_FILE_NAME}")
execute_process(
	COMMAND "${CMAKE_COMMAND}" -E env
		"ARCH=${CPACK_APPIMAGE_ARCH}"
		"APPIMAGE_EXTRACT_AND_RUN=1"
		"${CPACK_APPIMAGE_TOOL}" --no-appstream "${_appdir}" "${_out}"
	RESULT_VARIABLE _rc)
if(NOT _rc EQUAL 0)
	message(FATAL_ERROR "AppImage: appimagetool failed (${_rc})")
endif()
message(STATUS "AppImage: ${_out}")
