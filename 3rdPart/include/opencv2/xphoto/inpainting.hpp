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

#ifndef __OPENCV_INPAINTING_HPP__
#define __OPENCV_INPAINTING_HPP__

/** @file
@date Jul 22, 2014
@author Yury Gitman
*/

#include <opencv2/core.hpp>

namespace cv
{
namespace xphoto
{

//! @addtogroup xphoto
//! @{

    //! various inpainting algorithms
    enum InpaintTypes
    {
        /** This algorithm searches for dominant correspondences (transformations) of
        image patches and tries to seamlessly fill-in the area to be inpainted using this
        transformations */
        INPAINT_SHIFTMAP = 0
    };

    /** @brief The function implements different single-image inpainting algorithms.

    See the original paper @cite He2012 for details.

    @param src source image, it could be of any type and any number of channels from 1 to 4. In case of
    3- and 4-channels images the function expect them in CIELab colorspace or similar one, where first
    color component shows intensity, while second and third shows colors. Nonetheless you can try any
    colorspaces.
    @param mask mask (CV_8UC1), where non-zero pixels indicate valid image area, while zero pixels
    indicate area to be inpainted
    @param dst destination image
    @param algorithmType see xphoto::InpaintTypes
    */
    CV_EXPORTS_W void inpaint(const Mat &src, const Mat &mask, Mat &dst, const int algorithmType);

//! @}

}
}

#endif // __OPENCV_INPAINTING_HPP__
