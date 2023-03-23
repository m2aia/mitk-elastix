set(SRC_CPP_FILES

)

set(INTERNAL_CPP_FILES
  org_mitk_gui_qt_elastix_registration_Activator.cpp
  RegistrationView.cpp
  RegistrationDataWidget.cpp
)

set(UI_FILES
  src/internal/RegistrationViewControls.ui
  src/internal/ParameterFileEditorDialog.ui
  src/internal/RegistrationDataWidgetControls.ui
)

set(MOC_H_FILES
  src/internal/org_mitk_gui_qt_elastix_registration_Activator.h
  src/internal/RegistrationView.h
  src/internal/RegistrationDataWidget.h
)

# list of resource files which can be used by the plug-in
# system without loading the plug-ins shared library,
# for example the icon used in the menu and tabs for the
# plug-in views in the workbench
set(CACHED_RESOURCE_FILES
  resources/icon.svg
  plugin.xml
)

# list of Qt .qrc files which contain additional resources
# specific to this plugin
set(QRC_FILES

)

set(CPP_FILES )

foreach(file ${SRC_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/${file})
endforeach(file ${SRC_CPP_FILES})

foreach(file ${INTERNAL_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/internal/${file})
endforeach(file ${INTERNAL_CPP_FILES})
