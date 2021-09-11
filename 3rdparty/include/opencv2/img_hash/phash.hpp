// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#ifndef OPENCV_PHASH_HPP
#define OPENCV_PHASH_HPP

#include "img_hash_base.hpp"

namespace cv {
namespace img_hash {

//! @addtogroup img_hash
//! @{

/** @brief pHash

Slower than average_hash, but tolerant of minor modifications

This algorithm can combat more variation than averageHash, for more details please refer to @cite lookslikeit
*/
class CV_EXPORTS_W PHash : public ImgHashBase
{
public:
    CV_WRAP static Ptr<PHash> create();
protected:
    PHash() {}
};

/** @brief Computes pHash value of the input image
    @param inputArr input image want to compute hash value,
     type should be CV_8UC4, CV_8UC3, CV_8UC1.
    @param outputArr Hash value of input, it will contain 8 uchar value
*/
CV_EXPORTS_W void pHash(cv::InputArray inputArr, cv::OutputArray outputArr);

//! @}

} } // cv::img_hash::

#endif // OPENCV_PHASH_HPP
