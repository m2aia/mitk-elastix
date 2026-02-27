

#include <m2ElxDefaultParameterFiles.h>

    std::string m2::Elx::Rigid()
    {
      return R"(// =========================================================
// Rigid registration parameter file
// NOTE: The following parameters are overridden at runtime
// by the Registration UI controls:
//   Transform, Metric, NumberOfHistogramBins,
//   NumberOfResolutions, MaximumNumberOfIterations,
//   NumberOfSpatialSamples, Interpolator,
//   BSplineInterpolationOrder, ResampleInterpolator,
//   FinalBSplineInterpolationOrder,
//   AutomaticTransformInitialization,
//   AutomaticTransformInitializationMethod,
//   FixedImageDimension, MovingImageDimension
// =========================================================

(FixedInternalImagePixelType "float")
(MovingInternalImagePixelType "float")
(FixedImageDimension 2)
(MovingImageDimension 2)
(UseDirectionCosines "true")

// **************** Main Components **************************

(Registration "MultiResolutionRegistration")
(Interpolator "LinearInterpolator")
(ResampleInterpolator "FinalLinearInterpolator")
(Resampler "DefaultResampler")
(FixedImagePyramid "FixedRecursiveImagePyramid")
(MovingImagePyramid "MovingRecursiveImagePyramid")
(Optimizer "AdaptiveStochasticGradientDescent")
(Metric "AdvancedMattesMutualInformation")
(Transform "EulerTransform")

// ***************** Transformation **************************

(AutomaticScalesEstimation "true")
(AutomaticTransformInitialization "true")
(AutomaticTransformInitializationMethod "CenterOfGravity")
(HowToCombineTransforms "Compose")

// ******************* Similarity measure *********************

(NumberOfHistogramBins 32)

// If you use a mask, this option is important.
// If the mask serves as region of interest, set it to false.
// If the mask indicates which pixels are valid, then set it to true.
// If you do not use a mask, the option doesn't matter.
(ErodeMask "true")

// ******************** Multiresolution **********************

(NumberOfResolutions 3)

// ******************* Optimizer ****************************

(MaximumNumberOfIterations 300)

// The step size of the optimizer, in mm. By default the voxel size is used.
// which usually works well. In case of unusual high-resolution images
// (eg histology) it is necessary to increase this value a bit, to the size
// of the "smallest visible structure" in the image:
// (MaximumStepLength 5.0 3.0 1.0)

// **************** Image sampling **********************

(NumberOfSpatialSamples 25000)
(NewSamplesEveryIteration "true")
(ImageSampler "Random")

// ************* Interpolation and Resampling ****************

// (BSplineInterpolationOrder 3)

// Order of B-Spline interpolation used for applying the final
// deformation.
// 3 gives good accuracy; recommended in most cases.
// 1 gives worse accuracy (linear interpolation)
// 0 gives worst accuracy, but is appropriate for binary images
// (masks, segmentations); equivalent to nearest neighbor interpolation.
// (FinalBSplineInterpolationOrder 3)

// Default pixel value for pixels that come from outside the picture:
(DefaultPixelValue 0)

(WriteTransformParametersEachResolution "false")
(WriteResultImage "true")
(ResultImagePixelType "float")
(ResultImageFormat "nrrd")
)";
    }


std::string m2::Elx::Deformable()
    {
      return R"(// =========================================================
// Deformable registration parameter file
// NOTE: The following parameters are overridden at runtime
// by the Registration UI controls:
//   Transform, Metric, NumberOfHistogramBins,
//   NumberOfResolutions, MaximumNumberOfIterations,
//   FinalGridSpacingInPhysicalUnits, NumberOfSpatialSamples,
//   Interpolator, BSplineInterpolationOrder,
//   ResampleInterpolator, FinalBSplineInterpolationOrder,
//   FixedImageDimension, MovingImageDimension
// =========================================================

(FixedInternalImagePixelType "float")
(MovingInternalImagePixelType "float")
(FixedImageDimension 2)
(MovingImageDimension 2)
(UseDirectionCosines "true")

// **************** Main Components **************************

(Registration "MultiResolutionRegistration")
(Interpolator "LinearInterpolator")
// (ResampleInterpolator "FinalBSplineInterpolator")
(Resampler "DefaultResampler")
(Optimizer "AdaptiveStochasticGradientDescent")
(Transform "RecursiveBSplineTransform")
(Metric "AdvancedMattesMutualInformation")
(Metric0Weight 1.00)
(Metric1Weight 0.80)

// ******************** Multiresolution **********************

(NumberOfResolutions 4)

// ***************** Transformation **************************

(FinalGridSpacingInPhysicalUnits 35.0)
// (FinalGridSpacingInVoxels 40)
// (GridSpacingSchedule 8.0 8.0 3.0 3.0 2.5 2.5 1.0 1.0)
// (GridSpacingSchedule 4.0 4.0 2.0 1.0)
(HowToCombineTransforms "Compose")

// ******************* Similarity measure *********************

(NumberOfHistogramBins 32)

// If you use a mask, this option is important.
// If the mask serves as region of interest, set it to false.
// If the mask indicates which pixels are valid, then set it to true.
// If you do not use a mask, the option doesn't matter.
(ErodeMask "true")

// ******************* Optimizer ****************************

(MaximumNumberOfIterations 750)

// The step size of the optimizer, in mm. By default the voxel size is used.
// which usually works well. In case of unusual high-resolution images
// (eg histology) it is necessary to increase this value a bit, to the size
// of the "smallest visible structure" in the image:
// (MaximumStepLength 100 80 70 60 50 10 10 1 1 1)

// **************** Image sampling **********************

(NumberOfSpatialSamples 25000)
(NewSamplesEveryIteration "true")
(ImageSampler "Random")

// ************* Interpolation and Resampling ****************

// (BSplineInterpolationOrder 3)

// Order of B-Spline interpolation used for applying the final
// deformation.
// 3 gives good accuracy; recommended in most cases.
// 1 gives worse accuracy (linear interpolation)
// 0 gives worst accuracy, but is appropriate for binary images
// (masks, segmentations); equivalent to nearest neighbor interpolation.
// (FinalBSplineInterpolationOrder 3)

// Default pixel value for pixels that come from outside the picture:
(DefaultPixelValue 0)

// Choose whether to generate the deformed moving image.
// You can save some time by setting this to false, if you are
// not interested in the final deformed moving image, but only
// want to analyze the deformation field for example.
(WriteResultImage "true")
(WriteTransformParametersEachResolution "false")
//(WriteResultImageAfterEachResolution "true")

// The pixel type and format of the resulting deformed moving image
(ResultImagePixelType "float")
(ResultImageFormat "nrrd")
)";
    }
