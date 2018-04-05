find_package(PkgConfig QUIET)
pkg_check_modules(PC_LibYAML QUIET yaml-0.1)

set(LibYAML_VERSION ${PC_LibYAML_VERSION})
set(LibYAML_INCLUDE_DIRS ${PC_LibYAML_INCLUDEDIR})

find_library(LibYAML_LIBRARY
  NAMES ${PC_LibYAML_LIBRARIES}
  HINTS ${PC_LibYAML_INCLUDEDIR} ${PC_LibYAML_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibYAML
  REQUIRED_VARS LibYAML_LIBRARY LibYAML_INCLUDE_DIRS
  VERSION_VAR   LibYAML_VERSION)

mark_as_advanced(LibYAML_LIBRARY LibYAML_INCLUDE_DIRS)

if(LibYAML_LIBRARY)
  if(NOT TARGET libyaml::yaml)
    add_library(libyaml::yaml UNKNOWN IMPORTED)
    set_target_properties(libyaml::yaml PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${LibYAML_INCLUDE_DIRS}")
    set_target_properties(libyaml::yaml PROPERTIES
      IMPORTED_LOCATION "${LibYAML_LIBRARY}")
  endif()
endif()
