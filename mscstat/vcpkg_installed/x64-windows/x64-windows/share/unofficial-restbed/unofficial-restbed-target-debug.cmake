#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "unofficial::restbed::restbed-shared" for configuration "Debug"
set_property(TARGET unofficial::restbed::restbed-shared APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(unofficial::restbed::restbed-shared PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/restbed-shared.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/restbed-shared.dll"
  )

list(APPEND _cmake_import_check_targets unofficial::restbed::restbed-shared )
list(APPEND _cmake_import_check_files_for_unofficial::restbed::restbed-shared "${_IMPORT_PREFIX}/debug/lib/restbed-shared.lib" "${_IMPORT_PREFIX}/debug/bin/restbed-shared.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
