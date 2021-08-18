/*********************************************************************
 * Software License Agreement (BSD License)
 *
 * Copyright (c) 2013
 * Radhakrishna Achanta
 * email : Radhakrishna [dot] Achanta [at] epfl [dot] ch
 * web : http://ivrl.epfl.ch/people/achanta
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
 "SLIC Superpixels Compared to State-of-the-art Superpixel Methods"
 Radhakrishna Achanta, Appu Shaji, Kevin Smith, Aurelien Lucchi, Pascal Fua,
 and Sabine Susstrunk, IEEE TPAMI, Volume 34, Issue 11, Pages 2274-2282,
 November 2012.

 "SLIC Superpixels" Radhakrishna Achanta, Appu Shaji, Kevin Smith,
 Aurelien Lucchi, Pascal Fua, and Sabine Süsstrunk, EPFL Technical
 Report no. 149300, June 2010.

 OpenCV port by: Cristian Balint <cristian dot balint at gmail dot com>
 */

#ifndef __OPENCV_SLIC_HPP__
#define __OPENCV_SLIC_HPP__
#ifdef __cplusplus

#include <opencv2/core.hpp>

namespace cv
{
namespace ximgproc
{

//! @addtogroup ximgproc_superpixel
//! @{

    enum SLIC { SLIC = 100, SLICO = 101, MSLIC = 102 };
    typedef enum SLIC SLICType;

/** @brief Class implementing the SLIC (Simple Linear Iterative Clustering) superpixels
algorithm described in @cite Achanta2012.

SLIC (Simple Linear Iterative Clustering) clusters pixels using pixel channels and image plane space
to efficiently generate compact, nearly uniform superpixels. The simplicity of approach makes it
extremely easy to use a lone parameter specifies the number of superpixels and the efficiency of
the algorithm makes it very practical.
Several optimizations are available for SLIC class:
SLICO stands for "Zero parameter SLIC" and it is an optimization of baseline SLIC described in @cite Achanta2012.
MSLIC stands for "Manifold SLIC" and it is an optimization of baseline SLIC described in @cite Liu_2017_IEEE.
 */

class CV_EXPORTS_W SuperpixelSLIC : public Algorithm
{
public:

    /** @brief Calculates the actual amount of superpixels on a given segmentation computed
    and stored in SuperpixelSLIC object.
     */
    CV_WRAP virtual int getNumberOfSuperpixels() const = 0;

    /** @brief Calculates the superpixel segmentation on a given image with the initialized
    parameters in the SuperpixelSLIC object.

    This function can be called again without the need of initializing the algorithm with
    createSuperpixelSLIC(). This save the computational cost of allocating memory for all the
    structures of the algorithm.

    @param num_iterations Number of iterations. Higher number improves the result.

    The function computes the superpixels segmentation of an image with the parameters initialized
    with the function createSuperpixelSLIC(). The algorithms starts from a grid of superpixels and
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

    /** @brief Returns the mask of the superpixel segmentation stored in SuperpixelSLIC object.

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

/** @brief Initialize a SuperpixelSLIC object

@param image Image to segment
@param algorithm Chooses the algorithm variant to use:
SLIC segments image using a desired region_size, and in addition SLICO will optimize using adaptive compactness factor,
while MSLIC will optimize using manifold methods resulting in more content-sensitive superpixels.
@param region_size Chooses an average superpixel size measured in pixels
@param ruler Chooses the enforcement of superpixel smoothness factor of superpixel

The function initializes a SuperpixelSLIC object for the input image. It sets the parameters of choosed
superpixel algorithm, which are: region_size and ruler. It preallocate some buffers for future
computing iterations over the given image. For enanched results it is recommended for color images to
preprocess image with little gaussian blur using a small 3 x 3 kernel and additional conversion into
CieLAB color space. An example of SLIC versus SLICO and MSLIC is ilustrated in the following picture.

![image](pics/superpixels_slic.png)

 */

    CV_EXPORTS_W Ptr<SuperpixelSLIC> createSuperpixelSLIC( InputArray image, int algorithm = SLICO,
                                                           int region_size = 10, float ruler = 10.0f );

//! @}

}
}
#endif
#endif
