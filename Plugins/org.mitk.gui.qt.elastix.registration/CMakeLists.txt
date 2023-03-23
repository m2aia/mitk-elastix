project(org_mitk_gui_qt_elastix_registration)

mitk_create_plugin(
  EXPORT_DIRECTIVE ELASTIX_REGISTRATION_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgetsExt MitkElastix
)
