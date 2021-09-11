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

#ifndef __OPENCV_FUZZY_F1_MATH_H__
#define __OPENCV_FUZZY_F1_MATH_H__

#include "opencv2/fuzzy/types.hpp"
#include "opencv2/core.hpp"

namespace cv
{

namespace ft
{
    //! @addtogroup f1_math
    //! @{

    /** @brief Computes components of the array using direct \f$F^1\f$-transform.
    @param matrix Input array.
    @param kernel Kernel used for processing. Function `ft::createKernel` can be used.
    @param components Output 32-bit float array for the components.

    The function computes linear components using predefined kernel.
    */
    CV_EXPORTS_W void FT12D_components(InputArray matrix, InputArray kernel, OutputArray components);

    /** @brief Computes elements of \f$F^1\f$-transform components.
    @param matrix Input array.
    @param kernel Kernel used for processing. Function `ft::createKernel` can be used.
    @param c00 Elements represent average color.
    @param c10 Elements represent average vertical gradient.
    @param c01 Elements represent average horizontal gradient.
    @param components Output 32-bit float array for the components.
    @param mask Mask can be used for unwanted area marking.

    The function computes components and its elements using predefined kernel and mask.
    */
    CV_EXPORTS_W void FT12D_polynomial(InputArray matrix, InputArray kernel, OutputArray c00, OutputArray c10, OutputArray c01, OutputArray components, InputArray mask = noArray());

    /** @brief Creates vertical matrix for \f$F^1\f$-transform computation.
    @param radius Radius of the basic function.
    @param matrix The vertical matrix.
    @param chn Number of channels.

    The function creates helper vertical matrix for \f$F^1\f$-transfrom processing. It is used for gradient computation.
    */
    CV_EXPORTS_W void FT12D_createPolynomMatrixVertical(int radius, OutputArray matrix, const int chn);

    /** @brief Creates horizontal matrix for \f$F^1\f$-transform computation.
    @param radius Radius of the basic function.
    @param matrix The horizontal matrix.
    @param chn Number of channels.

    The function creates helper horizontal matrix for \f$F^1\f$-transfrom processing. It is used for gradient computation.
    */
    CV_EXPORTS_W void FT12D_createPolynomMatrixHorizontal(int radius, OutputArray matrix, const int chn);

    /** @brief Computes \f$F^1\f$-transfrom and inverse \f$F^1\f$-transfrom at once.
    @param matrix Input matrix.
    @param kernel Kernel used for processing. Function `ft::createKernel` can be used.
    @param output Output 32-bit float array.
    @param mask Mask used for unwanted area marking.

    This function computes \f$F^1\f$-transfrom and inverse \f$F^1\f$-transfotm in one step. It is fully sufficient and optimized for `cv::Mat`.

    @note
        F-transform technique of first degreee is described in paper @cite Vlas:FT.
    */
    CV_EXPORTS_W void FT12D_process(InputArray matrix, InputArray kernel, OutputArray output, InputArray mask = noArray());

    /** @brief Computes inverse \f$F^1\f$-transfrom.
    @param components Input 32-bit float single channel array for the components.
    @param kernel Kernel used for processing. The same kernel as for components computation must be used.
    @param output Output 32-bit float array.
    @param width Width of the output array.
    @param height Height of the output array.

    Computation of inverse \f$F^1\f$-transform.
    */
    CV_EXPORTS_W void FT12D_inverseFT(InputArray components, InputArray kernel, OutputArray output, int width, int height);

    //! @}
}
}

#endif // __OPENCV_FUZZY_F1_MATH_H__
