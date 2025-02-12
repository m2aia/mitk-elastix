/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include <queue>

// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>
#include <berryPlatform.h>
#include <berryPlatformUI.h>

// Qmitk
#include "QmitkMultiNodeSelectionWidget.h"
#include "QmitkSingleNodeSelectionWidget.h"
#include "RegistrationView.h"
// Qt

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QProcess>
#include <QVBoxLayout>
#include <QtConcurrent>
#include <qfiledialog.h>

// mitk
#include <mitkIOUtil.h>
#include <mitkImage.h>
#include <mitkImageAccessByItk.h>
#include <mitkImageCast.h>
#include <mitkImageReadAccessor.h>
#include <mitkImageWriteAccessor.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateFunction.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>
#include <mitkPointSet.h>
#include <mitkProgressBar.h>
#include <mitkCoreServices.h>
#include <mitkIPreferences.h>
#include <mitkIPreferencesService.h>

// itk
#include <itkInvertIntensityImageFilter.h>
#include <itksys/SystemTools.hxx>

// m2
#include "RegistrationDataWidget.h"
#include <m2ElxDefaultParameterFiles.h>
#include <m2ElxRegistrationHelper.h>
#include <m2ElxUtil.h>
#include <ui_ParameterFileEditorDialog.h>
#include <ui_ComponentSelectionDialog.h>
#include <m2ElxDefaultParameterFiles.h>

const std::string RegistrationView::VIEW_ID = "org.mitk.views.elastix.registration";

void RegistrationView::SetFocus() {}

void RegistrationView::CreateQtPartControl(QWidget *parent)
{
  // create GUI widgets from the Qt Designer's .ui file
  m_Controls.setupUi(parent);
  m_Parent = parent;
  m_Controls.tabWidget->setCornerWidget(m_Controls.btnAddModality);

  m_FixedEntity = new RegistrationDataWidget(parent, this->GetDataStorage());
  m_FixedEntity->EnableButtons(false);
  m_FixedEntity->m_Controls.imageSelection->SetAutoSelectNewNodes(true);
  m_Controls.tabWidget->addTab(m_FixedEntity, "Fixed");


  connect(m_Controls.btnStartRecon, SIGNAL(clicked()), this, SLOT(OnPostProcessReconstruction()));

  // connect(
  //   m_Controls.registrationStrategy, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](auto currentIndex)
  //   {
  //     m_ParameterFileEditorControls.rigidText->setText(m_ParameterFiles[currentIndex][0].c_str());
  //     m_ParameterFileEditorControls.deformableText->setText(m_ParameterFiles[currentIndex][1].c_str());

  //     switch (currentIndex)
  //     {
  //       case 0:
  //         m_Controls.label->setText("SliceImageData [NxM] or [NxMx1], MultiModal, MultiMetric, Rigid + Deformable");
  //         break;
  //       case 1:
  //         m_Controls.label->setText("VolumeImageData [NxMxD], MultiModal, MultiMetric, Rigid+Deformable");
  //         break;
  //     }
  //   });

  // m_Controls.registrationStrategy->setCurrentIndex(1);

  connect(m_Controls.btnStartRegistration, SIGNAL(clicked()), this, SLOT(OnStartRegistration()));

  connect(m_Controls.btnAddModality, SIGNAL(clicked()), this, SLOT(OnAddRegistrationData()));
  connect(m_Controls.btnSelectChannels, SIGNAL(clicked()), this, SLOT(OnSelectChannels()));

  connect(m_Controls.btnOpenPontSetInteractionView, &QPushButton::clicked, this, []() {
    try
    {
      if (auto platform = berry::PlatformUI::GetWorkbench())
        if (auto workbench = platform->GetActiveWorkbenchWindow())
          if (auto page = workbench->GetActivePage())
            if (page.IsNotNull())
              page->ShowView("org.mitk.views.pointsetinteraction", "", 1);
    }
    catch (berry::PartInitException &e)
    {
      BERRY_ERROR << "Error: " << e.what() << std::endl;
    }
  });


  m_ParameterFiles = {m2::Elx::Rigid(), m2::Elx::Deformable()};

  m_ParameterFileEditor = new QDialog(parent);
  m_ParameterFileEditorControls.setupUi(m_ParameterFileEditor);

  connect(m_ParameterFileEditorControls.buttonBox->button(QDialogButtonBox::StandardButton::RestoreDefaults),
          &QAbstractButton::clicked,
          this,
          [this]() {
            m_ParameterFileEditorControls.rigidText->setText(m2::Elx::Rigid().c_str());
            m_ParameterFileEditorControls.deformableText->setText(m2::Elx::Deformable().c_str());
          });

  connect(m_ParameterFileEditorControls.buttonBox->button(QDialogButtonBox::StandardButton::Close),
          &QAbstractButton::clicked,
          this,
          [this]() {
            m_ParameterFiles = {m_ParameterFileEditorControls.rigidText->toPlainText().toStdString(),
                                m_ParameterFileEditorControls.deformableText->toPlainText().toStdString()};
          });

  connect(m_Controls.btnEditParameterFiles, &QPushButton::clicked, this, [this]() {
    m_ParameterFileEditor->exec();
    m_ParameterFiles = {m_ParameterFileEditorControls.rigidText->toPlainText().toStdString(),
                        m_ParameterFileEditorControls.deformableText->toPlainText().toStdString()};
  });

}

std::vector<std::string> splitString(const std::string& str, char delimiter = ';') {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    for (std::string token; std::getline(ss, token, delimiter); )
        tokens.push_back(token);
    return tokens;
}

void RegistrationView::OnSelectChannels(){

  auto dia = new QDialog(m_Parent);

  Ui::ComponentSelectionDialog ui;
  ui.setupUi(dia);
  
  auto fixedEntity = dynamic_cast<RegistrationDataWidget *>(m_Controls.tabWidget->widget(0));
  if (fixedEntity->HasImage())
  {
    auto image = fixedEntity->GetImage();
    auto channelDetailsProperty = image->GetProperty("channel.details");
    if (channelDetailsProperty)
    {
      auto stringOfNames = channelDetailsProperty->GetValueAsString();
      auto listOfNames = splitString(stringOfNames);
      unsigned int itemIndex = 0;
      for (auto name : listOfNames)
      {
        auto item = new QListWidgetItem();
        ui.listWidget->addItem(item);
        auto *itemWidget = new QCheckBox(name.c_str());
        itemWidget->setAutoExclusive(true);
        ui.listWidget->setItemWidget(item, itemWidget);
        connect(itemWidget, &QCheckBox::toggled, this, [&, itemIndex](bool toggled)
                {
            if(toggled){
              fixedEntity->GetImageNode()->SetIntProperty("Image.Displayed Component", itemIndex);
                this->GetRenderWindowPart()->RequestUpdate();
            } });
        ++itemIndex;
      }
    }
  }

  if(auto movingEntity = dynamic_cast<RegistrationDataWidget *>(m_Controls.tabWidget->widget(1))){
    if (movingEntity->HasImage())
    {
      auto image = movingEntity->GetImage();
      auto channelDetailsProperty = image->GetProperty("channel.details");
      if (channelDetailsProperty)
      {
        auto stringOfNames = channelDetailsProperty->GetValueAsString();
        auto listOfNames = splitString(stringOfNames);
        unsigned int itemIndex = 0;
        for (auto name : listOfNames)
        {
          auto item = new QListWidgetItem();
          ui.movingImageListWidget->addItem(item);
          auto *itemWidget = new QCheckBox(name.c_str());
          ui.movingImageListWidget->setItemWidget(item, itemWidget);
          item->setData(Qt::UserRole, QVariant(itemIndex));
          // connect(itemWidget, &QCheckBox::clicked, this, [&, itemIndex](bool toggled)
          //         {
          //     if(toggled){
          //       movingEntity->GetImageNode()->SetIntProperty("Image.Displayed Component", itemIndex);
          //         this->GetRenderWindowPart()->RequestUpdate();
          //     } });
          ++itemIndex;
        }
      }
    }
    connect(ui.movingImageListWidget, &QListWidget::itemDoubleClicked, this, [&](QListWidgetItem * item)
                  {
              if(item){
                movingEntity->GetImageNode()->SetIntProperty("Image.Displayed Component", item->data(Qt::UserRole).toInt());
                  this->GetRenderWindowPart()->RequestUpdate();
              }
          });
  }

  
  if(dia->exec()){
    
  }
  delete dia;
}

void RegistrationView::OnAddRegistrationData()
{
  auto widget = new RegistrationDataWidget(m_Parent, GetDataStorage());
  auto tabWidget = m_Controls.tabWidget;
  auto ignoreCheck = [tabWidget](const mitk::DataNode *node) {
    std::vector<mitk::DataNode::Pointer> ignoreNodes;
    for (int i = 0; i < tabWidget->count(); i++)
    {
      auto data = dynamic_cast<RegistrationDataWidget *>(tabWidget->widget(i));
      ignoreNodes.push_back(data->GetImageNode());
    }

    bool result = true;
    for (const auto &ignoreNode : ignoreNodes)
    {
      if (node == ignoreNode)
      {
        result = false;
        break;
      }
    }
    return result;
  };

  auto predicate = widget->m_Controls.imageSelection->GetNodePredicate();
  auto ignoreNode = mitk::NodePredicateFunction::New(ignoreCheck);
  auto newPredicate = mitk::NodePredicateAnd::New(predicate, ignoreNode);
  widget->m_Controls.imageSelection->SetNodePredicate(newPredicate);
  widget->m_Controls.imageSelection->SetAutoSelectNewNodes(true);
  widget->m_Controls.imageSelection->SetAutoSelectNewNodes(false);
  widget->m_Controls.imageSelection->SetNodePredicate(predicate);

  auto name = (std::string("M") + std::to_string(m_Controls.tabWidget->count())).c_str();
  auto newIndex = m_Controls.tabWidget->addTab(widget, name);

  connect(widget->m_Controls.btnRemove, &QAbstractButton::clicked, this, [widget, this]() {
    OnRemoveRegistrationData(widget);
  });

  // bing new tab to front
  m_Controls.tabWidget->setCurrentIndex(newIndex);
}

void RegistrationView::OnRemoveRegistrationData(QWidget *registrationDataWidget)
{
  auto index = m_Controls.tabWidget->indexOf(registrationDataWidget);
  m_Controls.tabWidget->removeTab(index);
}

void RegistrationView::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*part*/,
                                          const QList<mitk::DataNode::Pointer> &nodes)
{
  if (m_Controls.chkBxReinitOnSelectionChanged->isChecked())
  {
    if (!nodes.empty())
    {
      auto set = mitk::DataStorage::SetOfObjects::New();
      set->push_back(nodes.back());
      auto data = nodes.back();
      if (auto image = dynamic_cast<mitk::Image *>(data->GetData()))
      {
        mitk::RenderingManager::GetInstance()->InitializeViews(
          image->GetTimeGeometry(), mitk::RenderingManager::REQUEST_UPDATE_ALL, true);
        // auto boundingGeometry = GetDataStorage()->ComputeBoundingGeometry3D(set, "visible", nullptr);

        // mitk::RenderingManager::GetInstance()->InitializeViews(boundingGeometry);
      }
    }
  }
}

QString RegistrationView::GetElastixPathFromPreferences() const
{
  auto preferences =
    mitk::CoreServices::GetPreferencesService()->GetSystemPreferences()->Node("/org.mitk.gui.qt.m2aia.preferences");

  return (preferences != nullptr ? preferences->Get("elastix", "") : "").c_str();
}

void RegistrationView::OnPostProcessReconstruction()
{
  // create volume node

  {
    MITK_INFO << "***** Initialize new volume *****";
    auto image = m_FixedEntity->GetImage();
    unsigned int dims[3] = {0, 0, 0};
    auto newVolume = mitk::Image::New();

    dims[0] = image->GetDimensions()[0];
    dims[1] = image->GetDimensions()[1];
    dims[2] = m_Controls.tabWidget->count();

    newVolume->Initialize(image->GetPixelType(), 3, dims);
    auto spacing = image->GetGeometry()->GetSpacing();
    spacing[2] = m_Controls.spinBoxZSpacing->value() * 10e-4;
    newVolume->SetSpacing(spacing);

    std::vector<mitk::Image::Pointer> orderedData(dims[2]);

    for (int i = 0; i < m_Controls.tabWidget->count(); i++)
    {
      auto data = dynamic_cast<RegistrationDataWidget *>(m_Controls.tabWidget->widget(i));

      const auto &transformations = data->GetRegistrationData()->m_Transformations;
      if (transformations.size())
      {
        m2::ElxRegistrationHelper helper;
        helper.SetTransformations(transformations);
        orderedData[i] = helper.WarpImage(data->GetImage());
      }
      else
      {
        orderedData[i] = data->GetImage();
      }
    }

    MITK_INFO << "***** Copy data to volume *****";
    AccessByItk(newVolume, ([&](auto itkImage) {
                  for (unsigned int i = 0; i < orderedData.size(); ++i)
                  {
                    AccessByItk(orderedData[i], ([&](auto itkInput) {
                                  auto td = itkImage->GetBufferPointer();
                                  auto sd = itkInput->GetBufferPointer(); // sourcedata
                                  std::copy(sd, sd + (dims[0] * dims[1]), td + (i * (dims[0] * dims[1])));
                                }));
                  }
                }));

    {
      MITK_INFO << "***** Add volume to data storage *****";

      auto r = mitk::DataNode::New();
      r->SetData(newVolume);
      r->SetName("Reconstruction");
      GetDataStorage()->Add(r);
    }
  }
}

void RegistrationView::Registration(RegistrationDataWidget *fixed, RegistrationDataWidget *moving)
{
  mitk::ProgressBar::GetInstance()->AddStepsToDo(6);
  mitk::ProgressBar::GetInstance()->SetPercentageVisible(false);

  const auto invert = [](mitk::Image::Pointer image) {
    mitk::Image::Pointer rImage;
    AccessByItk(image, ([&](auto imageItk) mutable {
                  using ItkImageType = typename std::remove_pointer<decltype(imageItk)>::type;
                  auto filter = itk::InvertIntensityImageFilter<ItkImageType>::New();
                  filter->SetInput(imageItk);
                  filter->Update();
                  typename ItkImageType::Pointer fout = filter->GetOutput();
                  mitk::CastToMitkImage(fout, rImage);
                }));
    return rImage;
  };

  // check if
  auto elastix = m2::ElxUtil::Executable("elastix");
  if (elastix.empty())
  {
    elastix = GetElastixPathFromPreferences().toStdString();
    if (elastix.empty())
    {
      QMessageBox::information(nullptr, "Error", "No elastix executable specified in the MITK properties!");
      return;
    }
  }

  mitk::Image::Pointer fixedImage, movingImage, fixedImageMask, movingImageMask;
  mitk::DataNode::Pointer fixedImageNode, movingImageNode, fixedImageMaskNode, movingImageMaskNode;
  mitk::PointSet::Pointer fixedPointSet, movingPointSet;

  std::list<std::string> queue;

  auto statusCallback = [=](std::string &&v) mutable {
    if (queue.size() > 15)
      queue.pop_front();
    queue.push_back(v);
    QString s;
    for (auto line : queue)
      s = s + line.c_str() + "\n";

    m_Controls.labelStatus->setText(s);
  };

  if (fixed->HasImage())
  {
    fixedImageNode = fixed->GetImageNode();
    fixedImage = fixed->GetImage();
    if (m_Controls.chkBxInvertIntensities->isChecked())
      fixedImage = invert(fixedImage);

    if (fixed->HasTransformations())
    {
      MITK_INFO << "***** Warp Fixed Image *****";
      m2::ElxRegistrationHelper warpingHelper;
      warpingHelper.SetTransformations(fixed->GetTransformations());
      fixedImage = warpingHelper.WarpImage(fixedImage);
    }
    mitk::ProgressBar::GetInstance()->Progress(1);
  }
  else
  {
    mitkThrow() << "No Fixed image data";
  }

  if (moving->HasImage())
  {
    movingImageNode = moving->GetImageNode();
    movingImage = moving->GetImage();
    if (m_Controls.chkBxInvertIntensities->isChecked())
      movingImage = invert(movingImage);
    mitk::ProgressBar::GetInstance()->Progress(1);
  }
  else
  {
    mitkThrow() << "No Moving image data";
  }

  // Fixed image mask

  if (fixed->HasMask())
  {
    fixedImageMaskNode = fixed->GetMaskNode();
    fixedImageMask = fixed->GetMask();
    if (fixed->HasTransformations())
    {
      m2::ElxRegistrationHelper warpingHelper;
      warpingHelper.SetTransformations(fixed->GetTransformations());
      fixedImageMask = warpingHelper.WarpImage(fixedImageMask, "short", 1);
    }
  }

  // PointSets

  fixedPointSet = fixed->GetPointSet();
  movingPointSet = moving->GetPointSet();
  try
  {
    std::vector<std::string> parameterFiles = m_ParameterFiles;

    if (!m_Controls.chkBxUseDeformableRegistration->isChecked() || parameterFiles.back().empty())
    {
      parameterFiles.pop_back();
    }

    auto helper = std::make_shared<m2::ElxRegistrationHelper>();

    mitk::ProgressBar::GetInstance()->Progress(1);
    // Images
    // setup and run
    MITK_INFO << "Use Count: " << helper.use_count();
    // m_Controls.btnStartRegistration->setEnabled(false);
    MITK_INFO << "***** Start Registration *****";
    helper->SetAdditionalBinarySearchPath(itksys::SystemTools::GetParentDirectory(elastix));
    helper->SetImageData(fixedImage, movingImage);
    helper->SetFixedImageMaskData(fixedImageMask);
    helper->SetPointData(fixedPointSet, movingPointSet);
    helper->SetRegistrationParameters(parameterFiles);
    helper->SetRemoveWorkingDirectory(true);
    // helper.UseMovingImageSpacing(m_Controls.keepSpacings->isChecked());
    helper->SetStatusCallback(statusCallback);
    helper->GetRegistration();
    
    mitk::ProgressBar::GetInstance()->Progress(1);

    // warp original data
    mitk::ProgressBar::GetInstance()->Progress(1);
    MITK_INFO << "Use Count: " << helper.use_count();
    auto movingImage = dynamic_cast<const mitk::Image *>(movingImageNode->GetData());
    auto warpedImage = helper->WarpImage(movingImage);
    moving->SetTransformations(helper->GetTransformation());

    // auto timeString = itksys::SystemTools::GetCurrentDateTime("%H%M");
    // auto node = mitk::DataNode::New();
    // node->SetData(warpedImage);
    // node->SetName(movingImageNode->GetName() + "_warped" + timeString);

    // MITK_INFO << "Attachments: " << moving->m_Attachments.size();
    // for (const QmitkSingleNodeSelectionWidget *attSel : moving->m_Attachments)
    // {
    //   if (auto node = attSel->GetSelectedNode())
    //   {
    //     if (auto labelSetImage = dynamic_cast<mitk::LabelSetImage *>(node->GetData()))
    //     {
    //       auto wapred = helper->WarpImage(labelSetImage, "short", 1);
    //       auto resNode = mitk::DataNode::New();
    //       resNode->SetData(wapred);
    //       resNode->SetName(node->GetName() + "_warped_mask" + timeString);
    //       moving->m_ResultAttachments.push_back(resNode);
    //     }
    //     else if (auto image = dynamic_cast<mitk::Image *>(node->GetData()))
    //     {
    //       auto warped = helper->WarpImage(image, "float", 1);
    //       auto resNode = mitk::DataNode::New();
    //       resNode->SetData(warped);
    //       resNode->SetName(node->GetName() + "_warped_att_" + timeString);
    //       moving->m_ResultAttachments.push_back(resNode);
    //     }
    //   }
    // }

    mitk::DataNode *parentNode = fixedImageNode;
    MITK_INFO << "Add moving image node";
    // auto newNode = moving->m_ResultNode;
    auto newNode = mitk::DataNode::New();
    newNode->SetData(warpedImage);
    newNode->SetName(moving->GetImageNode()->GetName() + "(warped)");
    this->GetDataStorage()->Add(newNode, parentNode);
    mitk::ProgressBar::GetInstance()->Progress(1);

  }
  catch (std::exception &e)
  {
    MITK_ERROR << e.what();
  }
}



void RegistrationView::OnStartRegistration()
{
  // check dimensionalities
  unsigned int maxDimZ = 0;
  for (int i = 0; i < m_Controls.tabWidget->count(); i++)
  {
    auto data = dynamic_cast<RegistrationDataWidget *>(m_Controls.tabWidget->widget(i));
    if (data->HasImage())
    {
      auto image = data->GetImage();
      if (image->GetDimension() == 3)
        maxDimZ = std::max(image->GetDimensions()[2], maxDimZ);
    }
  }

  // Use deformable transformations
  std::setlocale(LC_NUMERIC, "en_US.UTF-8");

  const auto gridSpacing = m_Controls.spinBoxFinalGridSpacing->value();
  const auto iterations = m_Controls.spinBxIterations->value();
  

  m2::ElxUtil::ReplaceParameter(m_ParameterFiles[1], "FinalGridSpacingInPhysicalUnits", std::to_string(gridSpacing));
  m2::ElxUtil::ReplaceParameter(m_ParameterFiles[1], "MaximumNumberOfIterations", std::to_string(iterations));

  auto text = m_Controls.comboBox->currentText();
  if(text == "None"){
    m2::ElxUtil::ReplaceParameter(m_ParameterFiles[0], "AutomaticTransformInitialization", "false");
  }else if(text == "Geometrical Center"){
    m2::ElxUtil::ReplaceParameter(m_ParameterFiles[0], "AutomaticTransformInitialization", "true");
    m2::ElxUtil::ReplaceParameter(m_ParameterFiles[0], "AutomaticTransformInitializationMethod", "GeometricalCenter");
  }else if(text == "Center of Gravity"){
    m2::ElxUtil::ReplaceParameter(m_ParameterFiles[0], "AutomaticTransformInitialization", "true");
    m2::ElxUtil::ReplaceParameter(m_ParameterFiles[0], "AutomaticTransformInitializationMethod", "CenterOfGravity");
  }
  

  if (maxDimZ > 1)
  {
    m2::ElxUtil::ReplaceParameter(m_ParameterFiles[0], "FixedImageDimension", "3");
    m2::ElxUtil::ReplaceParameter(m_ParameterFiles[0], "MovingImageDimension", "3");
    m2::ElxUtil::ReplaceParameter(m_ParameterFiles[1], "FixedImageDimension", "3");
    m2::ElxUtil::ReplaceParameter(m_ParameterFiles[1], "MovingImageDimension", "3");
  }
  else
  {
    m2::ElxUtil::ReplaceParameter(m_ParameterFiles[0], "FixedImageDimension", "2");
    m2::ElxUtil::ReplaceParameter(m_ParameterFiles[0], "MovingImageDimension", "2");
    m2::ElxUtil::ReplaceParameter(m_ParameterFiles[1], "FixedImageDimension", "2");
    m2::ElxUtil::ReplaceParameter(m_ParameterFiles[1], "MovingImageDimension", "2");
  }
  // m_RegistrationJob = std::make_shared<QFutureWatcher<void>>();
  // m_RegistrationJob->setFuture(QtConcurrent::run([&]() {
    if (m_Controls.rbAllToOne->isChecked())
    {
      for (int i = 0; i < m_Controls.tabWidget->count(); i++)
      {
        auto data = dynamic_cast<RegistrationDataWidget *>(m_Controls.tabWidget->widget(i));
        if (m_FixedEntity != data)
          Registration(m_FixedEntity, data);
      }
    }

    if (m_Controls.rbSubsequent->isChecked())
    {
      std::list<RegistrationDataWidget *> upperEntities, lowerEntities;

      upperEntities.push_back(m_FixedEntity);
      lowerEntities.push_back(m_FixedEntity);

      int fixedIndex = 0;
      for (int i = 0; i < m_Controls.tabWidget->count(); i++)
      {
        auto data = dynamic_cast<RegistrationDataWidget *>(m_Controls.tabWidget->widget(i));
        if (data == m_FixedEntity)
        {
          fixedIndex = i;
          break;
        }
      }

      for (int i = 0; i < m_Controls.tabWidget->count(); i++)
      {
        auto data = dynamic_cast<RegistrationDataWidget *>(m_Controls.tabWidget->widget(i));
        if (i < fixedIndex)
          upperEntities.push_back(data);

        if (i > fixedIndex)
          lowerEntities.push_back(data);
      }

      {
        auto a = upperEntities.rbegin();
        auto b = std::next(upperEntities.rbegin());
        for (; b != upperEntities.rend(); ++a, ++b)
        {
          Registration(*a, *b);
        }
      }
      {
        auto a = lowerEntities.begin();
        auto b = std::next(lowerEntities.begin());
        for (; b != lowerEntities.end(); ++a, ++b)
        {
          Registration(*a, *b);
        }
      }
    }

    // auto resultPath = SystemTools::ConvertToOutputPath(SystemTools::JoinPath({path, "/", "result.1.nrrd"}));
    // MITK_INFO << "Result image path: " << resultPath;
    // if (SystemTools::FileExists(resultPath))
    // {
    //   auto vData = mitk::IOUtil::Load(resultPath);
    //   mitk::Image::Pointer image = dynamic_cast<mitk::Image *>(vData.back().GetPointer());
    //   auto node = mitk::DataNode::New();
    //   node->SetName(movingNode->GetName() + "_result");
    //   node->SetData(image);
    //   this->GetDataStorage()->Add(node, movingNode);
    // }

    // filesystem::remove(path);
  // }));
}
