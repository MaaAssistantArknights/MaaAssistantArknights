/*
By downloading, copying, installing or using the software you agree to this license.
If you do not agree to this license, do not download, install,
copy or use the software.


                          License Agreement
               For Open Source Computer Vision Library
                       (3-clause BSD License)

Copyright (C) 2000-2015, Intel Corporation, all rights reserved.
Copyright (C) 2009-2011, Willow Garage Inc., all rights reserved.
Copyright (C) 2009-2015, NVIDIA Corporation, all rights reserved.
Copyright (C) 2010-2013, Advanced Micro Devices, Inc., all rights reserved.
Copyright (C) 2015, OpenCV Foundation, all rights reserved.
Copyright (C) 2015, Itseez Inc., all rights reserved.
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
warranties of merchantability and fitness for a particular purpose are disclaimed.
In no event shall copyright holders or contributors be liable for any direct,
indirect, incidental, special, exemplary, or consequential damages
(including, but not limited to, procurement of substitute goods or services;
loss of use, data, or profits; or business interruption) however caused
and on any theory of liability, whether in contract, strict liability,
or tort (including negligence or otherwise) arising in any way out of
the use of this software, even if advised of the possibility of such damage.
*/

#ifndef __OPENCV_BIF_HPP__
#define __OPENCV_BIF_HPP__

#include "opencv2/core.hpp"

namespace cv {
namespace face {

/** Implementation of bio-inspired features (BIF) from the paper:
 *  Guo, Guodong, et al. "Human age estimation using bio-inspired features."
 *  Computer Vision and Pattern Recognition, 2009. CVPR 2009.
 */
class CV_EXPORTS_W BIF : public Algorithm {
public:
    /** @returns The number of filter bands used for computing BIF. */
    CV_WRAP virtual int getNumBands() const = 0;

    /** @returns The number of image rotations. */
    CV_WRAP virtual int getNumRotations() const = 0;

    /** Computes features sby input image.
     *  @param image Input image (CV_32FC1).
     *  @param features Feature vector (CV_32FC1).
     */
    CV_WRAP virtual void compute(InputArray image,
                                 OutputArray features) const = 0;

    /**
     * @param num_bands The number of filter bands (<=8) used for computing BIF.
     * @param num_rotations The number of image rotations for computing BIF.
     * @returns Object for computing BIF.
     */
    CV_WRAP static Ptr<BIF> create(int num_bands = 8, int num_rotations = 12);
};

}  // namespace cv
}  // namespace face

#endif  // #ifndef __OPENCV_FACEREC_HPP__
