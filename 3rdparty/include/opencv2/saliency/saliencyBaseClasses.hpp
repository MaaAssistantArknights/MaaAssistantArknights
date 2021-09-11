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
 // Copyright (C) 2014, OpenCV Foundation, all rights reserved.
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

#ifndef __OPENCV_SALIENCY_BASE_CLASSES_HPP__
#define __OPENCV_SALIENCY_BASE_CLASSES_HPP__

#include "opencv2/core.hpp"
#include <opencv2/core/persistence.hpp>
#include "opencv2/imgproc.hpp"
#include <iostream>
#include <sstream>
#include <complex>

namespace cv
{
namespace saliency
{

//! @addtogroup saliency
//! @{

/************************************ Saliency Base Class ************************************/

class CV_EXPORTS_W Saliency : public virtual Algorithm
{
 public:
  /**
   * \brief Destructor
   */
  virtual ~Saliency();

  /**
   * \brief Compute the saliency
   * \param image        The image.
   * \param saliencyMap      The computed saliency map.
   * \return true if the saliency map is computed, false otherwise
   */
  CV_WRAP bool computeSaliency( InputArray image, OutputArray saliencyMap );

 protected:

  virtual bool computeSaliencyImpl( InputArray image, OutputArray saliencyMap ) = 0;
  String className;
};

/************************************ Static Saliency Base Class ************************************/
class CV_EXPORTS_W StaticSaliency : public virtual Saliency
{
 public:

    /** @brief This function perform a binary map of given saliency map. This is obtained in this
    way:

    In a first step, to improve the definition of interest areas and facilitate identification of
    targets, a segmentation by clustering is performed, using *K-means algorithm*. Then, to gain a
    binary representation of clustered saliency map, since values of the map can vary according to
    the characteristics of frame under analysis, it is not convenient to use a fixed threshold. So,
    *Otsu's algorithm* is used, which assumes that the image to be thresholded contains two classes
    of pixels or bi-modal histograms (e.g. foreground and back-ground pixels); later on, the
    algorithm calculates the optimal threshold separating those two classes, so that their
    intra-class variance is minimal.

    @param _saliencyMap the saliency map obtained through one of the specialized algorithms
    @param _binaryMap the binary map
     */
  CV_WRAP bool computeBinaryMap( InputArray _saliencyMap, OutputArray _binaryMap );
 protected:
  virtual bool computeSaliencyImpl( InputArray image, OutputArray saliencyMap ) CV_OVERRIDE = 0;

};

/************************************ Motion Saliency Base Class ************************************/
class CV_EXPORTS_W MotionSaliency : public virtual Saliency
{

 protected:
  virtual bool computeSaliencyImpl( InputArray image, OutputArray saliencyMap ) CV_OVERRIDE = 0;

};

/************************************ Objectness Base Class ************************************/
class CV_EXPORTS_W Objectness : public virtual Saliency
{

 protected:
  virtual bool computeSaliencyImpl( InputArray image, OutputArray saliencyMap ) CV_OVERRIDE = 0;

};

//! @}

} /* namespace saliency */
} /* namespace cv */

#endif
