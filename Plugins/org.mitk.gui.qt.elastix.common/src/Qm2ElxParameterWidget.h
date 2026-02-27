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

#include <QAbstractButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QPushButton>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include <m2ElxDefaultParameterFiles.h>
#include <m2ElxUtil.h>

#include "ui_Qm2ElxParameterWidgetControls.h"

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
  explicit Qm2ElxParameterWidget(QWidget *parent = nullptr) : QWidget(parent)
  {
    m_Controls.setupUi(this);

    // ----------------------------------------------------------------
    // Advanced parameter file editor dialog
    // ----------------------------------------------------------------
    m_ParamFileEditor = new QDialog(parent);
    m_ParamFileEditor->setWindowTitle("Elastix parameter file editor");
    m_ParamFileEditor->resize(700, 600);

    auto *dlgLayout = new QVBoxLayout(m_ParamFileEditor);
    auto *tabs = new QTabWidget(m_ParamFileEditor);

    const QFont mono("Monospace", 9);
    m_RigidText = new QTextEdit(m_ParamFileEditor);
    m_RigidText->setFont(mono);
    m_RigidText->setPlainText(QString::fromStdString(m2::Elx::Rigid()));
    tabs->addTab(m_RigidText, "Rigid");

    m_DeformableText = new QTextEdit(m_ParamFileEditor);
    m_DeformableText->setFont(mono);
    m_DeformableText->setPlainText(QString::fromStdString(m2::Elx::Deformable()));
    tabs->addTab(m_DeformableText, "Deformable");

    dlgLayout->addWidget(tabs);

    auto *btnBox = new QDialogButtonBox(
      QDialogButtonBox::RestoreDefaults | QDialogButtonBox::Close, m_ParamFileEditor);
    dlgLayout->addWidget(btnBox);

    connect(btnBox->button(QDialogButtonBox::RestoreDefaults),
            &QAbstractButton::clicked,
            this,
            [this]()
            {
              m_RigidText->setPlainText(QString::fromStdString(m2::Elx::Rigid()));
              m_DeformableText->setPlainText(QString::fromStdString(m2::Elx::Deformable()));
            });

    connect(btnBox->button(QDialogButtonBox::Close),
            &QAbstractButton::clicked,
            m_ParamFileEditor,
            &QDialog::accept);

    connect(m_Controls.btnAdvanced, &QPushButton::clicked,
            this, [this]() { m_ParamFileEditor->exec(); });

    // ----------------------------------------------------------------
    // Enable/disable BSpline order spinners based on interpolator choice
    // ----------------------------------------------------------------
    auto syncBSpline = [](QComboBox *combo, QSpinBox *spin, const QString &bsplineText)
    {
      const bool isBSpline = combo->currentText() == bsplineText;
      spin->setEnabled(isBSpline);
    };

    // Rigid – optimization interpolator
    connect(m_Controls.comboRigidInterpolator, &QComboBox::currentTextChanged,
            this, [this, syncBSpline](const QString &) {
              syncBSpline(m_Controls.comboRigidInterpolator,
                          m_Controls.spinRigidBSplineOrder,
                          "BSplineInterpolator");
            });
    // Rigid – resample interpolator
    connect(m_Controls.comboRigidResampleInterpolator, &QComboBox::currentTextChanged,
            this, [this, syncBSpline](const QString &) {
              syncBSpline(m_Controls.comboRigidResampleInterpolator,
                          m_Controls.spinRigidFinalBSplineOrder,
                          "FinalBSplineInterpolator");
            });
    // Deformable – optimization interpolator
    connect(m_Controls.comboDeformableInterpolator, &QComboBox::currentTextChanged,
            this, [this, syncBSpline](const QString &) {
              syncBSpline(m_Controls.comboDeformableInterpolator,
                          m_Controls.spinDeformableBSplineOrder,
                          "BSplineInterpolator");
            });
    // Deformable – resample interpolator
    connect(m_Controls.comboDeformableResampleInterpolator, &QComboBox::currentTextChanged,
            this, [this, syncBSpline](const QString &) {
              syncBSpline(m_Controls.comboDeformableResampleInterpolator,
                          m_Controls.spinDeformableFinalBSplineOrder,
                          "FinalBSplineInterpolator");
            });

    // Set initial enabled state to match current combo selection
    syncBSpline(m_Controls.comboRigidInterpolator,
                m_Controls.spinRigidBSplineOrder,       "BSplineInterpolator");
    syncBSpline(m_Controls.comboRigidResampleInterpolator,
                m_Controls.spinRigidFinalBSplineOrder,  "FinalBSplineInterpolator");
    syncBSpline(m_Controls.comboDeformableInterpolator,
                m_Controls.spinDeformableBSplineOrder,  "BSplineInterpolator");
    syncBSpline(m_Controls.comboDeformableResampleInterpolator,
                m_Controls.spinDeformableFinalBSplineOrder, "FinalBSplineInterpolator");
  }

  ~Qm2ElxParameterWidget() override = default;

  // ----------------------------------------------------------------
  // Public API
  // ----------------------------------------------------------------

  /**
   * Returns patched parameter file strings for all enabled stages.
   * - Index 0 = rigid   (present when the Rigid group is checked)
   * - Index 1 = deformable (present when the Deformable group is checked)
   *
   * All widget values are applied to the raw parameter file text exactly
   * as RegistrationView::OnStartRegistration() does.
   */
  std::vector<std::string> GetParameters() const
  {
    std::vector<std::string> params;

    auto rigid      = m_RigidText->toPlainText().toStdString();
    auto deformable = m_DeformableText->toPlainText().toStdString();

    // --- Rigid ---
    if (m_Controls.grpRigid->isChecked())
    {
      m2::ElxUtil::ReplaceParameter(rigid, "Transform",
        "\"" + m_Controls.comboRigidTransform->currentText().toStdString() + "\"");
      m2::ElxUtil::ReplaceParameter(rigid, "Metric",
        "\"" + m_Controls.comboRigidMetric->currentText().toStdString() + "\"");
      m2::ElxUtil::ReplaceParameter(rigid, "NumberOfHistogramBins",
        std::to_string(m_Controls.spinRigidHistBins->value()));
      m2::ElxUtil::ReplaceParameter(rigid, "NumberOfResolutions",
        std::to_string(m_Controls.spinRigidResolutions->value()));
      m2::ElxUtil::ReplaceParameter(rigid, "MaximumNumberOfIterations",
        std::to_string(m_Controls.spinRigidIterations->value()));
      m2::ElxUtil::ReplaceParameter(rigid, "NumberOfSpatialSamples",
        std::to_string(m_Controls.spinRigidSpatialSamples->value()));
      m2::ElxUtil::ReplaceParameter(rigid, "Interpolator",
        "\"" + m_Controls.comboRigidInterpolator->currentText().toStdString() + "\"");
      if (m_Controls.comboRigidInterpolator->currentText() == "BSplineInterpolator")
        m2::ElxUtil::ReplaceParameter(rigid, "BSplineInterpolationOrder",
          std::to_string(m_Controls.spinRigidBSplineOrder->value()));
      m2::ElxUtil::ReplaceParameter(rigid, "ResampleInterpolator",
        "\"" + m_Controls.comboRigidResampleInterpolator->currentText().toStdString() + "\"");
      if (m_Controls.comboRigidResampleInterpolator->currentText() == "FinalBSplineInterpolator")
        m2::ElxUtil::ReplaceParameter(rigid, "FinalBSplineInterpolationOrder",
          std::to_string(m_Controls.spinRigidFinalBSplineOrder->value()));

      // Initial alignment
      const auto alignText = m_Controls.comboInitialAlignment->currentText();
      if (alignText == "None")
      {
        m2::ElxUtil::ReplaceParameter(rigid, "AutomaticTransformInitialization", "\"false\"");
      }
      else if (alignText == "Geometrical Center")
      {
        m2::ElxUtil::ReplaceParameter(rigid, "AutomaticTransformInitialization", "\"true\"");
        m2::ElxUtil::ReplaceParameter(rigid, "AutomaticTransformInitializationMethod", "\"GeometricalCenter\"");
      }
      else if (alignText == "Center of Gravity")
      {
        m2::ElxUtil::ReplaceParameter(rigid, "AutomaticTransformInitialization", "\"true\"");
        m2::ElxUtil::ReplaceParameter(rigid, "AutomaticTransformInitializationMethod", "\"CenterOfGravity\"");
      }

      params.push_back(std::move(rigid));
    }

    // --- Deformable ---
    if (m_Controls.grpDeformable->isChecked())
    {
      m2::ElxUtil::ReplaceParameter(deformable, "Transform",
        "\"" + m_Controls.comboDeformableTransform->currentText().toStdString() + "\"");
      m2::ElxUtil::ReplaceParameter(deformable, "Metric",
        "\"" + m_Controls.comboDeformableMetric->currentText().toStdString() + "\"");
      m2::ElxUtil::ReplaceParameter(deformable, "NumberOfResolutions",
        std::to_string(m_Controls.spinDeformableResolutions->value()));
      m2::ElxUtil::ReplaceParameter(deformable, "MaximumNumberOfIterations",
        std::to_string(m_Controls.spinDeformableIterations->value()));
      m2::ElxUtil::ReplaceParameter(deformable, "FinalGridSpacingInPhysicalUnits",
        std::to_string(m_Controls.spinDeformableGridSpacing->value()));
      m2::ElxUtil::ReplaceParameter(deformable, "NumberOfHistogramBins",
        std::to_string(m_Controls.spinDeformableHistBins->value()));
      m2::ElxUtil::ReplaceParameter(deformable, "NumberOfSpatialSamples",
        std::to_string(m_Controls.spinDeformableSpatialSamples->value()));
      m2::ElxUtil::ReplaceParameter(deformable, "Interpolator",
        "\"" + m_Controls.comboDeformableInterpolator->currentText().toStdString() + "\"");
      if (m_Controls.comboDeformableInterpolator->currentText() == "BSplineInterpolator")
        m2::ElxUtil::ReplaceParameter(deformable, "BSplineInterpolationOrder",
          std::to_string(m_Controls.spinDeformableBSplineOrder->value()));
      m2::ElxUtil::ReplaceParameter(deformable, "ResampleInterpolator",
        "\"" + m_Controls.comboDeformableResampleInterpolator->currentText().toStdString() + "\"");
      if (m_Controls.comboDeformableResampleInterpolator->currentText() == "FinalBSplineInterpolator")
        m2::ElxUtil::ReplaceParameter(deformable, "FinalBSplineInterpolationOrder",
          std::to_string(m_Controls.spinDeformableFinalBSplineOrder->value()));

      params.push_back(std::move(deformable));
    }

    return params;
  }

  /** True when the deformable group is unchecked (rigid-only mode). */
  bool IsRigidOnly() const { return m_Controls.grpRigid->isChecked() && !m_Controls.grpDeformable->isChecked(); }

  /** True when the rigid registration group is enabled. */
  bool IsRigidEnabled() const { return m_Controls.grpRigid->isChecked(); }

  /** True when the deformable registration group is enabled. */
  bool IsDeformableEnabled() const { return m_Controls.grpDeformable->isChecked(); }

  /** Directly replace the underlying raw parameter file strings (e.g. when loading presets). */
  void SetRawParameters(const std::string &rigidParams, const std::string &deformableParams)
  {
    m_RigidText->setPlainText(QString::fromStdString(rigidParams));
    m_DeformableText->setPlainText(QString::fromStdString(deformableParams));
  }

private:
  Ui_Qm2ElxParameterWidgetControls m_Controls;

  // Advanced editor dialog (created at runtime, not in the UI file)
  QDialog   *m_ParamFileEditor = nullptr;
  QTextEdit *m_RigidText       = nullptr;
  QTextEdit *m_DeformableText  = nullptr;
};
