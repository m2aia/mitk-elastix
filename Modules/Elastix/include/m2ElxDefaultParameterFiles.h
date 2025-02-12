/*===================================================================
MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#pragma once
#include <string>
#include <MitkElastixExports.h>

namespace m2
{
  namespace Elx
  {
    MITKELASTIX_EXPORT std::string Rigid();
    MITKELASTIX_EXPORT std::string Deformable();
  } // namespace Elx

} // namespace m2