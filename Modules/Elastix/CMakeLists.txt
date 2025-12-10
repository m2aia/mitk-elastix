set(boost_depends "Boost|filesystem")

mitk_create_module(
  DEPENDS PUBLIC MitkCore MitkMultilabel
  PACKAGE_DEPENDS PUBLIC Poco ${boost_depends}
)


if(BUILD_TESTING)
add_subdirectory(testing)
endif()