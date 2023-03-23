mitk_create_module(
  DEPENDS PUBLIC MitkCore MitkMultilabel
  PACKAGE_DEPENDS PUBLIC Poco
)


if(BUILD_TESTING)
add_subdirectory(testing)
endif()