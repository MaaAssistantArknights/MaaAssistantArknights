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
// Copyright (C) 2015, University of Ostrava, Institute for Research and Applications of Fuzzy Modeling,
// Pavel Vlasanek, all rights reserved. Third party copyrights are property of their respective owners.
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

#ifndef __OPENCV_FUZZY_IMAGE_H__
#define __OPENCV_FUZZY_IMAGE_H__

#include "types.hpp"
#include "opencv2/core.hpp"

namespace cv
{

namespace ft
{
    //! @addtogroup f_image
    //! @{

    /** @brief Creates kernel from basic functions.
    @param A Basic function used in axis **x**.
    @param B Basic function used in axis **y**.
    @param kernel Final 32-bit kernel derived from **A** and **B**.
    @param chn Number of kernel channels.

    The function creates kernel usable for latter fuzzy image processing.
    */
    CV_EXPORTS_AS(createKernel1) void createKernel(InputArray A, InputArray B, OutputArray kernel, const int chn);

    /** @brief Creates kernel from general functions.
    @param function Function type could be one of the following:
        -   **LINEAR** Linear basic function.
    @param radius Radius of the basic function.
    @param kernel Final 32-bit kernel.
    @param chn Number of kernel channels.

    The function creates kernel from predefined functions.
    */
    CV_EXPORTS_W void createKernel(int function, int radius, OutputArray kernel, const int chn);

    /** @brief Image inpainting
    @param image Input image.
    @param mask Mask used for unwanted area marking.
    @param output Output 32-bit image.
    @param radius Radius of the basic function.
    @param function Function type could be one of the following:
        -   `ft::LINEAR` Linear basic function.
    @param algorithm Algorithm could be one of the following:
        -   `ft::ONE_STEP` One step algorithm.
        -   `ft::MULTI_STEP` This algorithm automaticaly increases radius of the basic function.
        -   `ft::ITERATIVE` Iterative algorithm running in more steps using partial computations.

    This function provides inpainting technique based on the fuzzy mathematic.

    @note
        The algorithms are described in paper @cite Perf:rec.
    */
    CV_EXPORTS_W void inpaint(InputArray image, InputArray mask, OutputArray output, int radius, int function, int algorithm);

    /** @brief Image filtering
    @param image Input image.
    @param kernel Final 32-bit kernel.
    @param output Output 32-bit image.

    Filtering of the input image by means of F-transform.
    */
    CV_EXPORTS_W void filter(InputArray image, InputArray kernel, OutputArray output);

    //! @}
}
}

#endif // __OPENCV_FUZZY_IMAGE_H__
