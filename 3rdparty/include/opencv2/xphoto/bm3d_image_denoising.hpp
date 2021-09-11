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

#ifndef __OPENCV_BM3D_IMAGE_DENOISING_HPP__
#define __OPENCV_BM3D_IMAGE_DENOISING_HPP__

/** @file
@date Jul 19, 2016
@author Bartek Pawlik
*/

#include <opencv2/core.hpp>

namespace cv
{
    namespace xphoto
    {
        //! @addtogroup xphoto
        //! @{

        //! BM3D transform types
        enum TransformTypes
        {
            /** Un-normalized Haar transform */
            HAAR = 0
        };

        //! BM3D algorithm steps
        enum Bm3dSteps
        {
            /** Execute all steps of the algorithm */
            BM3D_STEPALL = 0,
            /** Execute only first step of the algorithm */
            BM3D_STEP1 = 1,
            /** Execute only second step of the algorithm */
            BM3D_STEP2 = 2
        };

        /** @brief Performs image denoising using the Block-Matching and 3D-filtering algorithm
        <http://www.cs.tut.fi/~foi/GCF-BM3D/BM3D_TIP_2007.pdf> with several computational
        optimizations. Noise expected to be a gaussian white noise.

        @param src Input 8-bit or 16-bit 1-channel image.
        @param dstStep1 Output image of the first step of BM3D with the same size and type as src.
        @param dstStep2 Output image of the second step of BM3D with the same size and type as src.
        @param h Parameter regulating filter strength. Big h value perfectly removes noise but also
        removes image details, smaller h value preserves details but also preserves some noise.
        @param templateWindowSize Size in pixels of the template patch that is used for block-matching.
        Should be power of 2.
        @param searchWindowSize Size in pixels of the window that is used to perform block-matching.
        Affect performance linearly: greater searchWindowsSize - greater denoising time.
        Must be larger than templateWindowSize.
        @param blockMatchingStep1 Block matching threshold for the first step of BM3D (hard thresholding),
        i.e. maximum distance for which two blocks are considered similar.
        Value expressed in euclidean distance.
        @param blockMatchingStep2 Block matching threshold for the second step of BM3D (Wiener filtering),
        i.e. maximum distance for which two blocks are considered similar.
        Value expressed in euclidean distance.
        @param groupSize Maximum size of the 3D group for collaborative filtering.
        @param slidingStep Sliding step to process every next reference block.
        @param beta Kaiser window parameter that affects the sidelobe attenuation of the transform of the
        window. Kaiser window is used in order to reduce border effects. To prevent usage of the window,
        set beta to zero.
        @param normType Norm used to calculate distance between blocks. L2 is slower than L1
        but yields more accurate results.
        @param step Step of BM3D to be executed. Possible variants are: step 1, step 2, both steps.
        @param transformType Type of the orthogonal transform used in collaborative filtering step.
        Currently only Haar transform is supported.

        This function expected to be applied to grayscale images. Advanced usage of this function
        can be manual denoising of colored image in different colorspaces.

        @sa
        fastNlMeansDenoising
        */
        CV_EXPORTS_W void bm3dDenoising(
            InputArray src,
            InputOutputArray dstStep1,
            OutputArray dstStep2,
            float h = 1,
            int templateWindowSize = 4,
            int searchWindowSize = 16,
            int blockMatchingStep1 = 2500,
            int blockMatchingStep2 = 400,
            int groupSize = 8,
            int slidingStep = 1,
            float beta = 2.0f,
            int normType = cv::NORM_L2,
            int step = cv::xphoto::BM3D_STEPALL,
            int transformType = cv::xphoto::HAAR);

        /** @brief Performs image denoising using the Block-Matching and 3D-filtering algorithm
        <http://www.cs.tut.fi/~foi/GCF-BM3D/BM3D_TIP_2007.pdf> with several computational
        optimizations. Noise expected to be a gaussian white noise.

        @param src Input 8-bit or 16-bit 1-channel image.
        @param dst Output image with the same size and type as src.
        @param h Parameter regulating filter strength. Big h value perfectly removes noise but also
        removes image details, smaller h value preserves details but also preserves some noise.
        @param templateWindowSize Size in pixels of the template patch that is used for block-matching.
        Should be power of 2.
        @param searchWindowSize Size in pixels of the window that is used to perform block-matching.
        Affect performance linearly: greater searchWindowsSize - greater denoising time.
        Must be larger than templateWindowSize.
        @param blockMatchingStep1 Block matching threshold for the first step of BM3D (hard thresholding),
        i.e. maximum distance for which two blocks are considered similar.
        Value expressed in euclidean distance.
        @param blockMatchingStep2 Block matching threshold for the second step of BM3D (Wiener filtering),
        i.e. maximum distance for which two blocks are considered similar.
        Value expressed in euclidean distance.
        @param groupSize Maximum size of the 3D group for collaborative filtering.
        @param slidingStep Sliding step to process every next reference block.
        @param beta Kaiser window parameter that affects the sidelobe attenuation of the transform of the
        window. Kaiser window is used in order to reduce border effects. To prevent usage of the window,
        set beta to zero.
        @param normType Norm used to calculate distance between blocks. L2 is slower than L1
        but yields more accurate results.
        @param step Step of BM3D to be executed. Allowed are only BM3D_STEP1 and BM3D_STEPALL.
        BM3D_STEP2 is not allowed as it requires basic estimate to be present.
        @param transformType Type of the orthogonal transform used in collaborative filtering step.
        Currently only Haar transform is supported.

        This function expected to be applied to grayscale images. Advanced usage of this function
        can be manual denoising of colored image in different colorspaces.

        @sa
        fastNlMeansDenoising
        */
        CV_EXPORTS_W void bm3dDenoising(
            InputArray src,
            OutputArray dst,
            float h = 1,
            int templateWindowSize = 4,
            int searchWindowSize = 16,
            int blockMatchingStep1 = 2500,
            int blockMatchingStep2 = 400,
            int groupSize = 8,
            int slidingStep = 1,
            float beta = 2.0f,
            int normType = cv::NORM_L2,
            int step = cv::xphoto::BM3D_STEPALL,
            int transformType = cv::xphoto::HAAR);
        //! @}
    }
}

#endif // __OPENCV_BM3D_IMAGE_DENOISING_HPP__
