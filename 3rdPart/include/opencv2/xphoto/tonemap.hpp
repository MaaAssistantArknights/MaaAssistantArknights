// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#ifndef OPENCV_XPHOTO_TONEMAP_HPP
#define OPENCV_XPHOTO_TONEMAP_HPP

#include "opencv2/photo.hpp"

namespace cv { namespace xphoto {

//! @addtogroup xphoto
//! @{

/** @brief This algorithm decomposes image into two layers: base layer and detail layer using bilateral filter
and compresses contrast of the base layer thus preserving all the details.

This implementation uses regular bilateral filter from OpenCV.

Saturation enhancement is possible as in cv::TonemapDrago.

For more information see @cite DD02 .
 */
class CV_EXPORTS_W TonemapDurand : public Tonemap
{
public:

    CV_WRAP virtual float getSaturation() const = 0;
    CV_WRAP virtual void setSaturation(float saturation) = 0;

    CV_WRAP virtual float getContrast() const = 0;
    CV_WRAP virtual void setContrast(float contrast) = 0;

    CV_WRAP virtual float getSigmaSpace() const = 0;
    CV_WRAP virtual void setSigmaSpace(float sigma_space) = 0;

    CV_WRAP virtual float getSigmaColor() const = 0;
    CV_WRAP virtual void setSigmaColor(float sigma_color) = 0;
};

/** @brief Creates TonemapDurand object

You need to set the OPENCV_ENABLE_NONFREE option in cmake to use those. Use them at your own risk.

@param gamma gamma value for gamma correction. See createTonemap
@param contrast resulting contrast on logarithmic scale, i. e. log(max / min), where max and min
are maximum and minimum luminance values of the resulting image.
@param saturation saturation enhancement value. See createTonemapDrago
@param sigma_color bilateral filter sigma in color space
@param sigma_space bilateral filter sigma in coordinate space
 */
CV_EXPORTS_W Ptr<TonemapDurand>
createTonemapDurand(float gamma = 1.0f, float contrast = 4.0f, float saturation = 1.0f, float sigma_color = 2.0f, float sigma_space = 2.0f);

}} // namespace
#endif  // OPENCV_XPHOTO_TONEMAP_HPP
