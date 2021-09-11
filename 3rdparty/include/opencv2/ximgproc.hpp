/*
 *  By downloading, copying, installing or using the software you agree to this license.
 *  If you do not agree to this license, do not download, install,
 *  copy or use the software.
 *
 *
 *  License Agreement
 *  For Open Source Computer Vision Library
 *  (3 - clause BSD License)
 *
 *  Redistribution and use in source and binary forms, with or without modification,
 *  are permitted provided that the following conditions are met :
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and / or other materials provided with the distribution.
 *
 *  * Neither the names of the copyright holders nor the names of the contributors
 *  may be used to endorse or promote products derived from this software
 *  without specific prior written permission.
 *
 *  This software is provided by the copyright holders and contributors "as is" and
 *  any express or implied warranties, including, but not limited to, the implied
 *  warranties of merchantability and fitness for a particular purpose are disclaimed.
 *  In no event shall copyright holders or contributors be liable for any direct,
 *  indirect, incidental, special, exemplary, or consequential damages
 *  (including, but not limited to, procurement of substitute goods or services;
 *  loss of use, data, or profits; or business interruption) however caused
 *  and on any theory of liability, whether in contract, strict liability,
 *  or tort(including negligence or otherwise) arising in any way out of
 *  the use of this software, even if advised of the possibility of such damage.
 */

#ifndef __OPENCV_XIMGPROC_HPP__
#define __OPENCV_XIMGPROC_HPP__

#include "ximgproc/edge_filter.hpp"
#include "ximgproc/disparity_filter.hpp"
#include "ximgproc/sparse_match_interpolator.hpp"
#include "ximgproc/structured_edge_detection.hpp"
#include "ximgproc/edgeboxes.hpp"
#include "ximgproc/seeds.hpp"
#include "ximgproc/segmentation.hpp"
#include "ximgproc/fast_hough_transform.hpp"
#include "ximgproc/estimated_covariance.hpp"
#include "ximgproc/weighted_median_filter.hpp"
#include "ximgproc/slic.hpp"
#include "ximgproc/lsc.hpp"
#include "ximgproc/paillou_filter.hpp"
#include "ximgproc/fast_line_detector.hpp"
#include "ximgproc/deriche_filter.hpp"
#include "ximgproc/peilin.hpp"
#include "ximgproc/fourier_descriptors.hpp"
#include "ximgproc/ridgefilter.hpp"
#include "ximgproc/brightedges.hpp"


/** @defgroup ximgproc Extended Image Processing
  @{
    @defgroup ximgproc_edge Structured forests for fast edge detection

This module contains implementations of modern structured edge detection algorithms,
i.e. algorithms which somehow takes into account pixel affinities in natural images.

    @defgroup ximgproc_edgeboxes EdgeBoxes

    @defgroup ximgproc_filters Filters

    @defgroup ximgproc_superpixel Superpixels

    @defgroup ximgproc_segmentation Image segmentation

    @defgroup ximgproc_fast_line_detector Fast line detector

    @defgroup ximgproc_fourier Fourier descriptors
    @}
*/

namespace cv
{
namespace ximgproc
{

enum ThinningTypes{
    THINNING_ZHANGSUEN    = 0, // Thinning technique of Zhang-Suen
    THINNING_GUOHALL      = 1  // Thinning technique of Guo-Hall
};

/**
* @brief Specifies the binarization method to use in cv::ximgproc::niBlackThreshold
*/
enum LocalBinarizationMethods{
	BINARIZATION_NIBLACK = 0, //!< Classic Niblack binarization. See @cite Niblack1985 .
	BINARIZATION_SAUVOLA = 1, //!< Sauvola's technique. See @cite Sauvola1997 .
	BINARIZATION_WOLF = 2,    //!< Wolf's technique. See @cite Wolf2004 .
	BINARIZATION_NICK = 3     //!< NICK technique. See @cite Khurshid2009 .
};

//! @addtogroup ximgproc
//! @{

/** @brief Performs thresholding on input images using Niblack's technique or some of the
popular variations it inspired.

The function transforms a grayscale image to a binary image according to the formulae:
-   **THRESH_BINARY**
    \f[dst(x,y) =  \fork{\texttt{maxValue}}{if \(src(x,y) > T(x,y)\)}{0}{otherwise}\f]
-   **THRESH_BINARY_INV**
    \f[dst(x,y) =  \fork{0}{if \(src(x,y) > T(x,y)\)}{\texttt{maxValue}}{otherwise}\f]
where \f$T(x,y)\f$ is a threshold calculated individually for each pixel.

The threshold value \f$T(x, y)\f$ is determined based on the binarization method chosen. For
classic Niblack, it is the mean minus \f$ k \f$ times standard deviation of
\f$\texttt{blockSize} \times\texttt{blockSize}\f$ neighborhood of \f$(x, y)\f$.

The function can't process the image in-place.

@param _src Source 8-bit single-channel image.
@param _dst Destination image of the same size and the same type as src.
@param maxValue Non-zero value assigned to the pixels for which the condition is satisfied,
used with the THRESH_BINARY and THRESH_BINARY_INV thresholding types.
@param type Thresholding type, see cv::ThresholdTypes.
@param blockSize Size of a pixel neighborhood that is used to calculate a threshold value
for the pixel: 3, 5, 7, and so on.
@param k The user-adjustable parameter used by Niblack and inspired techniques. For Niblack, this is
normally a value between 0 and 1 that is multiplied with the standard deviation and subtracted from
the mean.
@param binarizationMethod Binarization method to use. By default, Niblack's technique is used.
Other techniques can be specified, see cv::ximgproc::LocalBinarizationMethods.

@sa  threshold, adaptiveThreshold
 */
CV_EXPORTS_W void niBlackThreshold( InputArray _src, OutputArray _dst,
                                    double maxValue, int type,
                                    int blockSize, double k, int binarizationMethod = BINARIZATION_NIBLACK );

/** @brief Applies a binary blob thinning operation, to achieve a skeletization of the input image.

The function transforms a binary blob image into a skeletized form using the technique of Zhang-Suen.

@param src Source 8-bit single-channel image, containing binary blobs, with blobs having 255 pixel values.
@param dst Destination image of the same size and the same type as src. The function can work in-place.
@param thinningType Value that defines which thinning algorithm should be used. See cv::ximgproc::ThinningTypes
 */
CV_EXPORTS_W void thinning( InputArray src, OutputArray dst, int thinningType = THINNING_ZHANGSUEN);

/** @brief Performs anisotropic diffusion on an image.

 The function applies Perona-Malik anisotropic diffusion to an image. This is the solution to the partial differential equation:

 \f[{\frac  {\partial I}{\partial t}}={\mathrm  {div}}\left(c(x,y,t)\nabla I\right)=\nabla c\cdot \nabla I+c(x,y,t)\Delta I\f]

 Suggested functions for c(x,y,t) are:

 \f[c\left(\|\nabla I\|\right)=e^{{-\left(\|\nabla I\|/K\right)^{2}}}\f]

 or

 \f[ c\left(\|\nabla I\|\right)={\frac {1}{1+\left({\frac  {\|\nabla I\|}{K}}\right)^{2}}} \f]

 @param src Source image with 3 channels.
 @param dst Destination image of the same size and the same number of channels as src .
 @param alpha The amount of time to step forward by on each iteration (normally, it's between 0 and 1).
 @param K sensitivity to the edges
 @param niters The number of iterations
*/
CV_EXPORTS_W void anisotropicDiffusion(InputArray src, OutputArray dst, float alpha, float K, int niters );

//! @}

}
}

#endif // __OPENCV_XIMGPROC_HPP__
