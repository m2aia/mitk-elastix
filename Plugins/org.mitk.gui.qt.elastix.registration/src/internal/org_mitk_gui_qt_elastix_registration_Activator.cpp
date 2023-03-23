/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/


#include "org_mitk_gui_qt_elastix_registration_Activator.h"
#include "RegistrationView.h"
#include <berryPlatform.h>

namespace mitk
{
  void org_mitk_gui_qt_elastix_registration_Activator::start(ctkPluginContext *context)
  {
    BERRY_REGISTER_EXTENSION_CLASS(RegistrationView, context)

    berry::IPreferencesService *preferencesService = berry::Platform::GetPreferencesService();
    if (preferencesService != nullptr)
    {
      berry::IPreferences::Pointer systemPreferences = preferencesService->GetSystemPreferences();

      if (systemPreferences.IsNotNull()){
        auto preferences = systemPreferences->Node("/org.mitk.gui.qt.ext.externalprograms");
        if(preferences->Get("elastix","").isEmpty())
          preferences->Put("elastix","");
        preferences->Put("elastix_check","--version=elastix");

        if(preferences->Get("transformix","").isEmpty())
          preferences->Put("transformix","");
        preferences->Put("transformix_check","--version=transformix");
      }
    }
  }

  void org_mitk_gui_qt_elastix_registration_Activator::stop(ctkPluginContext *context) { Q_UNUSED(context) }
}
