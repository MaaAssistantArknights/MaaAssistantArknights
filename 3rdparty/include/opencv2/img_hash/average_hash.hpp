// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#ifndef OPENCV_AVERAGE_HASH_HPP
#define OPENCV_AVERAGE_HASH_HPP

#include "img_hash_base.hpp"

namespace cv {
namespace img_hash {

//! @addtogroup img_hash
//! @{

/** @brief Computes average hash value of the input image

This is a fast image hashing algorithm, but only work on simple case. For more details, please
refer to @cite lookslikeit
*/
class CV_EXPORTS_W AverageHash : public ImgHashBase
{
public:
    CV_WRAP static Ptr<AverageHash> create();
protected:
    AverageHash() {}
};

/** @brief Calculates img_hash::AverageHash in one call
@param inputArr input image want to compute hash value, type should be CV_8UC4, CV_8UC3 or CV_8UC1.
@param outputArr Hash value of input, it will contain 16 hex decimal number, return type is CV_8U
*/
CV_EXPORTS_W void averageHash(cv::InputArray inputArr, cv::OutputArray outputArr);

//! @}

}} // cv::img_hash::

#endif // OPENCV_AVERAGE_HASH_HPP
