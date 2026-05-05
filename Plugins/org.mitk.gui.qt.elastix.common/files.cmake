set(SRC_CPP_FILES
  Qm2ElxParameterWidget.cpp
)

set(INTERNAL_CPP_FILES
  org_mitk_gui_qt_elastix_common_Activator.cpp
)

set(UI_FILES
  src/Qm2ElxParameterWidgetControls.ui
)

set(H_FILES
)

set(MOC_H_FILES
  src/internal/org_mitk_gui_qt_elastix_common_Activator.h
  src/Qm2ElxParameterWidget.h
)

set(CACHED_RESOURCE_FILES
  plugin.xml
)

set(QRC_FILES
)

set(CPP_FILES )

foreach(file ${SRC_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/${file})
endforeach(file ${SRC_CPP_FILES})

foreach(file ${INTERNAL_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/internal/${file})
endforeach(file ${INTERNAL_CPP_FILES})
