# FindElastix.cmake
# Locate the elastix and transformix executables.
#
# Result variables:
#   Elastix_FOUND            - TRUE if both executables were found
#   Elastix_DIR              - Directory containing the elastix executable
#   Elastix_LIBRARY          - Directory containing elastix runtime libraries
#   Elastix_EXECUTABLE       - Full path to the elastix executable
#   Transformix_EXECUTABLE   - Full path to the transformix executable

# Search system PATH first; also look inside the superbuild external-project prefix.
find_program(Elastix_EXECUTABLE
  NAMES elastix
  HINTS "${MITK_EXTERNAL_PROJECT_PREFIX}/bin"
  PATH_SUFFIXES bin
  DOC "Path to the elastix executable"
)

find_program(Transformix_EXECUTABLE
  NAMES transformix
  HINTS "${MITK_EXTERNAL_PROJECT_PREFIX}/bin"
  PATH_SUFFIXES bin
  DOC "Path to the transformix executable"
)

if(Elastix_EXECUTABLE)
  get_filename_component(Elastix_DIR  "${Elastix_EXECUTABLE}" DIRECTORY)
  get_filename_component(_elx_root    "${Elastix_DIR}" DIRECTORY)
  set(Elastix_LIBRARY "${_elx_root}/lib")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Elastix
  REQUIRED_VARS Elastix_EXECUTABLE Transformix_EXECUTABLE
)

mark_as_advanced(Elastix_EXECUTABLE Transformix_EXECUTABLE)

