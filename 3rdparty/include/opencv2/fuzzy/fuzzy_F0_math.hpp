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

#ifndef __OPENCV_FUZZY_F0_MATH_H__
#define __OPENCV_FUZZY_F0_MATH_H__

#include "opencv2/fuzzy/types.hpp"
#include "opencv2/core.hpp"

namespace cv
{

namespace ft
{
    //! @addtogroup f0_math
    //! @{

    /** @brief Computes components of the array using direct \f$F^0\f$-transform.
    @param matrix Input array.
    @param kernel Kernel used for processing. Function `ft::createKernel` can be used.
    @param components Output 32-bit float array for the components.
    @param mask Mask can be used for unwanted area marking.

    The function computes components using predefined kernel and mask.
     */
    CV_EXPORTS_W void FT02D_components(InputArray matrix, InputArray kernel, OutputArray components, InputArray mask = noArray());

    /** @brief Computes inverse \f$F^0\f$-transfrom.
    @param components Input 32-bit float single channel array for the components.
    @param kernel Kernel used for processing. Function `ft::createKernel` can be used.
    @param output Output 32-bit float array.
    @param width Width of the output array.
    @param height Height of the output array.

    Computation of inverse F-transform.
     */
    CV_EXPORTS_W void FT02D_inverseFT(InputArray components, InputArray kernel, OutputArray output, int width, int height);

    /** @brief Computes \f$F^0\f$-transfrom and inverse \f$F^0\f$-transfrom at once.
    @param matrix Input matrix.
    @param kernel Kernel used for processing. Function `ft::createKernel` can be used.
    @param output Output 32-bit float array.
    @param mask Mask used for unwanted area marking.

    This function computes F-transfrom and inverse F-transfotm in one step. It is fully sufficient and optimized for `cv::Mat`.
    */
    CV_EXPORTS_W void FT02D_process(InputArray matrix, InputArray kernel, OutputArray output, InputArray mask = noArray());

    /** @brief Computes \f$F^0\f$-transfrom and inverse \f$F^0\f$-transfrom at once and return state.
    @param matrix Input matrix.
    @param kernel Kernel used for processing. Function `ft::createKernel` can be used.
    @param output Output 32-bit float array.
    @param mask Mask used for unwanted area marking.
    @param maskOutput Mask after one iteration.
    @param firstStop If **true** function returns -1 when first problem appears. In case of `false` the process is completed and summation of all problems returned.

    This function computes iteration of F-transfrom and inverse F-transfotm and handle image and mask change. The function is used in `ft::inpaint` function.
    */
    CV_EXPORTS_W int FT02D_iteration(InputArray matrix, InputArray kernel, OutputArray output, InputArray mask, OutputArray maskOutput, bool firstStop);

    /** @brief Sligtly less accurate version of \f$F^0\f$-transfrom computation optimized for higher speed. The methods counts with linear basic function.
    @param matrix Input 3 channels matrix.
    @param radius Radius of the `ft::LINEAR` basic function.
    @param output Output array.

    This function computes F-transfrom and inverse F-transfotm using linear basic function in one step. It is ~10 times faster than `ft::FT02D_process` method.
    */
    CV_EXPORTS_W void FT02D_FL_process(InputArray matrix, const int radius, OutputArray output);

    /** @brief Sligtly less accurate version of \f$F^0\f$-transfrom computation optimized for higher speed. The methods counts with linear basic function.
    @param matrix Input 3 channels matrix.
    @param radius Radius of the `ft::LINEAR` basic function.
    @param output Output array.

    This function computes F-transfrom and inverse F-transfotm using linear basic function in one step. It is ~9 times faster then `ft::FT02D_process` method and more accurate than `ft::FT02D_FL_process` method.
    */
    CV_EXPORTS_W void FT02D_FL_process_float(InputArray matrix, const int radius, OutputArray output);

    //! @}
}
}

#endif // __OPENCV_FUZZY_F0_MATH_H__
