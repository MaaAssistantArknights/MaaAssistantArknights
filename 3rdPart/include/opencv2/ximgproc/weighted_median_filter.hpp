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
// Copyright (C) 2015, The Chinese University of Hong Kong, all rights reserved.
//
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

#ifndef  __OPENCV_WEIGHTED_MEDIAN_FILTER_HPP__
#define  __OPENCV_WEIGHTED_MEDIAN_FILTER_HPP__
#ifdef __cplusplus

/**
* @file
* @date Sept 9, 2015
* @author Zhou Chao
*/

#include <opencv2/core.hpp>
#include <string>

namespace cv
{
namespace ximgproc
{

/**
* @brief Specifies weight types of weighted median filter.
*/
enum WMFWeightType
{
    WMF_EXP = 1     , //!< \f$exp(-|I1-I2|^2/(2*sigma^2))\f$
    WMF_IV1 = 1 << 1, //!< \f$(|I1-I2|+sigma)^-1\f$
    WMF_IV2 = 1 << 2, //!< \f$(|I1-I2|^2+sigma^2)^-1\f$
    WMF_COS = 1 << 3, //!< \f$dot(I1,I2)/(|I1|*|I2|)\f$
    WMF_JAC = 1 << 4, //!< \f$(min(r1,r2)+min(g1,g2)+min(b1,b2))/(max(r1,r2)+max(g1,g2)+max(b1,b2))\f$
    WMF_OFF = 1 << 5  //!< unweighted
};

/**
* @brief   Applies weighted median filter to an image.
*
* For more details about this implementation, please see @cite zhang2014100+
*
* @param   joint       Joint 8-bit, 1-channel or 3-channel image.
* @param   src         Source 8-bit or floating-point, 1-channel or 3-channel image.
* @param   dst         Destination image.
* @param   r           Radius of filtering kernel, should be a positive integer.
* @param   sigma       Filter range standard deviation for the joint image.
* @param   weightType  weightType The type of weight definition, see WMFWeightType
* @param   mask        A 0-1 mask that has the same size with I. This mask is used to ignore the effect of some pixels. If the pixel value on mask is 0,
*                           the pixel will be ignored when maintaining the joint-histogram. This is useful for applications like optical flow occlusion handling.
*
* @sa medianBlur, jointBilateralFilter
*/
CV_EXPORTS_W void weightedMedianFilter(InputArray joint, InputArray src, OutputArray dst,
                                       int r, double sigma = 25.5, int weightType = WMF_EXP, InputArray mask = noArray());
}
}

#endif
#endif
