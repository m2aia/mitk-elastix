#include <algorithm>
#include <clocale>
#include <m2ElxDefaultParameterFiles.h>
#include <m2ElxRegistrationHelper.h>
#include <m2ElxUtil.h>
#include <m2ElxConfig.h>

#include <itkVectorIndexSelectionCastImageFilter.h>
#include <itkExtractImageFilter.h>

#include <mitkImage.h>
#include <mitkImageCast.h>
#include <mitkImageAccessByItk.h>
#include <mitkImageReadAccessor.h>
#include <mitkImageWriteAccessor.h>
#include <numeric>

#include "itkDisplacementFieldTransform.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkResampleImageFilter.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include <itkConstantPadImageFilter.h>
#include <Poco/Environment.h>

m2::ElxRegistrationHelper::~ElxRegistrationHelper()
{
  // for(auto dir : m_ListOFWorkingDirectories)
  // {
  //   itksys::SystemTools::RemoveADirectory(dir);
  // }
}

m2::ElxRegistrationHelper::ElxRegistrationHelper()
{
  // m_ChannelSelections.push_back(std::make_pair(0,0));
  // m_ChannelSelections.push_back(std::make_pair(10,10));
}

bool m2::ElxRegistrationHelper::CheckDimensions(const mitk::Image *image) const
{
  const auto dims = image->GetDimension();
  // const auto sizeZ = image->GetDimensions()[2];

  return dims == 3 || dims == 2;
}

void m2::ElxRegistrationHelper::SymlinkOrWriteNrrd(mitk::Image::Pointer image, std::string targetPath) const
{
  std::string originalImageFilePath;
  image->GetPropertyList()->GetStringProperty("MITK.IO.reader.inputlocation", originalImageFilePath);
  auto convertedFixedImage = ConvertForElastixProcessing(image);

  if (convertedFixedImage == image &&                           // check if no conversion
      itksys::SystemTools::FileExists(originalImageFilePath) && // file is still available
      itksys::SystemTools::GetFilenameLastExtension(originalImageFilePath).compare(".nrrd") == 0 &&
      itksys::SystemTools::CreateSymlink(originalImageFilePath, targetPath) //  check if symlinking pass
  )
  {
    MITK_INFO << "Symlinked " << originalImageFilePath << " to " << targetPath;
  }
  else
  {
    mitk::IOUtil::Save(convertedFixedImage, targetPath);
  }
}

mitk::Image::Pointer m2::ElxRegistrationHelper::ConvertForElastixProcessing(const mitk::Image *image) const
{
  if (image)
  {
    const auto dim = image->GetDimension();
    // const auto dimensions = image->GetDimensions();
    const auto sizeZ = image->GetDimensions()[2];
    auto geom3D = image->GetSlicedGeometry();
    auto n = std::accumulate(image->GetDimensions(), image->GetDimensions() + 2, 1, std::multiplies<>()) *
             image->GetPixelType().GetSize();
    if (dim == 3 && sizeZ == 1)
    {
      auto output = mitk::Image::New();
      AccessByItk(const_cast<mitk::Image *>(image), ([&](auto I)
                                                     {
                                                       using Image3DType = typename std::remove_pointer<decltype(I)>::type;
                                                       using ImagePixelType = typename Image3DType::PixelType;
                                                       using Image2DType = itk::Image<ImagePixelType, 2>;

                                                       // Extract a 2D slice
                                                       using ExtractFilterType = itk::ExtractImageFilter<Image3DType, Image2DType>;
                                                       typename ExtractFilterType::Pointer extractFilter = ExtractFilterType::New();

                                                       // Set the input image
                                                       extractFilter->SetInput(I);

                                                       // Define the region to extract (slice)
                                                       typename Image3DType::RegionType inputRegion = I->GetLargestPossibleRegion();
                                                       typename Image3DType::SizeType size = inputRegion.GetSize();
                                                       size[2] = 0; // We are extracting a 2D slice along the z-axis

                                                       typename Image3DType::IndexType start = inputRegion.GetIndex();
                                                       start[2] = 0; // The slice index along the z-axis

                                                       typename Image3DType::RegionType desiredRegion;
                                                       desiredRegion.SetSize(size);
                                                       desiredRegion.SetIndex(start);

                                                       extractFilter->SetExtractionRegion(desiredRegion);
                                                       extractFilter->SetDirectionCollapseToSubmatrix();
                                                       extractFilter->Update();

                                                       mitk::CastToMitkImage(extractFilter->GetOutput(), output); }));
      return output;
    }
    else if (dim == 2)
    {
      // Keep 2D volume
      return const_cast<mitk::Image *>(image);
    }
    else if (dim == 3 && sizeZ > 1)
    {
      // Keep 3D volume
      return const_cast<mitk::Image *>(image);
    }
    else if (dim == 4)
    {
      // 4D => 3D volume (4th dim is > 0)
      // TODO: enable Selection of time step != 0
      auto output = mitk::Image::New();
      output->Initialize(image->GetPixelType(), 3, image->GetDimensions());
      output->GetSlicedGeometry()->SetSpacing(geom3D->GetSpacing());
      output->GetSlicedGeometry()->SetOrigin(geom3D->GetOrigin());
      output->GetSlicedGeometry()->SetDirectionVector(geom3D->GetDirectionVector());
      mitk::ImageReadAccessor iAcc(image);
      mitk::ImageWriteAccessor oAcc(output);
      std::copy((const char *)iAcc.GetData(), (const char *)iAcc.GetData() + n, (char *)oAcc.GetData());
      MITK_WARN << "3D+t Images are not well supported! Time step 0 selected.";
      return output;
    }
  }
  mitkThrow() << "Image data is null!";
}

mitk::Image::Pointer m2::ElxRegistrationHelper::ConvertForM2aiaProcessing(const mitk::Image *image) const
{
  if (image)
  {
    // auto geom3D = image->GetSlicedGeometry();
    // const auto N = std::accumulate(image->GetDimensions(), image->GetDimensions() + 2, 1, std::multiplies<>()) *
    //  image->GetPixelType().GetSize();
    if (image->GetDimension() == 2)
    {
      // Get the size of the 2D image
      mitk::PixelType pixelType = image->GetPixelType();
      unsigned int width = image->GetDimension(0);
      unsigned int height = image->GetDimension(1);

      // Create a new 3D image with the same width and height, but with a depth of 1
      mitk::Image::Pointer outputImage3D = mitk::Image::New();
      unsigned int dimensions[3] = {width, height, 1};
      outputImage3D->Initialize(pixelType, 3, dimensions);

      // Copy spacing, origin, and direction
      mitk::Vector3D spacing = image->GetGeometry()->GetSpacing();
      mitk::Point3D origin = image->GetGeometry()->GetOrigin();
      auto directionMatrix = image->GetGeometry()->GetIndexToWorldTransform();

      // Adjust the spacing and origin for the 3rd dimension
      spacing[2] = 1.0; // Arbitrary spacing for the single slice in the 3rd dimension
      origin[2] = 0.0;  // Set origin for the 3rd dimension

      outputImage3D->SetSpacing(spacing);
      outputImage3D->GetGeometry()->SetOrigin(origin);
      outputImage3D->GetGeometry()->SetIndexToWorldTransform(directionMatrix);

      // Create write access to the 3D image data
      mitk::ImageWriteAccessor writeAccess(outputImage3D);
      mitk::ImageReadAccessor readAccess(image);

      // Get the raw data pointers
      auto inputData = readAccess.GetData();
      auto outputData = writeAccess.GetData();

      // Copy the 2D image data to the first slice of the 3D image
      std::memcpy(outputData, inputData, image->GetPixelType().GetSize() * width * height);

      return outputImage3D;
    }
    else
    {
      return const_cast<mitk::Image *>(image);
    }
  }
  else
  {
    mitkThrow() << "Image data is null!";
  }
}

void m2::ElxRegistrationHelper::SetPointData(mitk::PointSet *fixed, mitk::PointSet *moving)
{
  if (fixed == nullptr || moving == nullptr)
  {
    MITK_WARN << "Fixed pointset is [" << fixed << "]; moving pointset is [" << moving << "]";
    MITK_WARN << "No pointsets are used.";
    m_UsePointsForRegistration = false;
  }
  else
  {
    m_FixedPoints = fixed;
    m_MovingPoints = moving;
    m_UsePointsForRegistration = true;
  }
}

void m2::ElxRegistrationHelper::SetFixedImageMaskData(mitk::Image *fixed)
{
  if (fixed == nullptr)
  {
    MITK_WARN << "Can not proceed: fixed mask is [" << fixed << "]";
    return;
  }

  m_FixedMask = fixed;

  // if images were already set, check if geometries fit
  if (m_FixedImage)
  {
    if (!mitk::Equal(*m_FixedImage->GetGeometry(), *m_FixedMask->GetGeometry()))
    {
      MITK_ERROR << "Fixed image [" << m2::ElxUtil::GetShape(m_FixedImage) << "] and fixed mask image ["
                 << m2::ElxUtil::GetShape(m_FixedMask) << "].\n"
                 << "Image geometries of mask image and image have to be equal!";
    }
  }
  m_UseMasksForRegistration = true;
}

void m2::ElxRegistrationHelper::SetImageData(mitk::Image *fixed, mitk::Image *moving)
{
  if (fixed == nullptr || moving == nullptr)
    mitkThrow() << "Can not proceed: fixed is [" << fixed << "]; moving is [" << moving << "]";

  // if (!CheckDimensions(fixed) && !CheckDimensions(moving))
  //   mitkThrow() << "Fixed image [" << m2::ElxUtil::GetShape(fixed) << "] and moving image ["
  //               << m2::ElxUtil::GetShape(moving) << "]! This is yet not implemented.\n"
  //               << "Shape has to be [NxMx1]";

  m_FixedImage = fixed;
  m_MovingImage = moving;

  // if masks were already set, check if geometries fit
  if (m_UseMasksForRegistration)
  {
    if (!(mitk::Equal(*m_FixedImage->GetGeometry(), *m_FixedMask->GetGeometry()) &&
          mitk::Equal(*m_MovingImage->GetGeometry(), *m_MovingMask->GetGeometry())))
    {
      mitkThrow() << "Fixed image [" << m2::ElxUtil::GetShape(m_FixedImage) << "] and fixed mask image ["
                  << m2::ElxUtil::GetShape(m_FixedMask) << "].\n"
                  << "Moving image [" << m2::ElxUtil::GetShape(m_MovingImage) << "] and moving mask image ["
                  << m2::ElxUtil::GetShape(m_MovingMask) << "].\n"
                  << "Image geometries of mask image and image have to be equal!";
    }
  }
}

void m2::ElxRegistrationHelper::SetDirectory(const std::string &dir)
{
  m_ExternalWorkingDirectory = dir;
}

void m2::ElxRegistrationHelper::SetRegistrationParameters(const std::vector<std::string> &params)
{
  m_RegistrationParameters = params;
}

void m2::ElxRegistrationHelper::SetAdditionalBinarySearchPath(const std::string &path)
{
  m_BinarySearchPath = ElxUtil::JoinPath({path});
}

void m2::ElxRegistrationHelper::SetChannelSelections(const std::vector<std::pair<unsigned int, unsigned int>> &channelSelections)
{
  m_ChannelSelections = channelSelections;
}

std::string m2::ElxRegistrationHelper::CreateWorkingDirectory() const
{
  // Create a temporary directory if workdir not defined
  std::string workingDirectory;
  if (m_ExternalWorkingDirectory.empty())
  {
    workingDirectory = ElxUtil::JoinPath({mitk::IOUtil::CreateTemporaryDirectory()});
    MITK_INFO << "Create Working Directory: " << workingDirectory;
  }
  else if (!itksys::SystemTools::PathExists(m_ExternalWorkingDirectory))
  {
    workingDirectory = ElxUtil::JoinPath({m_ExternalWorkingDirectory});
    itksys::SystemTools::MakeDirectory(workingDirectory);
    MITK_INFO << "Use External Working Directory: " << workingDirectory;
  }

  // m_ListOFWorkingDirectories.push_back(workingDirectory);
  return workingDirectory;
}

void m2::ElxRegistrationHelper::GetRegistration()
{
  if (m_FixedImage.IsNull() || m_MovingImage.IsNull())
  {
    MITK_ERROR << "No image set for registration!";
    return;
  }

  const auto exeElastix = m2::ElxUtil::Executable("elastix");
  if (exeElastix.empty())
    mitkThrow() << "Elastix executable not found!";
  MITK_INFO << "Use Elastix found at [" << exeElastix << "]";
  auto workingDirectory = CreateWorkingDirectory();
  MITK_INFO << workingDirectory << " " << itksys::SystemTools::PathExists(workingDirectory);
  

  if (m_RegistrationParameters.empty())
    m_RegistrationParameters.push_back(m2::Elx::Rigid());

  // Write parameter files
  for (unsigned int i = 0; i < m_RegistrationParameters.size(); ++i)
  {
    const auto targetParamterFilePath = ElxUtil::JoinPath({workingDirectory, "/", "pp" + std::to_string(i) + ".txt"});
    const auto element = m_RegistrationParameters[i];
    std::string parameterText;
    if (itksys::SystemTools::FileExists(element) && !itksys::SystemTools::FileIsDirectory(element))
    {
      const auto extension = itksys::SystemTools::GetFilenameExtension(element);
      // TODO: additional checks if p is a real path to an elastix parameter file
      auto ifs = std::ifstream(element);
      parameterText = std::string(std::istreambuf_iterator<char>{ifs}, {});
    }
    else
    {
      parameterText = element;
    }

    // ElxUtil::ReplaceParameter(parameterText, "AutomaticTransformInitializationMethod", "\"GeometricalCenters\"");
    // ElxUtil::ReplaceParameter(parameterText, "FinalGridSpacingInPhysicalUnits", "0.6");
    // ElxUtil::ReplaceParameter(parameterText, "FinalGridSpacingInVoxels", "\"GeometricalCenters\"");

    
    // add PointsEuclidianDistance metric
    if (m_UsePointsForRegistration)
    {
      if (parameterText.find("MultiResolution") != std::string::npos)
        ElxUtil::ReplaceParameter(parameterText, "Registration", "\"MultiMetricMultiResolutionRegistration\"");
      else
        ElxUtil::ReplaceParameter(parameterText, "Registration", "\"MultiMetricRegistration\"");
      ElxUtil::ReplaceParameter(
          parameterText, "Metric", "\"AdvancedMattesMutualInformation\" \"CorrespondingPointsEuclideanDistanceMetric\"");
    }
    // Write the parameter file to working directory
    std::ofstream outStream(targetParamterFilePath);
    outStream << parameterText;
    outStream.close();
    m_StatusFunction("Parameter file written: " + targetParamterFilePath);
  }

  std::vector<std::string> args;
  args.insert(args.end(), {"-out", workingDirectory});

  // SAVE MOVING IMAGE(s) ON DISK
  if (m_MovingImage->GetPixelType().GetNumberOfComponents() > 1)
  {
    mitk::Image::Pointer output;
    unsigned int component = 0;
    for (auto channelSelection : m_ChannelSelections)
    {
      AccessVectorPixelTypeByItk(m_MovingImage, ([&](auto itkImage)
                                                 {
          using SourceImageType = typename std::remove_pointer<decltype(itkImage)>::type;
          using ScalarImageType = itk::Image<typename SourceImageType::ValueType::ComponentType, SourceImageType::ImageDimension>;
          using IndexSelectionType = itk::VectorIndexSelectionCastImageFilter<SourceImageType, ScalarImageType>;
          auto indexSelectionFilter = IndexSelectionType::New();
          indexSelectionFilter->SetIndex(channelSelection.second);
          indexSelectionFilter->SetInput(itkImage);
          indexSelectionFilter->Update();
          mitk::CastToMitkImage(indexSelectionFilter->GetOutput(), output); }));
          const auto movingPath = ElxUtil::JoinPath({workingDirectory, "/", "moving" + std::to_string(channelSelection.second) + ".nrrd"});
          SymlinkOrWriteNrrd(output, movingPath);
          args.insert(args.end(), {"-m" + std::to_string(component), movingPath});
          ++component;
    }
  }
  else
  {
    const auto movingPath = ElxUtil::JoinPath({workingDirectory, "/", "moving.nrrd"});
    SymlinkOrWriteNrrd(m_MovingImage, movingPath);
    args.insert(args.end(), {"-m", movingPath});
  }

  // SAVE FIXED IMAGE(s) ON DISK
  if (m_FixedImage->GetPixelType().GetNumberOfComponents() > 1)
  {
    mitk::Image::Pointer output;
    unsigned int component = 0;
    for (auto channelSelection : m_ChannelSelections)
    {
      AccessVectorPixelTypeByItk(m_FixedImage, ([&](auto itkImage)
                                                {
          using SourceImageType = typename std::remove_pointer<decltype(itkImage)>::type;
          using ScalarImageType = itk::Image<typename SourceImageType::ValueType::ComponentType, SourceImageType::ImageDimension>;
          using IndexSelectionType = itk::VectorIndexSelectionCastImageFilter<SourceImageType, ScalarImageType>;
          auto indexSelectionFilter = IndexSelectionType::New();
          indexSelectionFilter->SetIndex(channelSelection.first);
          indexSelectionFilter->SetInput(itkImage);
          indexSelectionFilter->Update();
          mitk::CastToMitkImage(indexSelectionFilter->GetOutput(), output); 
        }));
      const auto fixedPath = ElxUtil::JoinPath({workingDirectory, "/", "fixed" + std::to_string(channelSelection.first) + ".nrrd"});
      // const auto movingPath = ElxUtil::JoinPath({workingDirectory, "/", "moving"+std::to_string(component)+".nrrd"});
      SymlinkOrWriteNrrd(output, fixedPath);
      args.insert(args.end(), {"-f" + std::to_string(component), fixedPath});
      ++component;
    }
  }
  else
  {
    const auto fixedPath = ElxUtil::JoinPath({workingDirectory, "/", "fixed.nrrd"});

    mitk::Image::Pointer outputImage = m_FixedImage;
    // AccessByItk(m_FixedImage, ([&](auto I)
    // {
    //   using ImageType = std::remove_pointer_t<decltype(I)>;

    //   auto padFilter = itk::ConstantPadImageFilter<ImageType, ImageType>::New();
    //   padFilter->SetInput(I);
    //   auto size = I->GetLargestPossibleRegion().GetSize();
    //   decltype(size) lowerPadding;
    //   lowerPadding[0] = size[0]/6;
    //   lowerPadding[1] = size[1]/6;
    //   lowerPadding[2] = 0;

    //   // decltype(size) upperPadding;
    //   // upperPadding[0] = size[0] + 2*lowerPadding[0];
    //   // upperPadding[1] = size[1] + 2*lowerPadding[1];
    //   // upperPadding[2] = 0;

    //   padFilter->SetPadLowerBound(lowerPadding);
    //   padFilter->SetPadUpperBound(lowerPadding);
    //   padFilter->Update();
    //   mitk::CastToMitkImage(padFilter->GetOutput(), outputImage);
    // }));
    SymlinkOrWriteNrrd(outputImage, fixedPath);
    args.insert(args.end(), {"-f", fixedPath});
  }

  if (m_UseMasksForRegistration)
  {
    const auto fixedMaskPath = ElxUtil::JoinPath({workingDirectory, "/", "fixedMask.nrrd"});
    mitk::IOUtil::Save(ConvertForElastixProcessing(m_FixedMask), fixedMaskPath);
    args.insert(args.end(), {"-fMask", fixedMaskPath});
    // args.insert(args.end(), {"-mMask", movingMaskPath});
    // const auto movingMaskPath = ElxUtil::JoinPath({workingDirectory, "/", "movingMask.nrrd"});
    // mitk::IOUtil::Save(GetSlice2DData(m_MovingMask), movingMaskPath);
  }

  if (m_UsePointsForRegistration)
  {
    const auto fixedPointsPath = ElxUtil::JoinPath({workingDirectory, "/", "fixedPoints.txt"});
    const auto movingPointsPath = ElxUtil::JoinPath({workingDirectory, "/", "movingPoints.txt"});
    ElxUtil::SavePointSet(m_MovingPoints, movingPointsPath);
    ElxUtil::SavePointSet(m_FixedPoints, fixedPointsPath);
    args.insert(args.end(), {"-mp", movingPointsPath});
    args.insert(args.end(), {"-fp", fixedPointsPath});
  }

  for (unsigned int i = 0; i < m_RegistrationParameters.size(); ++i)
  {
    const auto parameterFile = m2::ElxUtil::JoinPath({workingDirectory, "/", "pp" + std::to_string(i) + ".txt"});
    args.insert(args.end(), {"-p", parameterFile});
  }

  MITK_INFO << "Registration started ...";
  MITK_INFO << exeElastix;
  for(auto kv : args){
    MITK_INFO << kv;
  }
  
  
  // const std::map<std::string , std::string> env{ {std::string("LD_LIBRARY_PATH"), std::string(Elastix_LIBRARY)}};
  // Poco::ProcessHandle ph(Poco::Process::launch(exeElastix, args, nullptr, &oPipe, nullptr, env));
  // Poco::ProcessHandle ph(Poco::Process::launch(exeElastix, args, nullptr, nullptr, nullptr));
  // ph.wait();

  m2::ElxUtil::run(exeElastix, args);

  

  MITK_INFO << "Registration finished.";
  for (unsigned int i = 0; i < m_RegistrationParameters.size(); ++i)
  {
    const auto transformationParameterFile =
        ElxUtil::JoinPath({workingDirectory, "/", "TransformParameters." + std::to_string(i) + ".txt"});
    auto ifs = std::ifstream(transformationParameterFile);
    m_Transformations.emplace_back(std::string{std::istreambuf_iterator<char>{ifs}, {}});
  }

  auto logFilePath = ElxUtil::JoinPath({workingDirectory, "/", "elastix.log"});
  // open logfile and scan last line for "error"
  std::ifstream logFile(logFilePath);
  std::string lastLine;
  while (logFile >> std::ws && std::getline(logFile, lastLine))
    ;
  if (lastLine.find("Error") != std::string::npos)
  {
    mitkThrow() << "Elastix log file contains error: " << lastLine;
  }

  MITK_INFO << "Transformation parameters assimilated";
  // try{
  TransformixDeformationField(workingDirectory);
  // }catch(std::exception& e){
  MITK_INFO << "Registration OK!";
  // }
  // RemoveWorkingDirectory(workingDirectory);
}

void m2::ElxRegistrationHelper::SetStatusCallback(const std::function<void(std::string)> &callback)
{
  m_StatusFunction = callback;
}

std::vector<std::string> m2::ElxRegistrationHelper::GetTransformation() const
{
  return m_Transformations;
}

void m2::ElxRegistrationHelper::SetTransformations(const std::vector<std::string> &transforms)
{
  m_Transformations = transforms;
}

std::string m2::ElxRegistrationHelper::WriteTransformation(std::string workingDirectory) const
{
  std::string transformationPath;
  for (unsigned int i = 0; i < m_Transformations.size(); ++i)
  {
    transformationPath = ElxUtil::JoinPath({workingDirectory, "/", "TransformParameters." + std::to_string(i) + ".txt"});
    std::ofstream(transformationPath) << m_Transformations[i];
  }
  return transformationPath;
}

void m2::ElxRegistrationHelper::TransformixDeformationField(std::string workingDirectory)
{
  const auto exeTransformix = m2::ElxUtil::Executable("transformix", m_BinarySearchPath);
  if (exeTransformix.empty())
    mitkThrow() << "Transformix executable not found!";

  const auto resultPath = ElxUtil::JoinPath({workingDirectory, "/", "result.nrrd"});
  const auto deformationFieldPath = ElxUtil::JoinPath({workingDirectory, "/", "deformationField.nrrd"});
  auto transformationPath = WriteTransformation(workingDirectory);

  try
  {
    Poco::Process::Args args;
    args.insert(args.end(), {"-def", "all"});
    args.insert(args.end(), {"-tp", transformationPath});
    args.insert(args.end(), {"-out", workingDirectory});

    

    // Poco::Pipe oPipe;
    // const std::map<std::string , std::string> env{ {std::string("LD_LIBRARY_PATH"), std::string(Elastix_LIBRARY)}};
    // Poco::ProcessHandle ph(Poco::Process::launch(exeTransformix, args, nullptr, &oPipe, nullptr, env));
    // Poco::ProcessHandle ph(Poco::Process::launch(exeTransformix, args, nullptr, nullptr, nullptr));
    // ph.wait();

    m2::ElxUtil::run(exeTransformix, args);

    // oPipe.close();

    auto dataVector = mitk::IOUtil::Load(deformationFieldPath);
    if (auto deformationField = dynamic_cast<mitk::Image *>(dataVector.front().GetPointer()))
    {
      m_DeformationField = deformationField;
    }
    else
    {
      MITK_ERROR << "The object found at" + deformationFieldPath + "is not an image!";
    }
  }
  catch (std::exception &e)
  {
    MITK_ERROR << "Error loading deformation field: " << e.what();
  }
}

mitk::Image::Pointer m2::ElxRegistrationHelper::GetDeformationField() const
{

  return m_DeformationField;
}

mitk::Image::Pointer m2::ElxRegistrationHelper::WarpImage(const mitk::Image *inputData,
                                                          const std::string &pixelType,
                                                          const unsigned char &interpolationOrder) const
{
  auto data = ConvertForElastixProcessing(inputData);
  auto fixedData = ConvertForElastixProcessing(m_FixedImage);

  if (!CheckDimensions(data))
  {
    MITK_ERROR << "Image [" << m2::ElxUtil::GetShape(data) << "]. This is yet not implemented.\n"
               << "Shape has to be [NxMx1]";
    return nullptr;
  }

 if (!CheckDimensions(fixedData))
  {
    MITK_ERROR << "Image [" << m2::ElxUtil::GetShape(fixedData) << "]. This is yet not implemented.\n"
               << "Shape has to be [NxMx1]";
    return nullptr;
  }
  
  if (auto deformationField = GetDeformationField())
  {
    using VectorImageType = itk::VectorImage<double, 2>;
    auto itkDeformationField = VectorImageType::New();
    mitk::CastToItkImage(deformationField, itkDeformationField);

    mitk::Image::Pointer result = mitk::Image::New();
    AccessTwoImagesFixedDimensionByItk(data, fixedData, ([&](auto itkMovingImage2D, auto itkFixedImage2D)
    {
      using ImageType = typename std::remove_pointer<decltype(itkMovingImage2D)>::type;      
      using TransformType = itk::DisplacementFieldTransform<double, 2>;
      auto transform = TransformType::New();
      transform->SetDisplacementField(itkDeformationField);

      using ResampleFilterType = itk::ResampleImageFilter<ImageType, ImageType>;
      auto resampler = ResampleFilterType::New();
      resampler->SetInput(itkMovingImage2D);
      resampler->SetTransform(transform.GetPointer());
      resampler->SetReferenceImage(itkFixedImage2D);
      resampler->UseReferenceImageOn();

      if (pixelType == "short")
      {
        resampler->SetInterpolator(itk::NearestNeighborInterpolateImageFunction<ImageType>::New());
      }else{
        resampler->SetInterpolator(itk::LinearInterpolateImageFunction<ImageType>::New());
      }
      resampler->SetDefaultPixelValue(0);

      resampler->Update();
      mitk::CastToMitkImage(resampler->GetOutput(), result); 
    }),2);
    
    try
    {
      result = ConvertForM2aiaProcessing(result);

      if (result->GetDimensions()[2] == 1)
      {
        auto s = result->GetGeometry()->GetSpacing();
        s[2] = inputData->GetGeometry()->GetSpacing()[2];
        result->GetGeometry()->SetSpacing(s);
      }
    }
    catch (std::exception &e)
    {
      MITK_ERROR << "Error loading warped image (deformation field): " << e.what();
    }
    return result;
  }
  else
  {
    const auto exeTransformix = m2::ElxUtil::Executable("transformix", m_BinarySearchPath);
    if (exeTransformix.empty())
      mitkThrow() << "Transformix executable not found!";
    auto workingDirectory = CreateWorkingDirectory();
    const auto imagePath = ElxUtil::JoinPath({workingDirectory, "/", "data.nrrd"});
    const auto resultPath = ElxUtil::JoinPath({workingDirectory, "/", "result.nrrd"});

    mitk::IOUtil::Save(data, imagePath);

    for (unsigned int i = 0; i < m_Transformations.size(); ++i)
    {
      auto transformationPath =
          ElxUtil::JoinPath({workingDirectory, "/", "TransformParameters." + std::to_string(i) + ".txt"});

      auto T = m_Transformations[i];

      ElxUtil::ReplaceParameter(T, "ResultImagePixelType", "\"" + pixelType + "\"");
      if (pixelType == "short")
      {
        ElxUtil::ReplaceParameter(T, "ResampleInterpolator", "\"FinalNearestNeighborInterpolator\"");
      }
      else
      {
        ElxUtil::ReplaceParameter(T, "ResampleInterpolator", "\"FinalBSplineInterpolatorFloat\"");
        ElxUtil::ReplaceParameter(T, "FinalBSplineInterpolationOrder", std::to_string(interpolationOrder));
      }

      if (i == 0)
      {
        ElxUtil::ReplaceParameter(T, "InitialTransformParametersFileName", R"("NoInitialTransform")");
      }
      else if (i > 0)
      {
        const auto initialTransform =
            ElxUtil::JoinPath({workingDirectory, "/", "TransformParameters." + std::to_string(i - 1) + ".txt"});
        ElxUtil::ReplaceParameter(T, "InitialTransformParametersFileName", "\"" + initialTransform + "\"");
      }

      std::ofstream(transformationPath) << T;
    }

    const auto transformationPath = ElxUtil::JoinPath(
        {workingDirectory, "/", "TransformParameters." + std::to_string(m_Transformations.size() - 1) + ".txt"});

    Poco::Process::Args args;
    args.insert(args.end(), {"-in", imagePath});
    args.insert(args.end(), {"-tp", transformationPath});
    args.insert(args.end(), {"-out", workingDirectory});

    

    // Poco::Pipe oPipe;
    // const std::map<std::string , std::string> env{ {std::string("LD_LIBRARY_PATH"), std::string(Elastix_LIBRARY)}};
    // Poco::ProcessHandle ph(Poco::Process::launch(exeTransformix, args, nullptr, &oPipe, nullptr, env));
    // Poco::ProcessHandle ph(Poco::Process::launch(exeTransformix, args, nullptr, nullptr, nullptr));
    // ph.wait();

    m2::ElxUtil::run(exeTransformix, args);
    
    // oPipe.close();
    mitk::Image::Pointer result;
    try
    {
      auto resultData = mitk::IOUtil::Load(resultPath).front();
      result = dynamic_cast<mitk::Image *>(resultData.GetPointer());

      result = ConvertForM2aiaProcessing(result);

      if (result->GetDimensions()[2] == 1)
      {
        auto s = result->GetGeometry()->GetSpacing();
        s[2] = inputData->GetGeometry()->GetSpacing()[2];
        result->GetGeometry()->SetSpacing(s);
      }
    }
    catch (std::exception &e)
    {
      MITK_ERROR << "Error loading warped image: " << e.what();
    }

    // RemoveWorkingDirectory(workingDirectory);
    return result;
  }
}

void m2::ElxRegistrationHelper::SetRemoveWorkingDirectory(bool val)
{
  m_RemoveWorkingDirectory = val;
}

void m2::ElxRegistrationHelper::RemoveWorkingDirectory(std::string workingDirectory) const
{
  try
  {
    if (m_RemoveWorkingDirectory && itksys::SystemTools::PathExists(workingDirectory) &&
        itksys::SystemTools::FileIsDirectory(workingDirectory))
    {
      itksys::SystemTools::RemoveADirectory(workingDirectory);
    }
  }
  catch (std::exception &e)
  {
    MITK_ERROR << "Cleanup ElxRegistrationHelper fails!\n"
               << e.what();
  }
}