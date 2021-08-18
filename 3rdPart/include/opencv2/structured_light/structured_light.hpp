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
 // Copyright (C) 2015, OpenCV Foundation, all rights reserved.
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

#ifndef __OPENCV_STRUCTURED_LIGHT_HPP__
#define __OPENCV_STRUCTURED_LIGHT_HPP__

#include "opencv2/core.hpp"

namespace cv {
namespace structured_light {
//! @addtogroup structured_light
//! @{

//! Type of the decoding algorithm
// other algorithms can be implemented
enum
{
  DECODE_3D_UNDERWORLD = 0  //!< Kyriakos Herakleous, Charalambos Poullis. "3DUNDERWORLD-SLS: An Open-Source Structured-Light Scanning System for Rapid Geometry Acquisition", arXiv preprint arXiv:1406.6595 (2014).
};

/** @brief Abstract base class for generating and decoding structured light patterns.
 */
class CV_EXPORTS_W StructuredLightPattern : public virtual Algorithm
{
 public:
  /** @brief Generates the structured light pattern to project.

   @param patternImages The generated pattern: a vector<Mat>, in which each image is a CV_8U Mat at projector's resolution.
   */
  CV_WRAP
  virtual bool generate( OutputArrayOfArrays patternImages ) = 0;

  /** @brief Decodes the structured light pattern, generating a disparity map

   @param patternImages The acquired pattern images to decode (vector<vector<Mat>>), loaded as grayscale and previously rectified.
   @param disparityMap The decoding result: a CV_64F Mat at image resolution, storing the computed disparity map.
   @param blackImages The all-black images needed for shadowMasks computation.
   @param whiteImages The all-white images needed for shadowMasks computation.
   @param flags Flags setting decoding algorithms. Default: DECODE_3D_UNDERWORLD.
   @note All the images must be at the same resolution.
   */
  CV_WRAP
  virtual bool decode( const std::vector< std::vector<Mat> >& patternImages, OutputArray disparityMap,
                       InputArrayOfArrays blackImages = noArray(),
                       InputArrayOfArrays whiteImages = noArray(),
                       int flags = DECODE_3D_UNDERWORLD ) const = 0;
};

//! @}

}
}
#endif
