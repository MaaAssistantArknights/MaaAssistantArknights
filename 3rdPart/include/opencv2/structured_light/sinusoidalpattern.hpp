/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2015, OpenCV Foundation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef __OPENCV_SINUSOIDAL_PATTERN_HPP__
#define __OPENCV_SINUSOIDAL_PATTERN_HPP__

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/structured_light/structured_light.hpp"
#include <opencv2/phase_unwrapping.hpp>
#include <opencv2/calib3d.hpp>

namespace cv {
namespace structured_light {
//! @addtogroup structured_light
//! @{

 //! Type of sinusoidal pattern profilometry methods.
enum{
  FTP = 0,
  PSP = 1,
  FAPS = 2
 };
/**
 * @brief Class implementing Fourier transform profilometry (FTP) , phase-shifting profilometry (PSP)
 * and Fourier-assisted phase-shifting profilometry (FAPS) based on @cite faps.

 * This class generates sinusoidal patterns that can be used with FTP, PSP and FAPS.
*/
class CV_EXPORTS_W SinusoidalPattern : public StructuredLightPattern
{
public:
    /**
     * @brief Parameters of SinusoidalPattern constructor
     * @param width Projector's width.
     * @param height Projector's height.
     * @param nbrOfPeriods Number of period along the patterns direction.
     * @param shiftValue Phase shift between two consecutive patterns.
     * @param methodId Allow to choose between FTP, PSP and FAPS.
     * @param nbrOfPixelsBetweenMarkers Number of pixels between two consecutive markers on the same row.
     * @param setMarkers Allow to set markers on the patterns.
     * @param markersLocation vector used to store markers location on the patterns.
     */
    struct CV_EXPORTS_W Params
    {
        CV_WRAP Params();
        CV_PROP_RW int width;
        CV_PROP_RW int height;
        CV_PROP_RW int nbrOfPeriods;
        CV_PROP_RW float shiftValue;
        CV_PROP_RW int methodId;
        CV_PROP_RW int nbrOfPixelsBetweenMarkers;
        CV_PROP_RW bool horizontal;
        CV_PROP_RW bool setMarkers;
        std::vector<Point2f> markersLocation;
    };
    /**
     * @brief Constructor.
     * @param parameters SinusoidalPattern parameters SinusoidalPattern::Params: width, height of the projector and patterns parameters.
     *
     */
    CV_WRAP static Ptr<SinusoidalPattern> create( Ptr<SinusoidalPattern::Params> parameters =
                                          makePtr<SinusoidalPattern::Params>() );
    /**
     * @brief Compute a wrapped phase map from sinusoidal patterns.
     * @param patternImages Input data to compute the wrapped phase map.
     * @param wrappedPhaseMap Wrapped phase map obtained through one of the three methods.
     * @param shadowMask Mask used to discard shadow regions.
     * @param fundamental Fundamental matrix used to compute epipolar lines and ease the matching step.
     */
    CV_WRAP
    virtual void computePhaseMap( InputArrayOfArrays patternImages,
                                  OutputArray wrappedPhaseMap,
                                  OutputArray shadowMask = noArray(),
                                  InputArray fundamental = noArray()) = 0;
    /**
     * @brief Unwrap the wrapped phase map to remove phase ambiguities.
     * @param wrappedPhaseMap The wrapped phase map computed from the pattern.
     * @param unwrappedPhaseMap The unwrapped phase map used to find correspondences between the two devices.
     * @param camSize Resolution of the camera.
     * @param shadowMask Mask used to discard shadow regions.
     */
    CV_WRAP
    virtual void unwrapPhaseMap( InputArray wrappedPhaseMap,
                                 OutputArray unwrappedPhaseMap,
                                 cv::Size camSize,
                                 InputArray shadowMask = noArray() ) = 0;
    /**
     * @brief Find correspondences between the two devices thanks to unwrapped phase maps.
     * @param projUnwrappedPhaseMap Projector's unwrapped phase map.
     * @param camUnwrappedPhaseMap Camera's unwrapped phase map.
     * @param matches Images used to display correspondences map.
     */
    CV_WRAP
    virtual void findProCamMatches( InputArray projUnwrappedPhaseMap, InputArray camUnwrappedPhaseMap,
                                    OutputArrayOfArrays matches ) = 0;

    /**
     * @brief compute the data modulation term.
     * @param patternImages captured images with projected patterns.
     * @param dataModulationTerm Mat where the data modulation term is saved.
     * @param shadowMask Mask used to discard shadow regions.
     */
    CV_WRAP
    virtual void computeDataModulationTerm( InputArrayOfArrays patternImages,
                                            OutputArray dataModulationTerm,
                                            InputArray shadowMask ) = 0;

};
//! @}
}
}
#endif
