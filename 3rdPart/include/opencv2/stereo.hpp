/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                          License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Copyright (C) 2013, OpenCV Foundation, all rights reserved.
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

#ifndef __OPENCV_STEREO_HPP__
#define __OPENCV_STEREO_HPP__

#include "opencv2/core.hpp"
#include "opencv2/stereo/descriptor.hpp"

/**
@defgroup stereo Stereo Correspondance Algorithms

*/

namespace cv
{
    namespace stereo
    {
        //! @addtogroup stereo
        //! @{
        /** @brief Filters off small noise blobs (speckles) in the disparity map
        @param img The input 16-bit signed disparity image
        @param newVal The disparity value used to paint-off the speckles
        @param maxSpeckleSize The maximum speckle size to consider it a speckle. Larger blobs are not
        affected by the algorithm
        @param maxDiff Maximum difference between neighbor disparity pixels to put them into the same
        blob. Note that since StereoBM, StereoSGBM and may be other algorithms return a fixed-point
        disparity map, where disparity values are multiplied by 16, this scale factor should be taken into
        account when specifying this parameter value.
        @param buf The optional temporary buffer to avoid memory allocation within the function.
        */
        /** @brief The base class for stereo correspondence algorithms.
        */
        class StereoMatcher : public Algorithm
        {
        public:
            enum { DISP_SHIFT = 4,
                DISP_SCALE = (1 << DISP_SHIFT)
            };

            /** @brief Computes disparity map for the specified stereo pair

            @param left Left 8-bit single-channel image.
            @param right Right image of the same size and the same type as the left one.
            @param disparity Output disparity map. It has the same size as the input images. Some algorithms,
            like StereoBM or StereoSGBM compute 16-bit fixed-point disparity map (where each disparity value
            has 4 fractional bits), whereas other algorithms output 32-bit floating-point disparity map.
            */
            virtual void compute( InputArray left, InputArray right,
                OutputArray disparity ) = 0;

            virtual int getMinDisparity() const = 0;
            virtual void setMinDisparity(int minDisparity) = 0;

            virtual int getNumDisparities() const = 0;
            virtual void setNumDisparities(int numDisparities) = 0;

            virtual int getBlockSize() const = 0;
            virtual void setBlockSize(int blockSize) = 0;

            virtual int getSpeckleWindowSize() const = 0;
            virtual void setSpeckleWindowSize(int speckleWindowSize) = 0;

            virtual int getSpeckleRange() const = 0;
            virtual void setSpeckleRange(int speckleRange) = 0;

            virtual int getDisp12MaxDiff() const = 0;
            virtual void setDisp12MaxDiff(int disp12MaxDiff) = 0;

        };
        //!speckle removal algorithms. These algorithms have the purpose of removing small regions
        enum {
            CV_SPECKLE_REMOVAL_ALGORITHM, CV_SPECKLE_REMOVAL_AVG_ALGORITHM
        };
        //!subpixel interpolationm methods for disparities.
        enum{
            CV_QUADRATIC_INTERPOLATION, CV_SIMETRICV_INTERPOLATION
        };
        /** @brief Class for computing stereo correspondence using the block matching algorithm, introduced and
        contributed to OpenCV by K. Konolige.
        */
        class StereoBinaryBM : public StereoMatcher
        {
        public:
            enum { PREFILTER_NORMALIZED_RESPONSE = 0,
                PREFILTER_XSOBEL              = 1
            };

            virtual int getPreFilterType() const = 0;
            virtual void setPreFilterType(int preFilterType) = 0;

            virtual int getPreFilterSize() const = 0;
            virtual void setPreFilterSize(int preFilterSize) = 0;

            virtual int getPreFilterCap() const = 0;
            virtual void setPreFilterCap(int preFilterCap) = 0;

            virtual int getTextureThreshold() const = 0;
            virtual void setTextureThreshold(int textureThreshold) = 0;

            virtual int getUniquenessRatio() const = 0;
            virtual void setUniquenessRatio(int uniquenessRatio) = 0;

            virtual int getSmallerBlockSize() const = 0;
            virtual void setSmallerBlockSize(int blockSize) = 0;

            virtual int getScalleFactor() const = 0 ;
            virtual void setScalleFactor(int factor) = 0;

            virtual int getSpekleRemovalTechnique() const = 0 ;
            virtual void setSpekleRemovalTechnique(int factor) = 0;

            virtual bool getUsePrefilter() const = 0 ;
            virtual void setUsePrefilter(bool factor) = 0;

            virtual int getBinaryKernelType() const = 0;
            virtual void setBinaryKernelType(int value) = 0;

            virtual int getAgregationWindowSize() const = 0;
            virtual void setAgregationWindowSize(int value) = 0;
            /** @brief Creates StereoBM object

            @param numDisparities the disparity search range. For each pixel algorithm will find the best
            disparity from 0 (default minimum disparity) to numDisparities. The search range can then be
            shifted by changing the minimum disparity.
            @param blockSize the linear size of the blocks compared by the algorithm. The size should be odd
            (as the block is centered at the current pixel). Larger block size implies smoother, though less
            accurate disparity map. Smaller block size gives more detailed disparity map, but there is higher
            chance for algorithm to find a wrong correspondence.

            The function create StereoBM object. You can then call StereoBM::compute() to compute disparity for
            a specific stereo pair.
            */
            CV_EXPORTS static Ptr< cv::stereo::StereoBinaryBM > create(int numDisparities = 0, int blockSize = 9);
        };

        /** @brief The class implements the modified H. Hirschmuller algorithm @cite HH08 that differs from the original
        one as follows:

        -   By default, the algorithm is single-pass, which means that you consider only 5 directions
        instead of 8. Set mode=StereoSGBM::MODE_HH in createStereoSGBM to run the full variant of the
        algorithm but beware that it may consume a lot of memory.
        -   The algorithm matches blocks, not individual pixels. Though, setting blockSize=1 reduces the
        blocks to single pixels.
        -   Mutual information cost function is not implemented. Instead, a simpler Birchfield-Tomasi
        sub-pixel metric from @cite BT98 is used. Though, the color images are supported as well.
        -   Some pre- and post- processing steps from K. Konolige algorithm StereoBM are included, for
        example: pre-filtering (StereoBM::PREFILTER_XSOBEL type) and post-filtering (uniqueness
        check, quadratic interpolation and speckle filtering).

        @note
        -   (Python) An example illustrating the use of the StereoSGBM matching algorithm can be found
        at opencv_source_code/samples/python2/stereo_match.py
        */
        class StereoBinarySGBM : public StereoMatcher
        {
        public:
            enum
            {
                MODE_SGBM = 0,
                MODE_HH   = 1
            };

            virtual int getPreFilterCap() const = 0;
            virtual void setPreFilterCap(int preFilterCap) = 0;

            virtual int getUniquenessRatio() const = 0;
            virtual void setUniquenessRatio(int uniquenessRatio) = 0;

            virtual int getP1() const = 0;
            virtual void setP1(int P1) = 0;

            virtual int getP2() const = 0;
            virtual void setP2(int P2) = 0;

            virtual int getMode() const = 0;
            virtual void setMode(int mode) = 0;

            virtual int getSpekleRemovalTechnique() const = 0 ;
            virtual void setSpekleRemovalTechnique(int factor) = 0;

            virtual int getBinaryKernelType() const = 0;
            virtual void setBinaryKernelType(int value) = 0;

            virtual int getSubPixelInterpolationMethod() const = 0;
            virtual void setSubPixelInterpolationMethod(int value) = 0;

            /** @brief Creates StereoSGBM object

            @param minDisparity Minimum possible disparity value. Normally, it is zero but sometimes
            rectification algorithms can shift images, so this parameter needs to be adjusted accordingly.
            @param numDisparities Maximum disparity minus minimum disparity. The value is always greater than
            zero. In the current implementation, this parameter must be divisible by 16.
            @param blockSize Matched block size. It must be an odd number \>=1 . Normally, it should be
            somewhere in the 3..11 range.
            @param P1 The first parameter controlling the disparity smoothness.This parameter is used for the case of slanted surfaces (not fronto parallel).
            @param P2 The second parameter controlling the disparity smoothness.This parameter is used for "solving" the depth discontinuities problem.
            The larger the values are, the smoother the disparity is. P1 is the penalty on the disparity change by plus or minus 1
            between neighbor pixels. P2 is the penalty on the disparity change by more than 1 between neighbor
            pixels. The algorithm requires P2 \> P1 . See stereo_match.cpp sample where some reasonably good
            P1 and P2 values are shown (like 8\*number_of_image_channels\*SADWindowSize\*SADWindowSize and
            32\*number_of_image_channels\*SADWindowSize\*SADWindowSize , respectively).
            @param disp12MaxDiff Maximum allowed difference (in integer pixel units) in the left-right
            disparity check. Set it to a non-positive value to disable the check.
            @param preFilterCap Truncation value for the prefiltered image pixels. The algorithm first
            computes x-derivative at each pixel and clips its value by [-preFilterCap, preFilterCap] interval.
            The result values are passed to the Birchfield-Tomasi pixel cost function.
            @param uniquenessRatio Margin in percentage by which the best (minimum) computed cost function
            value should "win" the second best value to consider the found match correct. Normally, a value
            within the 5-15 range is good enough.
            @param speckleWindowSize Maximum size of smooth disparity regions to consider their noise speckles
            and invalidate. Set it to 0 to disable speckle filtering. Otherwise, set it somewhere in the
            50-200 range.
            @param speckleRange Maximum disparity variation within each connected component. If you do speckle
            filtering, set the parameter to a positive value, it will be implicitly multiplied by 16.
            Normally, 1 or 2 is good enough.
            @param mode Set it to StereoSGBM::MODE_HH to run the full-scale two-pass dynamic programming
            algorithm. It will consume O(W\*H\*numDisparities) bytes, which is large for 640x480 stereo and
            huge for HD-size pictures. By default, it is set to false .

            The first constructor initializes StereoSGBM with all the default parameters. So, you only have to
            set StereoSGBM::numDisparities at minimum. The second constructor enables you to set each parameter
            to a custom value.
            */
            CV_EXPORTS static Ptr<cv::stereo::StereoBinarySGBM> create(int minDisparity, int numDisparities, int blockSize,
                int P1 = 100, int P2 = 1000, int disp12MaxDiff = 1,
                int preFilterCap = 0, int uniquenessRatio = 5,
                int speckleWindowSize = 400, int speckleRange = 200,
                int mode = StereoBinarySGBM::MODE_SGBM);
        };
        //! @}
    }//stereo
} // cv

#endif
