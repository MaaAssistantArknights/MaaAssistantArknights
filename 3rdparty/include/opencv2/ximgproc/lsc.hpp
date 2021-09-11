/*********************************************************************
 * Software License Agreement (BSD License)
 *
 * Copyright (c) 2014, 2015
 * Zhengqin Li <li-zq12 at mails dot tsinghua dot edu dot cn>
 * Jiansheng Chen <jschenthu at mail dot tsinghua dot edu dot cn>
 * Tsinghua University
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the copyright holders nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/*

 "Superpixel Segmentation using Linear Spectral Clustering"
 Zhengqin Li, Jiansheng Chen, IEEE Conference on Computer Vision and Pattern
 Recognition (CVPR), Jun. 2015

 OpenCV port by: Cristian Balint <cristian dot balint at gmail dot com>
 */

#ifndef __OPENCV_LSC_HPP__
#define __OPENCV_LSC_HPP__
#ifdef __cplusplus

#include <opencv2/core.hpp>

namespace cv
{
namespace ximgproc
{

//! @addtogroup ximgproc_superpixel
//! @{

/** @brief Class implementing the LSC (Linear Spectral Clustering) superpixels
algorithm described in @cite LiCVPR2015LSC.

LSC (Linear Spectral Clustering) produces compact and uniform superpixels with low
computational costs. Basically, a normalized cuts formulation of the superpixel
segmentation is adopted based on a similarity metric that measures the color
similarity and space proximity between image pixels. LSC is of linear computational
complexity and high memory efficiency and is able to preserve global properties of images

 */

class CV_EXPORTS_W SuperpixelLSC : public Algorithm
{
public:

    /** @brief Calculates the actual amount of superpixels on a given segmentation computed
    and stored in SuperpixelLSC object.
     */
    CV_WRAP virtual int getNumberOfSuperpixels() const = 0;

    /** @brief Calculates the superpixel segmentation on a given image with the initialized
    parameters in the SuperpixelLSC object.

    This function can be called again without the need of initializing the algorithm with
    createSuperpixelLSC(). This save the computational cost of allocating memory for all the
    structures of the algorithm.

    @param num_iterations Number of iterations. Higher number improves the result.

    The function computes the superpixels segmentation of an image with the parameters initialized
    with the function createSuperpixelLSC(). The algorithms starts from a grid of superpixels and
    then refines the boundaries by proposing updates of edges boundaries.

     */
    CV_WRAP virtual void iterate( int num_iterations = 10 ) = 0;

    /** @brief Returns the segmentation labeling of the image.

    Each label represents a superpixel, and each pixel is assigned to one superpixel label.

    @param labels_out Return: A CV_32SC1 integer array containing the labels of the superpixel
    segmentation. The labels are in the range [0, getNumberOfSuperpixels()].

    The function returns an image with the labels of the superpixel segmentation. The labels are in
    the range [0, getNumberOfSuperpixels()].
     */
    CV_WRAP virtual void getLabels( OutputArray labels_out ) const = 0;

    /** @brief Returns the mask of the superpixel segmentation stored in SuperpixelLSC object.

    @param image Return: CV_8U1 image mask where -1 indicates that the pixel is a superpixel border,
    and 0 otherwise.

    @param thick_line If false, the border is only one pixel wide, otherwise all pixels at the border
    are masked.

    The function return the boundaries of the superpixel segmentation.
     */
    CV_WRAP virtual void getLabelContourMask( OutputArray image, bool thick_line = true ) const = 0;

    /** @brief Enforce label connectivity.

    @param min_element_size The minimum element size in percents that should be absorbed into a bigger
    superpixel. Given resulted average superpixel size valid value should be in 0-100 range, 25 means
    that less then a quarter sized superpixel should be absorbed, this is default.

    The function merge component that is too small, assigning the previously found adjacent label
    to this component. Calling this function may change the final number of superpixels.
     */
    CV_WRAP virtual void enforceLabelConnectivity( int min_element_size = 25 ) = 0;


};

/** @brief Class implementing the LSC (Linear Spectral Clustering) superpixels

@param image Image to segment
@param region_size Chooses an average superpixel size measured in pixels
@param ratio Chooses the enforcement of superpixel compactness factor of superpixel

The function initializes a SuperpixelLSC object for the input image. It sets the parameters of
superpixel algorithm, which are: region_size and ruler. It preallocate some buffers for future
computing iterations over the given image. An example of LSC is ilustrated in the following picture.
For enanched results it is recommended for color images to preprocess image with little gaussian blur
with a small 3 x 3 kernel and additional conversion into CieLAB color space.

![image](pics/superpixels_lsc.png)

 */

    CV_EXPORTS_W Ptr<SuperpixelLSC> createSuperpixelLSC( InputArray image, int region_size = 10, float ratio = 0.075f );

//! @}

}
}
#endif
#endif
