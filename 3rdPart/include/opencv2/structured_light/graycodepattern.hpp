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

#ifndef __OPENCV_GRAY_CODE_PATTERN_HPP__
#define __OPENCV_GRAY_CODE_PATTERN_HPP__

#include "opencv2/core.hpp"
#include "opencv2/structured_light/structured_light.hpp"

namespace cv {
namespace structured_light {
//! @addtogroup structured_light
//! @{

/** @brief Class implementing the Gray-code pattern, based on @cite UNDERWORLD.
 *
 *  The generation of the pattern images is performed with Gray encoding using the traditional white and black colors.
 *
 *  The information about the two image axes x, y is encoded separately into two different pattern sequences.
 *  A projector P with resolution (P_res_x, P_res_y) will result in Ncols = log 2 (P_res_x) encoded pattern images representing the columns, and
 *  in Nrows = log 2 (P_res_y) encoded pattern images representing the rows.
 *  For example a projector with resolution 1024x768 will result in Ncols = 10 and Nrows = 10.

 *  However, the generated pattern sequence consists of both regular color and color-inverted images: inverted pattern images are images
 *  with the same structure as the original but with inverted colors.
 *  This provides an effective method for easily determining the intensity value of each pixel when it is lit (highest value) and
 *  when it is not lit (lowest value). So for a a projector with resolution 1024x768, the number of pattern images will be Ncols * 2 + Nrows * 2 = 40.
 *
 */
class CV_EXPORTS_W GrayCodePattern : public StructuredLightPattern
{
 public:

  /** @brief Parameters of StructuredLightPattern constructor.
   *  @param width Projector's width. Default value is 1024.
   *  @param height Projector's height. Default value is 768.
   */
  struct CV_EXPORTS Params
  {
    Params();
    int width;
    int height;
  };

  /** @brief Constructor
   @param parameters GrayCodePattern parameters GrayCodePattern::Params: the width and the height of the projector.
   */
  static Ptr<GrayCodePattern> create( const GrayCodePattern::Params &parameters = GrayCodePattern::Params() );

  // alias for scripting
  CV_WRAP
  static Ptr<GrayCodePattern> create( int width, int height );

  /** @brief Get the number of pattern images needed for the graycode pattern.
   *
   * @return The number of pattern images needed for the graycode pattern.
   *
   */
   CV_WRAP
   virtual size_t getNumberOfPatternImages() const = 0;

  /** @brief Sets the value for white threshold, needed for decoding.
   *
   *  White threshold is a number between 0-255 that represents the minimum brightness difference required for valid pixels, between the graycode pattern and its inverse images; used in getProjPixel method.
   *
   *  @param value The desired white threshold value.
   *
   */
  CV_WRAP
  virtual void setWhiteThreshold( size_t value ) = 0;

  /** @brief Sets the value for black threshold, needed for decoding (shadowsmasks computation).
   *
   *  Black threshold is a number between 0-255 that represents the minimum brightness difference required for valid pixels, between the fully illuminated (white) and the not illuminated images (black); used in computeShadowMasks method.
   *
   *  @param value The desired black threshold value.
   *
   */
  CV_WRAP
  virtual void setBlackThreshold( size_t value ) = 0;

  /** @brief Generates the all-black and all-white images needed for shadowMasks computation.
   *
   *  To identify shadow regions, the regions of two images where the pixels are not lit by projector's light and thus where there is not coded information,
   *  the 3DUNDERWORLD algorithm computes a shadow mask for the two cameras views, starting from a white and a black images captured by each camera.
   *  This method generates these two additional images to project.
   *
   *  @param blackImage The generated all-black CV_8U image, at projector's resolution.
   *  @param whiteImage The generated all-white CV_8U image, at projector's resolution.
   */
  CV_WRAP
  virtual void getImagesForShadowMasks( InputOutputArray blackImage, InputOutputArray whiteImage ) const = 0;

  /** @brief For a (x,y) pixel of a camera returns the corresponding projector pixel.
   *
   *  The function decodes each pixel in the pattern images acquired by a camera into their corresponding decimal numbers representing the projector's column and row,
   *  providing a mapping between camera's and projector's pixel.
   *
   *  @param patternImages The pattern images acquired by the camera, stored in a grayscale vector < Mat >.
   *  @param x x coordinate of the image pixel.
   *  @param y y coordinate of the image pixel.
   *  @param projPix Projector's pixel corresponding to the camera's pixel: projPix.x and projPix.y are the image coordinates of the projector's pixel corresponding to the pixel being decoded in a camera.
   */
  CV_WRAP
  virtual bool getProjPixel( InputArrayOfArrays patternImages, int x, int y, CV_OUT Point &projPix ) const = 0;
};

//! @}
}
}
#endif
