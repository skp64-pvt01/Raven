# Included by CPack once per generator before packaging.
#
# Debian packages install under /usr (the FHS); the archives stay a
# relocatable install prefix (bin/, lib/, share/ at the root).
if(CPACK_GENERATOR STREQUAL "DEB")
	set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
	# Debian: the synopsis must not repeat the package name ("raven")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Lightweight multithreaded GUI library in C")
else()
	set(CPACK_PACKAGING_INSTALL_PREFIX "")
endif()
