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

Algorithmic details of this algorithm can be found at:
 * O. Green, Y. Birk, "A Computationally Efficient Algorithm for the 2D Covariance Method", ACM/IEEE International Conference on High Performance Computing, Networking, Storage and Analysis, Denver, Colorado, 2013
A previous and less efficient version of the algorithm can be found:
 * O. Green, L. David, A. Galperin, Y. Birk, "Efficient parallel computation of the estimated covariance matrix", arXiv, 2013


*/
#ifndef __OPENCV_ESTIMATECOVARIANCE_HPP__
#define __OPENCV_ESTIMATECOVARIANCE_HPP__
#ifdef __cplusplus

#include <opencv2/core.hpp>

namespace cv
{
namespace ximgproc
{

/** @brief Computes the estimated covariance matrix of an image using the sliding
window forumlation.

@param src The source image. Input image must be of a complex type.
@param dst The destination estimated covariance matrix. Output matrix will be size (windowRows*windowCols, windowRows*windowCols).
@param windowRows The number of rows in the window.
@param windowCols The number of cols in the window.
The window size parameters control the accuracy of the estimation.
The sliding window moves over the entire image from the top-left corner
to the bottom right corner. Each location of the window represents a sample.
If the window is the size of the image, then this gives the exact covariance matrix.
For all other cases, the sizes of the window will impact the number of samples
and the number of elements in the estimated covariance matrix.
*/

CV_EXPORTS_W void covarianceEstimation(InputArray src, OutputArray dst, int windowRows, int windowCols);

}
}
#endif
#endif
