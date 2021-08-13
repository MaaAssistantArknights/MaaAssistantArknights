/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
// Copyright (C) 2013, Alfonso Sanchez-Beato, all rights reserved.
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
// In no event shall the contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef MAP_H_
#define MAP_H_

#include <opencv2/core.hpp> // Basic OpenCV structures (cv::Mat, Scalar)

/** @defgroup reg Image Registration

The Registration module implements parametric image registration. The implemented method is direct
alignment, that is, it uses directly the pixel values for calculating the registration between a
pair of images, as opposed to feature-based registration. The implementation follows essentially the
corresponding part of @cite Szeliski06 .

Feature based methods have some advantages over pixel based methods when we are trying to register
pictures that have been shoot under different lighting conditions or exposition times, or when the
images overlap only partially. On the other hand, the main advantage of pixel-based methods when
compared to feature based methods is their better precision for some pictures (those shoot under
similar lighting conditions and that have a significative overlap), due to the fact that we are
using all the information available in the image, which allows us to achieve subpixel accuracy. This
is particularly important for certain applications like multi-frame denoising or super-resolution.

In fact, pixel and feature registration methods can complement each other: an application could
first obtain a coarse registration using features and then refine the registration using a pixel
based method on the overlapping area of the images. The code developed allows this use case.

The module implements classes derived from the abstract classes cv::reg::Map or cv::reg::Mapper. The
former models a coordinate transformation between two reference frames, while the later encapsulates
a way of invoking a method that calculates a Map between two images. Although the objective has been
to implement pixel based methods, the module can be extended to support other methods that can
calculate transformations between images (feature methods, optical flow, etc.).

Each class derived from Map implements a motion model, as follows:

-   MapShift: Models a simple translation
-   MapAffine: Models an affine transformation
-   MapProjec: Models a projective transformation

MapProject can also be used to model affine motion or translations, but some operations on it are
more costly, and that is the reason for defining the other two classes.

The classes derived from Mapper are

-   MapperGradShift: Gradient based alignment for calculating translations. It produces a MapShift
    (two parameters that correspond to the shift vector).
-   MapperGradEuclid: Gradient based alignment for euclidean motions, that is, rotations and
    translations. It calculates three parameters (angle and shift vector), although the result is
    stored in a MapAffine object for convenience.
-   MapperGradSimilar: Gradient based alignment for calculating similarities, which adds scaling to
    the euclidean motion. It calculates four parameters (two for the anti-symmetric matrix and two
    for the shift vector), although the result is stored in a MapAffine object for better
    convenience.
-   MapperGradAffine: Gradient based alignment for an affine motion model. The number of parameters
    is six and the result is stored in a MapAffine object.
-   MapperGradProj: Gradient based alignment for calculating projective transformations. The number
    of parameters is eight and the result is stored in a MapProject object.
-   MapperPyramid: It implements hyerarchical motion estimation using a Gaussian pyramid. Its
    constructor accepts as argument any other object that implements the Mapper interface, and it is
    that mapper the one called by MapperPyramid for each scale of the pyramid.

If the motion between the images is not very small, the normal way of using these classes is to
create a MapperGrad\* object and use it as input to create a MapperPyramid, which in turn is called
to perform the calculation. However, if the motion between the images is small enough, we can use
directly the MapperGrad\* classes. Another possibility is to use first a feature based method to
perform a coarse registration and then do a refinement through MapperPyramid or directly a
MapperGrad\* object. The "calculate" method of the mappers accepts an initial estimation of the
motion as input.

When deciding which MapperGrad to use we must take into account that mappers with more parameters
can handle more complex motions, but involve more calculations and are therefore slower. Also, if we
are confident on the motion model that is followed by the sequence, increasing the number of
parameters beyond what we need will decrease the accuracy: it is better to use the least number of
degrees of freedom that we can.

In the module tests there are examples that show how to register a pair of images using any of the
implemented mappers.
*/

namespace cv {
namespace reg {

//! @addtogroup reg
//! @{

/** @brief Base class for modelling a Map between two images.

The class is only used to define the common interface for any possible map.
 */
class CV_EXPORTS_W Map
{
public:
    /*!
     * Virtual destructor
     */
    virtual ~Map();

    /*!
     * Warps image to a new coordinate frame. The calculation is img2(x)=img1(T^{-1}(x)), as we
     * have to apply the inverse transformation to the points to move them to were the values
     * of img2 are.
     * \param[in] img1 Original image
     * \param[out] img2 Warped image
     */
    CV_WRAP virtual void warp(InputArray img1, OutputArray img2) const;

    /*!
     * Warps image to a new coordinate frame. The calculation is img2(x)=img1(T(x)), so in fact
     * this is the inverse warping as we are taking the value of img1 with the forward
     * transformation of the points.
     * \param[in] img1 Original image
     * \param[out] img2 Warped image
     */
    CV_WRAP virtual void inverseWarp(InputArray img1, OutputArray img2) const = 0;

    /*!
     * Calculates the inverse map
     * \return Inverse map
     */
    CV_WRAP virtual cv::Ptr<Map> inverseMap() const = 0;

    /*!
     * Changes the map composing the current transformation with the one provided in the call.
     * The order is first the current transformation, then the input argument.
     * \param[in] map Transformation to compose with.
     */
    CV_WRAP virtual void compose(cv::Ptr<Map> map) = 0;

    /*!
     * Scales the map by a given factor as if the coordinates system is expanded/compressed
     * by that factor.
     * \param[in] factor Expansion if bigger than one, compression if smaller than one
     */
    CV_WRAP virtual void scale(double factor) = 0;
};

//! @}

}}  // namespace cv::reg

#endif  // MAP_H_
