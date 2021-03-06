#pragma once

#include "itkConstantPadImageFilter.h"
#include "itkNeighborhoodIterator.h"
#include "itkShrinkImageFilter.h"
#include "itkMaskImageFilter.h"
#include "itkThresholdImageFilter.h"
#include "itkMaskedImageToHistogramFilter.h"
#include "itkImageToListSampleAdaptor.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkMultiplyImageFilter.h"
#include "itkSubtractImageFilter.h"
#include "itkDivideImageFilter.h"

#include "cbicaITKUtilities.h"
#include "cbicaStatistics.h"

template< class TImageType = itk::Image< float, 3 > >
class DeepMedicNormalizer
{
  using TIteratorType = itk::ImageRegionIteratorWithIndex< TImageType >;
  using TConstIteratorType = itk::ImageRegionConstIterator< TImageType >;

public:
  DeepMedicNormalizer() {};
  ~DeepMedicNormalizer() {};

  //! Sets the input image
  void SetInputImage(typename TImageType::Pointer image) 
  { 
    m_inputImage = image;
    m_algorithmDone = false;
  }

  //! Sets the input mask
  void SetInputMask(typename TImageType::Pointer image)
  {
    m_mask = image;
    m_algorithmDone = false;
  }

  //! Set the quantile locations
  void SetQuantiles(float lower, float upper)
  {
    m_quantLower = lower;
    m_quantUpper = upper;
  }

  //! Set the cutoff locations
  void SetCutoffs(float lower, float upper)
  {
    m_cutoffLower = lower;
    m_cutoffUpper = upper;
  }

  //! Actual computer
  void Update()
  {
    if (!m_algorithmDone)
    {
      if (m_doSanityCheck)
      {
        if (m_mask.IsNull())
        {
          m_inputImage = cbica::CreateImage< TImageType >(m_inputImage);
        }
        auto inputSize = m_inputImage->GetLargestPossibleRegion().GetSize();
        auto inputSpacing = m_inputImage->GetSpacing();
        auto inputOrigin = m_inputImage->GetOrigin();
        auto maskSize = m_mask->GetLargestPossibleRegion().GetSize();
        auto maskSpacing = m_mask->GetSpacing();
        auto maskOrigin = m_mask->GetOrigin();

        if (m_inputImage->GetNumberOfComponentsPerPixel() != 1)
        {
          std::cerr << "Only grayscale images are supported for normalization (inputImage is non-gray).\n";
          exit(EXIT_FAILURE);
        }
        if (m_mask->GetNumberOfComponentsPerPixel() != 1)
        {
          std::cerr << "Only grayscale images are supported for normalization (maskImage is non-gray).\n";
          exit(EXIT_FAILURE);
        }
        for (size_t i = 0; i < m_inputImage->GetImageDimension(); i++)
        {
          if (inputSize[i] != maskSize[i])
          {
            std::cerr << "Size mismatch between inputImage and maskImage.\n";
            exit(EXIT_FAILURE);
          }
          if (inputSpacing[i] != maskSpacing[i])
          {
            std::cerr << "Spacing mismatch between inputImage and maskImage.\n";
            exit(EXIT_FAILURE);
          }
          if (inputOrigin[i] != maskOrigin[i])
          {
            std::cerr << "Origin mismatch between inputImage and maskImage.\n";
            exit(EXIT_FAILURE);
          }
        }

        bool emptyMask = true;
        // ensure mask is always defined as '1' or '0'
        TIteratorType maskIter(m_mask, m_mask->GetLargestPossibleRegion());
        for (maskIter.GoToBegin(); !maskIter.IsAtEnd(); ++maskIter)
        {
          if (maskIter.Get() > 0)
          {
            maskIter.Set(1);
            emptyMask = false;
          }
        }

        if (emptyMask)
        {
          m_mask = cbica::CreateImage< TImageType >(m_inputImage, 1);
        }

        // apply the mask to the m_inputImage
        auto maskFilter = itk::MaskImageFilter< TImageType, TImageType >::New();
        maskFilter->SetInput(m_inputImage);
        maskFilter->SetMaskImage(m_mask);
        maskFilter->Update();
        m_inputImageMasked = maskFilter->GetOutput();
      }

      // estimate statistics
      auto stats_originalImage = GetStatisticsForImage(m_inputImage, false);
      auto stats_inROI = GetStatisticsForImage(m_inputImageMasked); // by default, these statistics are used for normalization
      auto stats_forNorm = stats_inROI; // by default, these statistics are used for normalization

                                        // we want to maximize the lower threshold and minimize the upper threshold to exclude the maximum number of voxels
      float lowerThresh = stats_inROI["Min"];
      float upperThresh = stats_inROI["Max"];

      if ((m_quantLower != 0) && (m_quantUpper != 0))
      {
        if (m_quantLower > 1)
        {
          m_quantLower /= 100;
        }
        if (m_quantUpper > 1)
        {
          m_quantUpper /= 100;
        }

        // initialize the histogram    
        using ImageToHistogramFilterType = itk::Statistics::MaskedImageToHistogramFilter< TImageType, TImageType >;

        const unsigned int numberOfComponents = 1; // we are always assuming a greyscale image
        typename ImageToHistogramFilterType::HistogramType::SizeType size(numberOfComponents);
        size.Fill(stats_inROI["Max"]); // maximum calculated before will be the max in the histogram

        auto filter = ImageToHistogramFilterType::New();
        typename ImageToHistogramFilterType::HistogramType::MeasurementVectorType min(numberOfComponents), max(numberOfComponents);
        min.fill(stats_inROI["Min"]);
        max.fill(stats_inROI["Max"]);
        filter->SetInput(m_inputImageMasked);
        filter->SetMaskImage(m_mask);
        filter->SetMaskValue(1);
        filter->SetHistogramSize(size);
        filter->SetHistogramBinMinimum(min);
        filter->SetHistogramBinMaximum(max);
        filter->SetMarginalScale(1000); // this is for accuracy
        filter->Update();

        auto histogram = filter->GetOutput();

        auto lower = histogram->Quantile(0, m_quantLower);
        auto upper = histogram->Quantile(0, m_quantUpper);

        if (lower > lowerThresh)
        {
          lowerThresh = lower;
        }
        if (upper < upperThresh)
        {
          upperThresh = upper;
        }
      }

      if ((m_cutoffLower != 0) && (m_cutoffUpper != 0))
      {
        float lower = stats_inROI["Mean"] - stats_inROI["Std"] * m_cutoffLower;
        float upper = stats_inROI["Mean"] + stats_inROI["Std"] * m_cutoffUpper;

        std::cout << "Cut-offs calculated: Lower = " << lower << "; Upper = " << upper << "\n";

        if (lower > lowerThresh)
        {
          lowerThresh = lower;
        }
        if (upper < upperThresh)
        {
          upperThresh = upper;
        }
      }

      if (m_wholeImageMeanThreshold)
      {
        if (stats_originalImage["Mean"] > lowerThresh)
        {
          lowerThresh = stats_originalImage["Mean"];
        }
      }

      auto thresholder = itk::ThresholdImageFilter< TImageType >::New();
      thresholder->SetInput(m_inputImage);
      thresholder->ThresholdBelow(lowerThresh);
      thresholder->ThresholdAbove(upperThresh);
      thresholder->Update();

      auto maskUpdater = itk::BinaryThresholdImageFilter< TImageType, TImageType >::New();
      maskUpdater->SetInput(thresholder->GetOutput());
      maskUpdater->SetLowerThreshold(lowerThresh);
      maskUpdater->SetUpperThreshold(upperThresh);
      maskUpdater->SetInsideValue(1);
      maskUpdater->SetOutsideValue(0);
      maskUpdater->Update();

      // [orgCode] boolMaskForStatsCalc = boolMaskForStatsCalc * boolOverLowCutoff * boolBelowHighCutoff
      auto multiplier = itk::MultiplyImageFilter< TImageType >::New();
      multiplier->SetInput1(maskUpdater->GetOutput());
      multiplier->SetInput2(m_mask);
      multiplier->Update();
      //maskForStats = multiplier->GetOutput();
      //maskForStats->DisconnectPipeline();

      // [orgCode] srcImgNpArr[boolMaskForStatsCalc]
      auto maskFilter_new = itk::MaskImageFilter< TImageType, TImageType >::New();
      maskFilter_new->SetInput(m_inputImage);
      maskFilter_new->SetMaskImage(multiplier->GetOutput());
      maskFilter_new->SetMaskingValue(0);
      maskFilter_new->Update();

      stats_forNorm = GetStatisticsForImage(maskFilter_new->GetOutput());

      // applying the statistics
      auto subtractor = itk::SubtractImageFilter< TImageType >::New();
      subtractor->SetInput1(m_inputImage);
      subtractor->SetConstant2(stats_forNorm["Mean"]);
      subtractor->Update();
      auto divider = itk::DivideImageFilter< TImageType, TImageType, TImageType >::New();
      divider->SetInput1(subtractor->GetOutput());
      divider->SetConstant2(stats_forNorm["Std"]);
      divider->Update();

      m_output = divider->GetOutput();

      m_algorithmDone = true;
    }
  }

  typename TImageType::Pointer GetOutput()
  {
    if (!m_algorithmDone)
    {
      Update();
    }
    return m_output;
  }

private:

  //! Get basic statistics from image
  std::map< std::string, double > GetStatisticsForImage(const typename TImageType::Pointer m_inputImage, bool considerMask = true)
  {
    std::map< std::string, double > results;
    std::vector< typename TImageType::PixelType > nonZeroPixels;
    // mean, stdDev, max

    TConstIteratorType  imageIterator(m_inputImage, m_inputImage->GetLargestPossibleRegion());
    imageIterator.GoToBegin();
    while (!imageIterator.IsAtEnd())
    {
      auto currentPixel = imageIterator.Get();
      if (considerMask)
      {
        if (currentPixel > 0)
        {
          nonZeroPixels.push_back(currentPixel);
        }
      }
      else
      {
        nonZeroPixels.push_back(currentPixel);
      }

      ++imageIterator;
    }
    cbica::Statistics< typename TImageType::PixelType > calculator;
    calculator.SetInput(nonZeroPixels);

    results["Max"] = calculator.GetMaximum();
    results["Min"] = calculator.GetMinimum();
    results["Std"] = calculator.GetStandardDeviation();
    results["Mean"] = calculator.GetMean();

    return results;
  }

  typename TImageType::Pointer m_inputImage, m_mask, m_inputImageMasked, m_output;
  float m_quantLower = 0.05, m_quantUpper = 0.95;
  float m_cutoffLower = 3, m_cutoffUpper = 3;
  bool m_wholeImageMeanThreshold = false;
  bool m_doSanityCheck = true; //! not needed when called from CaPTk since it does those checks already
  bool m_algorithmDone = false;
};