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
#include <mitkIPreferencesService.h>
#include <mitkIPreferences.h>
#include <mitkCoreServices.h>

#include <usModuleInitialization.h>

US_INITIALIZE_MODULE

namespace mitk
{
  void org_mitk_gui_qt_elastix_registration_Activator::start(ctkPluginContext *context)
  {
    BERRY_REGISTER_EXTENSION_CLASS(RegistrationView, context)

    auto preferencesService = mitk::CoreServices::GetPreferencesService();
    if (preferencesService != nullptr)
    {
      auto systemPreferences = preferencesService->GetSystemPreferences();

      if (systemPreferences){
        auto preferences = systemPreferences->Node("/org.mitk.gui.qt.ext.externalprograms");
        if(preferences->Get("elastix","").empty())
          preferences->Put("elastix","");
        preferences->Put("elastix_check","--version=elastix");

        if(preferences->Get("transformix","").empty())
          preferences->Put("transformix","");
        preferences->Put("transformix_check","--version=transformix");
      }
    }
  }

  void org_mitk_gui_qt_elastix_registration_Activator::stop(ctkPluginContext *context) { Q_UNUSED(context) }
}
