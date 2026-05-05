/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#pragma once

#include <org_mitk_gui_qt_elastix_common_Export.h>

#include <QDialog>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>

#include "ui_Qm2ElxParameterWidgetControls.h"

#include <string>
#include <vector>

/**
 * \brief Reusable widget providing the full elastix registration parameter
 *        controls (identical to the Elastix Registration view).
 *
 * Contains:
 *   - Registration Setup group  (initial alignment)
 *   - Rigid Registration group  (checkable; transform, metric, iterations, …)
 *   - Deformable Registration group (checkable; metric, iterations, grid spacing, …)
 *   - "Advanced…" button → raw parameter file editor dialog
 *
 * Usage:
 * \code
 *   auto* w = new Qm2ElxParameterWidget(parent);
 *   layout->addWidget(w);
 *   auto params = w->GetParameters();   // {patchedRigid [, patchedDeformable]}
 * \endcode
 */
class ELASTIX_COMMON_EXPORT Qm2ElxParameterWidget : public QWidget
{
  Q_OBJECT

public:
  explicit Qm2ElxParameterWidget(QWidget *parent = nullptr);
  ~Qm2ElxParameterWidget() override = default;

  /**
   * Returns patched parameter file strings for all enabled stages.
   * - Index 0 = rigid       (present when the Rigid group is checked)
   * - Index 1 = deformable  (present when the Deformable group is checked)
   */
  std::vector<std::string> GetParameters() const;

  /** True when the deformable group is unchecked (rigid-only mode). */
  bool IsRigidOnly() const;

  /** True when the rigid registration group is enabled. */
  bool IsRigidEnabled() const;

  /** True when the deformable registration group is enabled. */
  bool IsDeformableEnabled() const;

  /** Directly replace the underlying raw parameter file strings (e.g. when loading presets). */
  void SetRawParameters(const std::string &rigidParams, const std::string &deformableParams);

private:
  mutable Ui_Qm2ElxParameterWidgetControls m_Controls;

  QDialog   *m_ParamFileEditor = nullptr;
  QTextEdit *m_RigidText       = nullptr;
  QTextEdit *m_DeformableText  = nullptr;
};

