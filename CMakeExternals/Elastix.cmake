#-----------------------------------------------------------------------------
# Elastix
#-----------------------------------------------------------------------------

set(proj Elastix)

if(MITK_USE_Elastix)

  set(Elastix_VERSION "5.2.0")

  # Prefer a system-installed Elastix; fall back to downloading a pre-built binary.
  find_program(Elastix_SYSTEM_EXECUTABLE
    NAMES elastix
    PATH_SUFFIXES bin
    DOC "Path to system-installed elastix executable"
  )

  if(Elastix_SYSTEM_EXECUTABLE)
    get_filename_component(Elastix_DIR  "${Elastix_SYSTEM_EXECUTABLE}" DIRECTORY)
    get_filename_component(_elx_root    "${Elastix_DIR}" DIRECTORY)
    set(Elastix_LIBRARY "${_elx_root}/lib")
    message(STATUS "Elastix: using system installation at ${Elastix_DIR}")
    mitkMacroEmptyExternalProject(${proj})
  else()
    message(STATUS "Elastix: no system installation found — downloading version ${Elastix_VERSION}")

    # APPLE must be checked before the generic UNIX/else branch
    if(WIN32)
      set(Elastix_URL "https://github.com/SuperElastix/elastix/releases/download/${Elastix_VERSION}/elastix-${Elastix_VERSION}-win64.zip")
      set(Elastix_MD5 "ad398c3768b82878b8c1fbfadd606027")
    elseif(APPLE)
      set(Elastix_URL "https://github.com/SuperElastix/elastix/releases/download/${Elastix_VERSION}/elastix-${Elastix_VERSION}-mac.tar.gz")
      set(Elastix_MD5 "9902cf6c3d2b223b58a96bb80ea47046")
    else() # Linux / other Unix
      set(Elastix_URL "https://github.com/SuperElastix/elastix/releases/download/${Elastix_VERSION}/elastix-${Elastix_VERSION}-linux.zip")
      set(Elastix_MD5 "b7dcce312bc6424f69f6e1187bf7208e")
    endif()

    set(noop_cmd ${CMAKE_COMMAND} -E echo "skipping")

    if(WIN32)
      # Windows archive: executables at the root level — copy everything into bin/
      set(install_cmd
        ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR> <INSTALL_DIR>/bin
      )
    else()
      # Linux/macOS archives: separate bin/ and lib/ subdirectories
      # chmod is needed because zip/tar extraction via ExternalProject does not
      # guarantee the execute bit is preserved.
      set(install_cmd
        ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/bin <INSTALL_DIR>/bin
        && ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/lib <INSTALL_DIR>/lib
        && chmod +x <INSTALL_DIR>/bin/elastix <INSTALL_DIR>/bin/transformix
      )
    endif()

    ExternalProject_Add(${proj}
      URL               ${Elastix_URL}
      URL_MD5           ${Elastix_MD5}
      CONFIGURE_COMMAND ${noop_cmd}
      BUILD_COMMAND     ${noop_cmd}
      INSTALL_COMMAND   ${install_cmd}
    )

    set(Elastix_DIR     "${ep_prefix}/bin")
    set(Elastix_LIBRARY "${ep_prefix}/lib")
  endif()

  # Generate the compile-time config header used by C++ modules
  configure_file(
    ${MITK_CMAKE_EXTERNALS_EXTENSION_DIR}/m2ElxConfig.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/MITK-build/m2ElxConfig.h
    @ONLY
  )

else()
  mitkMacroEmptyExternalProject(${proj})
endif()

