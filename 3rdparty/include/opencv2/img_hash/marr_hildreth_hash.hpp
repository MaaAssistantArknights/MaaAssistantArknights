// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#ifndef OPENCV_MARR_HILDRETH_HASH_HPP
#define OPENCV_MARR_HILDRETH_HASH_HPP

#include "img_hash_base.hpp"

namespace cv {
namespace img_hash {

//! @addtogroup img_hash
//! @{

/** @brief Marr-Hildreth Operator Based Hash, slowest but more discriminative.

See @cite zauner2010implementation for details.
*/
class CV_EXPORTS_W MarrHildrethHash : public ImgHashBase
{
public:
    /**
     * @brief self explain
     */
    CV_WRAP float getAlpha() const;

    /**
     * @brief self explain
     */
    CV_WRAP float getScale() const;

    /** @brief Set Mh kernel parameters
        @param alpha int scale factor for marr wavelet (default=2).
        @param scale int level of scale factor (default = 1)
    */
    CV_WRAP void setKernelParam(float alpha, float scale);

    /**
        @param alpha int scale factor for marr wavelet (default=2).
        @param scale int level of scale factor (default = 1)
    */
    CV_WRAP static Ptr<MarrHildrethHash> create(float alpha = 2.0f, float scale = 1.0f);
protected:
    MarrHildrethHash() {}
};

/** @brief Computes average hash value of the input image
    @param inputArr input image want to compute hash value,
    type should be CV_8UC4, CV_8UC3, CV_8UC1.
    @param outputArr Hash value of input, it will contain 16 hex
    decimal number, return type is CV_8U
    @param alpha int scale factor for marr wavelet (default=2).
    @param scale int level of scale factor (default = 1)
*/
CV_EXPORTS_W void marrHildrethHash(cv::InputArray inputArr,
                                   cv::OutputArray outputArr,
                                   float alpha = 2.0f, float scale = 1.0f);

//! @}

}} // cv::img_hash::

#endif // OPENCV_MARR_HILDRETH_HASH_HPP
