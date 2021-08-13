/*

By downloading, copying, installing or using the software you agree to this
license. If you do not agree to this license, do not download, install,
copy or use the software.


                          License Agreement
               For Open Source Computer Vision Library
                       (3-clause BSD License)

Copyright (C) 2013, OpenCV Foundation, all rights reserved.
Third party copyrights are property of their respective owners.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  * Neither the names of the copyright holders nor the names of the contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

This software is provided by the copyright holders and contributors "as is" and
any express or implied warranties, including, but not limited to, the implied
warranties of merchantability and fitness for a particular purpose are
disclaimed. In no event shall copyright holders or contributors be liable for
any direct, indirect, incidental, special, exemplary, or consequential damages
(including, but not limited to, procurement of substitute goods or services;
loss of use, data, or profits; or business interruption) however caused
and on any theory of liability, whether in contract, strict liability,
or tort (including negligence or otherwise) arising in any way out of
the use of this software, even if advised of the possibility of such damage.

*/

#ifndef __OPENCV_XOBJDETECT_XOBJDETECT_HPP__
#define __OPENCV_XOBJDETECT_XOBJDETECT_HPP__

#include <opencv2/core.hpp>
#include <vector>
#include <string>

/** @defgroup xobjdetect Extended object detection
*/
namespace cv
{
namespace xobjdetect
{
//! @addtogroup xobjdetect
//! @{


/** @brief WaldBoost detector
*/
class CV_EXPORTS WBDetector {
public:
    /** @brief Read detector from FileNode.
    @param node FileNode for input
    */
    virtual void read(const FileNode &node) = 0;

    /** @brief Write detector to FileStorage.
    @param fs FileStorage for output
    */
    virtual void write(FileStorage &fs) const = 0;

    /** @brief Train WaldBoost detector
    @param pos_samples Path to directory with cropped positive samples
    @param neg_imgs Path to directory with negative (background) images
    */
    virtual void train(
        const std::string& pos_samples,
        const std::string& neg_imgs) = 0;

    /** @brief Detect objects on image using WaldBoost detector
    @param img Input image for detection
    @param bboxes Bounding boxes coordinates output vector
    @param confidences Confidence values for bounding boxes output vector
    */
    virtual void detect(
        const Mat& img,
        std::vector<Rect> &bboxes,
        std::vector<double> &confidences) = 0;

    /** @brief Create instance of WBDetector
    */
    static Ptr<WBDetector> create();

    virtual ~WBDetector(){}
};


//! @}

} /* namespace xobjdetect */
} /* namespace cv */

#endif /* __OPENCV_XOBJDETECT_XOBJDETECT_HPP__ */
