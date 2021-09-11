// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#ifndef OPENCV_IMG_HASH_BASE_HPP
#define OPENCV_IMG_HASH_BASE_HPP

#include "opencv2/core.hpp"

namespace cv {
namespace img_hash {

//! @addtogroup img_hash
//! @{

/** @brief The base class for image hash algorithms
 */
class CV_EXPORTS_W ImgHashBase : public Algorithm
{
public:
    class ImgHashImpl;

    ~ImgHashBase();
    /** @brief Computes hash of the input image
        @param inputArr input image want to compute hash value
        @param outputArr hash of the image
    */
    CV_WRAP void compute(cv::InputArray inputArr, cv::OutputArray outputArr);
    /** @brief Compare the hash value between inOne and inTwo
        @param hashOne Hash value one
        @param hashTwo Hash value two
        @return value indicate similarity between inOne and inTwo, the meaning
        of the value vary from algorithms to algorithms
    */
    CV_WRAP double compare(cv::InputArray hashOne, cv::InputArray hashTwo) const;
protected:
    ImgHashBase();
protected:
    Ptr<ImgHashImpl> pImpl;
};

//! @}

} } // cv::img_hash::

#endif // OPENCV_IMG_HASH_BASE_HPP
