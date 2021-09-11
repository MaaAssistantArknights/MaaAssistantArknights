// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#ifndef OPENCV_COLOR_MOMENT_HASH_HPP
#define OPENCV_COLOR_MOMENT_HASH_HPP

#include "img_hash_base.hpp"

namespace cv {
namespace img_hash {

//! @addtogroup img_hash
//! @{

/** @brief Image hash based on color moments.

See @cite tang2012perceptual for details.
*/
class CV_EXPORTS_W ColorMomentHash : public ImgHashBase
{
public:
    CV_WRAP static Ptr<ColorMomentHash> create();
protected:
    ColorMomentHash() {}
};

/** @brief Computes color moment hash of the input, the algorithm
    is come from the paper "Perceptual  Hashing  for  Color  Images
    Using  Invariant Moments"
    @param inputArr input image want to compute hash value,
    type should be CV_8UC4, CV_8UC3 or CV_8UC1.
    @param outputArr 42 hash values with type CV_64F(double)
     */
CV_EXPORTS_W void colorMomentHash(cv::InputArray inputArr, cv::OutputArray outputArr);

//! @}

}} // cv::img_hash::

#endif // OPENCV_COLOR_MOMENT_HASH_HPP
