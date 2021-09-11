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
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009-2011, Willow Garage Inc., all rights reserved.
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

#ifndef __OPENCV_SIMPLE_COLOR_BALANCE_HPP__
#define __OPENCV_SIMPLE_COLOR_BALANCE_HPP__

/** @file
@date Jun 26, 2014
@author Yury Gitman
*/

#include <opencv2/core.hpp>

namespace cv
{
namespace xphoto
{

//! @addtogroup xphoto
//! @{

/** @brief The base class for auto white balance algorithms.
 */
class CV_EXPORTS_W WhiteBalancer : public Algorithm
{
  public:
    /** @brief Applies white balancing to the input image

    @param src Input image
    @param dst White balancing result
    @sa cvtColor, equalizeHist
    */
    CV_WRAP virtual void balanceWhite(InputArray src, OutputArray dst) = 0;
};

/** @brief A simple white balance algorithm that works by independently stretching
    each of the input image channels to the specified range. For increased robustness
    it ignores the top and bottom \f$p\%\f$ of pixel values.
 */
class CV_EXPORTS_W SimpleWB : public WhiteBalancer
{
  public:
    /** @brief Input image range minimum value
    @see setInputMin */
    CV_WRAP virtual float getInputMin() const = 0;
    /** @copybrief getInputMin @see getInputMin */
    CV_WRAP virtual void setInputMin(float val) = 0;

    /** @brief Input image range maximum value
    @see setInputMax */
    CV_WRAP virtual float getInputMax() const = 0;
    /** @copybrief getInputMax @see getInputMax */
    CV_WRAP virtual void setInputMax(float val) = 0;

    /** @brief Output image range minimum value
    @see setOutputMin */
    CV_WRAP virtual float getOutputMin() const = 0;
    /** @copybrief getOutputMin @see getOutputMin */
    CV_WRAP virtual void setOutputMin(float val) = 0;

    /** @brief Output image range maximum value
    @see setOutputMax */
    CV_WRAP virtual float getOutputMax() const = 0;
    /** @copybrief getOutputMax @see getOutputMax */
    CV_WRAP virtual void setOutputMax(float val) = 0;

    /** @brief Percent of top/bottom values to ignore
    @see setP */
    CV_WRAP virtual float getP() const = 0;
    /** @copybrief getP @see getP */
    CV_WRAP virtual void setP(float val) = 0;
};

/** @brief Creates an instance of SimpleWB
 */
CV_EXPORTS_W Ptr<SimpleWB> createSimpleWB();

/** @brief Gray-world white balance algorithm

This algorithm scales the values of pixels based on a
gray-world assumption which states that the average of all channels
should result in a gray image.

It adds a modification which thresholds pixels based on their
saturation value and only uses pixels below the provided threshold in
finding average pixel values.

Saturation is calculated using the following for a 3-channel RGB image per
pixel I and is in the range [0, 1]:

\f[ \texttt{Saturation} [I] = \frac{\textrm{max}(R,G,B) - \textrm{min}(R,G,B)
}{\textrm{max}(R,G,B)} \f]

A threshold of 1 means that all pixels are used to white-balance, while a
threshold of 0 means no pixels are used. Lower thresholds are useful in
white-balancing saturated images.

Currently supports images of type @ref CV_8UC3 and @ref CV_16UC3.
 */
class CV_EXPORTS_W GrayworldWB : public WhiteBalancer
{
  public:
    /** @brief Maximum saturation for a pixel to be included in the
        gray-world assumption
    @see setSaturationThreshold */
    CV_WRAP virtual float getSaturationThreshold() const = 0;
    /** @copybrief getSaturationThreshold @see getSaturationThreshold */
    CV_WRAP virtual void setSaturationThreshold(float val) = 0;
};

/** @brief Creates an instance of GrayworldWB
 */
CV_EXPORTS_W Ptr<GrayworldWB> createGrayworldWB();

/** @brief More sophisticated learning-based automatic white balance algorithm.

As @ref GrayworldWB, this algorithm works by applying different gains to the input
image channels, but their computation is a bit more involved compared to the
simple gray-world assumption. More details about the algorithm can be found in
@cite Cheng2015 .

To mask out saturated pixels this function uses only pixels that satisfy the
following condition:

\f[ \frac{\textrm{max}(R,G,B)}{\texttt{range_max_val}} < \texttt{saturation_thresh} \f]

Currently supports images of type @ref CV_8UC3 and @ref CV_16UC3.
 */
class CV_EXPORTS_W LearningBasedWB : public WhiteBalancer
{
  public:
    /** @brief Implements the feature extraction part of the algorithm.

    In accordance with @cite Cheng2015 , computes the following features for the input image:
    1. Chromaticity of an average (R,G,B) tuple
    2. Chromaticity of the brightest (R,G,B) tuple (while ignoring saturated pixels)
    3. Chromaticity of the dominant (R,G,B) tuple (the one that has the highest value in the RGB histogram)
    4. Mode of the chromaticity palette, that is constructed by taking 300 most common colors according to
       the RGB histogram and projecting them on the chromaticity plane. Mode is the most high-density point
       of the palette, which is computed by a straightforward fixed-bandwidth kernel density estimator with
       a Epanechnikov kernel function.

    @param src Input three-channel image (BGR color space is assumed).
    @param dst An array of four (r,g) chromaticity tuples corresponding to the features listed above.
    */
    CV_WRAP virtual void extractSimpleFeatures(InputArray src, OutputArray dst) = 0;

    /** @brief Maximum possible value of the input image (e.g. 255 for 8 bit images,
               4095 for 12 bit images)
    @see setRangeMaxVal */
    CV_WRAP virtual int getRangeMaxVal() const = 0;
    /** @copybrief getRangeMaxVal @see getRangeMaxVal */
    CV_WRAP virtual void setRangeMaxVal(int val) = 0;

    /** @brief Threshold that is used to determine saturated pixels, i.e. pixels where at least one of the
        channels exceeds \f$\texttt{saturation_threshold}\times\texttt{range_max_val}\f$ are ignored.
    @see setSaturationThreshold */
    CV_WRAP virtual float getSaturationThreshold() const = 0;
    /** @copybrief getSaturationThreshold @see getSaturationThreshold */
    CV_WRAP virtual void setSaturationThreshold(float val) = 0;

    /** @brief Defines the size of one dimension of a three-dimensional RGB histogram that is used internally
        by the algorithm. It often makes sense to increase the number of bins for images with higher bit depth
        (e.g. 256 bins for a 12 bit image).
    @see setHistBinNum */
    CV_WRAP virtual int getHistBinNum() const = 0;
    /** @copybrief getHistBinNum @see getHistBinNum */
    CV_WRAP virtual void setHistBinNum(int val) = 0;
};

/** @brief Creates an instance of LearningBasedWB

@param path_to_model Path to a .yml file with the model. If not specified, the default model is used
 */
CV_EXPORTS_W Ptr<LearningBasedWB> createLearningBasedWB(const String& path_to_model = String());

/** @brief Implements an efficient fixed-point approximation for applying channel gains, which is
    the last step of multiple white balance algorithms.

@param src Input three-channel image in the BGR color space (either CV_8UC3 or CV_16UC3)
@param dst Output image of the same size and type as src.
@param gainB gain for the B channel
@param gainG gain for the G channel
@param gainR gain for the R channel
*/
CV_EXPORTS_W void applyChannelGains(InputArray src, OutputArray dst, float gainB, float gainG, float gainR);
//! @}
}
}

#endif // __OPENCV_SIMPLE_COLOR_BALANCE_HPP__
