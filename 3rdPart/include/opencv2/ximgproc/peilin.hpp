// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#ifndef __OPENCV_PEILIN_HPP__
#define __OPENCV_PEILIN_HPP__

#include <opencv2/core.hpp>

namespace cv { namespace ximgproc {

    //! @addtogroup ximgproc
    //! @{

    /**
    * @brief   Calculates an affine transformation that normalize given image using Pei&Lin Normalization.
    *
    * Assume given image \f$I=T(\bar{I})\f$ where \f$\bar{I}\f$ is a normalized image and \f$T\f$ is an affine transformation distorting this image by translation, rotation, scaling and skew.
    * The function returns an affine transformation matrix corresponding to the transformation \f$T^{-1}\f$ described in [PeiLin95].
    * For more details about this implementation, please see
    * [PeiLin95] Soo-Chang Pei and Chao-Nan Lin. Image normalization for pattern recognition. Image and Vision Computing, Vol. 13, N.10, pp. 711-723, 1995.
    *
    * @param I Given transformed image.
    * @return Transformation matrix corresponding to inversed image transformation
    */
    CV_EXPORTS Matx23d PeiLinNormalization ( InputArray I );
    /** @overload */
    CV_EXPORTS_W void PeiLinNormalization ( InputArray I, OutputArray T );

}} // namespace

#endif
