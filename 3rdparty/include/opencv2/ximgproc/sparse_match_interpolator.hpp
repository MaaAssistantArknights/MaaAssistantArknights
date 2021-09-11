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

#ifndef __OPENCV_SPARSEMATCHINTERPOLATOR_HPP__
#define __OPENCV_SPARSEMATCHINTERPOLATOR_HPP__
#ifdef __cplusplus

#include <opencv2/core.hpp>

namespace cv {
namespace ximgproc {

//! @addtogroup ximgproc_filters
//! @{

/** @brief Main interface for all filters, that take sparse matches as an
input and produce a dense per-pixel matching (optical flow) as an output.
 */
class CV_EXPORTS_W SparseMatchInterpolator : public Algorithm
{
public:
    /** @brief Interpolate input sparse matches.

    @param from_image first of the two matched images, 8-bit single-channel or three-channel.

    @param from_points points of the from_image for which there are correspondences in the
    to_image (Point2f vector, size shouldn't exceed 32767)

    @param to_image second of the two matched images, 8-bit single-channel or three-channel.

    @param to_points points in the to_image corresponding to from_points
    (Point2f vector, size shouldn't exceed 32767)

    @param dense_flow output dense matching (two-channel CV_32F image)
     */
    CV_WRAP virtual void interpolate(InputArray from_image, InputArray from_points,
                                     InputArray to_image  , InputArray to_points,
                                     OutputArray dense_flow) = 0;
};

/** @brief Sparse match interpolation algorithm based on modified locally-weighted affine
estimator from @cite Revaud2015 and Fast Global Smoother as post-processing filter.
 */
class CV_EXPORTS_W EdgeAwareInterpolator : public SparseMatchInterpolator
{
public:
    /** @brief K is a number of nearest-neighbor matches considered, when fitting a locally affine
    model. Usually it should be around 128. However, lower values would make the interpolation
    noticeably faster.
     */
    CV_WRAP virtual void setK(int _k) = 0;
    /** @see setK */
    CV_WRAP virtual int  getK() = 0;

    /** @brief Sigma is a parameter defining how fast the weights decrease in the locally-weighted affine
    fitting. Higher values can help preserve fine details, lower values can help to get rid of noise in the
    output flow.
     */
    CV_WRAP virtual void  setSigma(float _sigma) = 0;
    /** @see setSigma */
    CV_WRAP virtual float getSigma() = 0;

    /** @brief Lambda is a parameter defining the weight of the edge-aware term in geodesic distance,
    should be in the range of 0 to 1000.
     */
    CV_WRAP virtual void  setLambda(float _lambda) = 0;
    /** @see setLambda */
    CV_WRAP virtual float getLambda() = 0;

    /** @brief Sets whether the fastGlobalSmootherFilter() post-processing is employed. It is turned on by
    default.
     */
    CV_WRAP virtual void setUsePostProcessing(bool _use_post_proc) = 0;
    /** @see setUsePostProcessing */
    CV_WRAP virtual bool getUsePostProcessing() = 0;

    /** @brief Sets the respective fastGlobalSmootherFilter() parameter.
     */
    CV_WRAP virtual void  setFGSLambda(float _lambda) = 0;
    /** @see setFGSLambda */
    CV_WRAP virtual float getFGSLambda() = 0;

    /** @see setFGSLambda */
    CV_WRAP virtual void  setFGSSigma(float _sigma) = 0;
    /** @see setFGSLambda */
    CV_WRAP virtual float getFGSSigma() = 0;
};

/** @brief Factory method that creates an instance of the
EdgeAwareInterpolator.
*/
CV_EXPORTS_W
Ptr<EdgeAwareInterpolator> createEdgeAwareInterpolator();

//! @}
}
}
#endif
#endif
