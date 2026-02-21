project(org_mitk_gui_qt_elastix_common)

mitk_create_plugin(
  EXPORT_DIRECTIVE ELASTIX_COMMON_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgets MitkElastix
)
