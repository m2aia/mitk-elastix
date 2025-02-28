#-----------------------------------------------------------------------------
# Elastix
#-----------------------------------------------------------------------------

if(MITK_USE_Elastix)

  message(STATUS "Elastix: enabled")
  set(proj Elastix)

  
  set(dummy_cmd cd .)
  set(patch_cmd ${dummy_cmd})
  set(configure_cmd ${dummy_cmd})
  set(install_cmd "")

  
  set(Elastix_VERSION 5.2.0)
  set(Elastix_URL "")
  set(Elastix_MD5 "")

  if(WIN32)
    set(install_cmd
      ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR> <INSTALL_DIR>/lib/
      && ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR> <INSTALL_DIR>/bin/
    )

    set(build_cmd
      ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR> .
    )
  else()
    set(install_cmd
      ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/lib/ <INSTALL_DIR>/lib/
      && ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/bin/ <INSTALL_DIR>/bin/
    )

    set(build_cmd
      ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR> .
    )

  endif()

  if(WIN32)
    set(Elastix_URL "https://github.com/SuperElastix/elastix/releases/download/${Elastix_VERSION}/elastix-${Elastix_VERSION}-win64.zip")
    set(Elastix_MD5 ad398c3768b82878b8c1fbfadd606027)
  elseif(UNIX)
    set(Elastix_URL "https://github.com/SuperElastix/elastix/releases/download/${Elastix_VERSION}/elastix-${Elastix_VERSION}-linux.zip")
    set(Elastix_MD5 b7dcce312bc6424f69f6e1187bf7208e)
  elseif(APPLE)
    set(Elastix_URL "https://github.com/SuperElastix/elastix/releases/download/${Elastix_VERSION}/elastix-${Elastix_VERSION}-mac.tar.gz")
    set(Elastix_MD5 9902cf6c3d2b223b58a96bb80ea47046)
  endif()

      
  ExternalProject_Add(${proj}
    URL ${Elastix_URL}
    URL_MD5 ${Elastix_MD5}
    PATCH_COMMAND ${patch_cmd}
    CONFIGURE_COMMAND ${configure_cmd}
    BUILD_COMMAND ${build_cmd}
    INSTALL_COMMAND ${install_cmd}
  )

  # ExternalProject_Add_Step(${proj} post_install
  #   COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/Elastix-post_install-WIN32.cmake
  #   DEPENDEES install
  #   WORKING_DIRECTORY <INSTALL_DIR>/bin
  # )
  
  ExternalProject_Add_Step(${proj} install_manifest
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/Elastix-install_manifest.cmake
      DEPENDEES install
      WORKING_DIRECTORY ${ep_prefix}
    )
    
  set(Elastix_DIR ${ep_prefix}/bin)
  set(Elastix_LIBRARY ${ep_prefix}/lib)

  # Create an configuration file for accessing the correct binary path of elastix
  configure_file(${MITK_CMAKE_EXTERNALS_EXTENSION_DIR}/m2ElxConfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/MITK-build/m2ElxConfig.h @ONLY)

else()
  mitkMacroEmptyExternalProject(${proj})
endif()

  
