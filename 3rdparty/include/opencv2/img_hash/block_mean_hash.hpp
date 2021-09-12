// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#ifndef OPENCV_BLOCK_MEAN_HASH_HPP
#define OPENCV_BLOCK_MEAN_HASH_HPP

#include "img_hash_base.hpp"

namespace cv {
namespace img_hash {

//! @addtogroup img_hash
//! @{

enum BlockMeanHashMode
{
    BLOCK_MEAN_HASH_MODE_0 = 0, //!< use fewer block and generate 16*16/8 uchar hash value
    BLOCK_MEAN_HASH_MODE_1 = 1, //!< use block blocks(step sizes/2), generate 31*31/8 + 1 uchar hash value
};

/** @brief Image hash based on block mean.

See @cite zauner2010implementation for details.
*/
class CV_EXPORTS_W BlockMeanHash : public ImgHashBase
{
public:
    /** @brief Create BlockMeanHash object
        @param mode the mode
    */
    CV_WRAP void setMode(int mode);
    CV_WRAP std::vector<double> getMean() const;
    CV_WRAP static Ptr<BlockMeanHash> create(int mode = BLOCK_MEAN_HASH_MODE_0);
protected:
    BlockMeanHash() {}
};

/** @brief Computes block mean hash of the input image
    @param inputArr input image want to compute hash value, type should be CV_8UC4, CV_8UC3 or CV_8UC1.
    @param outputArr Hash value of input, it will contain 16 hex decimal number, return type is CV_8U
    @param mode the mode
*/
CV_EXPORTS_W void blockMeanHash(cv::InputArray inputArr,
                                cv::OutputArray outputArr,
                                int mode = BLOCK_MEAN_HASH_MODE_0);

//! @}

}} // cv::img_hash::

#endif // OPENCV_BLOCK_MEAN_HASH_HPP
