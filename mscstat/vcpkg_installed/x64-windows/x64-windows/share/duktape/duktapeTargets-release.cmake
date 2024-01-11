#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "duktape::duktape" for configuration "Release"
set_property(TARGET duktape::duktape APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(duktape::duktape PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/duktape.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/duktape.dll"
  )

list(APPEND _cmake_import_check_targets duktape::duktape )
list(APPEND _cmake_import_check_files_for_duktape::duktape "${_IMPORT_PREFIX}/lib/duktape.lib" "${_IMPORT_PREFIX}/bin/duktape.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
