// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

/*
Ridge Detection Filter.
OpenCV port by : Kushal Vyas (@kushalvyas), Venkatesh Vijaykumar(@venkateshvijaykumar)
Adapted from Niki Estner's explanation of RidgeFilter.
*/

#ifndef __OPENCV_XIMGPROC_RIDGEFILTER_HPP__
#define __OPENCV_XIMGPROC_RIDGEFILTER_HPP__

#include <opencv2/core.hpp>

namespace cv { namespace ximgproc {

//! @addtogroup ximgproc_filters
//! @{

/** @brief  Applies Ridge Detection Filter to an input image.
Implements Ridge detection similar to the one in [Mathematica](http://reference.wolfram.com/language/ref/RidgeFilter.html)
using the eigen values from the Hessian Matrix of the input image using Sobel Derivatives.
Additional refinement can be done using Skeletonization and Binarization. Adapted from @cite segleafvein and @cite M_RF

*/
class CV_EXPORTS_W RidgeDetectionFilter : public Algorithm
{
public:
    /**
    @brief Create pointer to the Ridge detection filter.
    @param ddepth  Specifies output image depth. Defualt is CV_32FC1
    @param dx Order of derivative x, default is 1
    @param dy  Order of derivative y, default is 1
    @param ksize Sobel kernel size , default is 3
    @param out_dtype Converted format for output, default is CV_8UC1
    @param scale Optional scale value for derivative values, default is 1
    @param delta  Optional bias added to output, default is 0
    @param borderType Pixel extrapolation method, default is BORDER_DEFAULT
    @see Sobel, threshold, getStructuringElement, morphologyEx.( for additional refinement)
    */
    CV_WRAP static Ptr<RidgeDetectionFilter> create(int ddepth = CV_32FC1, int dx=1, int dy=1, int ksize = 3, int out_dtype=CV_8UC1, double scale = 1, double delta = 0, int borderType = BORDER_DEFAULT);
    /**
    @brief Apply Ridge detection filter on input image.
    @param _img InputArray as supported by Sobel. img can be 1-Channel or 3-Channels.
    @param out OutputAray of structure as RidgeDetectionFilter::ddepth. Output image with ridges.
    */
    CV_WRAP virtual void getRidgeFilteredImage(InputArray _img, OutputArray out) = 0;
};

//! @}
}} // namespace
#endif
