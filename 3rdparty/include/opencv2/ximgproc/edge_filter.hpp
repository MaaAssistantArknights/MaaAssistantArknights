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

#ifndef __OPENCV_EDGEFILTER_HPP__
#define __OPENCV_EDGEFILTER_HPP__
#ifdef __cplusplus

#include <opencv2/core.hpp>

namespace cv
{
namespace ximgproc
{

//! @addtogroup ximgproc_filters
//! @{

enum EdgeAwareFiltersList
{
    DTF_NC,
    DTF_IC,
    DTF_RF,

    GUIDED_FILTER,
    AM_FILTER
};


/** @brief Interface for realizations of Domain Transform filter.

For more details about this filter see @cite Gastal11 .
 */
class CV_EXPORTS_W DTFilter : public Algorithm
{
public:

    /** @brief Produce domain transform filtering operation on source image.

    @param src filtering image with unsigned 8-bit or floating-point 32-bit depth and up to 4 channels.

    @param dst destination image.

    @param dDepth optional depth of the output image. dDepth can be set to -1, which will be equivalent
    to src.depth().
     */
    CV_WRAP virtual void filter(InputArray src, OutputArray dst, int dDepth = -1) = 0;
};

/** @brief Factory method, create instance of DTFilter and produce initialization routines.

@param guide guided image (used to build transformed distance, which describes edge structure of
guided image).

@param sigmaSpatial \f${\sigma}_H\f$ parameter in the original article, it's similar to the sigma in the
coordinate space into bilateralFilter.

@param sigmaColor \f${\sigma}_r\f$ parameter in the original article, it's similar to the sigma in the
color space into bilateralFilter.

@param mode one form three modes DTF_NC, DTF_RF and DTF_IC which corresponds to three modes for
filtering 2D signals in the article.

@param numIters optional number of iterations used for filtering, 3 is quite enough.

For more details about Domain Transform filter parameters, see the original article @cite Gastal11 and
[Domain Transform filter homepage](http://www.inf.ufrgs.br/~eslgastal/DomainTransform/).
 */
CV_EXPORTS_W
Ptr<DTFilter> createDTFilter(InputArray guide, double sigmaSpatial, double sigmaColor, int mode = DTF_NC, int numIters = 3);

/** @brief Simple one-line Domain Transform filter call. If you have multiple images to filter with the same
guided image then use DTFilter interface to avoid extra computations on initialization stage.

@param guide guided image (also called as joint image) with unsigned 8-bit or floating-point 32-bit
depth and up to 4 channels.
@param src filtering image with unsigned 8-bit or floating-point 32-bit depth and up to 4 channels.
@param dst destination image
@param sigmaSpatial \f${\sigma}_H\f$ parameter in the original article, it's similar to the sigma in the
coordinate space into bilateralFilter.
@param sigmaColor \f${\sigma}_r\f$ parameter in the original article, it's similar to the sigma in the
color space into bilateralFilter.
@param mode one form three modes DTF_NC, DTF_RF and DTF_IC which corresponds to three modes for
filtering 2D signals in the article.
@param numIters optional number of iterations used for filtering, 3 is quite enough.
@sa bilateralFilter, guidedFilter, amFilter
 */
CV_EXPORTS_W
void dtFilter(InputArray guide, InputArray src, OutputArray dst, double sigmaSpatial, double sigmaColor, int mode = DTF_NC, int numIters = 3);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/** @brief Interface for realizations of Guided Filter.

For more details about this filter see @cite Kaiming10 .
 */
class CV_EXPORTS_W GuidedFilter : public Algorithm
{
public:

    /** @brief Apply Guided Filter to the filtering image.

    @param src filtering image with any numbers of channels.

    @param dst output image.

    @param dDepth optional depth of the output image. dDepth can be set to -1, which will be equivalent
    to src.depth().
     */
    CV_WRAP virtual void filter(InputArray src, OutputArray dst, int dDepth = -1) = 0;
};

/** @brief Factory method, create instance of GuidedFilter and produce initialization routines.

@param guide guided image (or array of images) with up to 3 channels, if it have more then 3
channels then only first 3 channels will be used.

@param radius radius of Guided Filter.

@param eps regularization term of Guided Filter. \f${eps}^2\f$ is similar to the sigma in the color
space into bilateralFilter.

For more details about Guided Filter parameters, see the original article @cite Kaiming10 .
 */
CV_EXPORTS_W Ptr<GuidedFilter> createGuidedFilter(InputArray guide, int radius, double eps);

/** @brief Simple one-line Guided Filter call.

If you have multiple images to filter with the same guided image then use GuidedFilter interface to
avoid extra computations on initialization stage.

@param guide guided image (or array of images) with up to 3 channels, if it have more then 3
channels then only first 3 channels will be used.

@param src filtering image with any numbers of channels.

@param dst output image.

@param radius radius of Guided Filter.

@param eps regularization term of Guided Filter. \f${eps}^2\f$ is similar to the sigma in the color
space into bilateralFilter.

@param dDepth optional depth of the output image.

@sa bilateralFilter, dtFilter, amFilter */
CV_EXPORTS_W void guidedFilter(InputArray guide, InputArray src, OutputArray dst, int radius, double eps, int dDepth = -1);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/** @brief Interface for Adaptive Manifold Filter realizations.

For more details about this filter see @cite Gastal12 and References_.

Below listed optional parameters which may be set up with Algorithm::set function.
-   member double sigma_s = 16.0
Spatial standard deviation.
-   member double sigma_r = 0.2
Color space standard deviation.
-   member int tree_height = -1
Height of the manifold tree (default = -1 : automatically computed).
-   member int num_pca_iterations = 1
Number of iterations to computed the eigenvector.
-   member bool adjust_outliers = false
Specify adjust outliers using Eq. 9 or not.
-   member bool use_RNG = true
Specify use random number generator to compute eigenvector or not.
 */
class CV_EXPORTS_W AdaptiveManifoldFilter : public Algorithm
{
public:
    /** @brief Apply high-dimensional filtering using adaptive manifolds.

    @param src filtering image with any numbers of channels.

    @param dst output image.

    @param joint optional joint (also called as guided) image with any numbers of channels.
     */
    CV_WRAP virtual void filter(InputArray src, OutputArray dst, InputArray joint = noArray()) = 0;

    CV_WRAP virtual void collectGarbage() = 0;

    CV_WRAP static Ptr<AdaptiveManifoldFilter> create();

    /** @see setSigmaS */
    virtual double getSigmaS() const = 0;
    /** @copybrief getSigmaS @see getSigmaS */
    virtual void setSigmaS(double val) = 0;
    /** @see setSigmaR */
    virtual double getSigmaR() const = 0;
    /** @copybrief getSigmaR @see getSigmaR */
    virtual void setSigmaR(double val) = 0;
    /** @see setTreeHeight */
    virtual int getTreeHeight() const = 0;
    /** @copybrief getTreeHeight @see getTreeHeight */
    virtual void setTreeHeight(int val) = 0;
    /** @see setPCAIterations */
    virtual int getPCAIterations() const = 0;
    /** @copybrief getPCAIterations @see getPCAIterations */
    virtual void setPCAIterations(int val) = 0;
    /** @see setAdjustOutliers */
    virtual bool getAdjustOutliers() const = 0;
    /** @copybrief getAdjustOutliers @see getAdjustOutliers */
    virtual void setAdjustOutliers(bool val) = 0;
    /** @see setUseRNG */
    virtual bool getUseRNG() const = 0;
    /** @copybrief getUseRNG @see getUseRNG */
    virtual void setUseRNG(bool val) = 0;
};

/** @brief Factory method, create instance of AdaptiveManifoldFilter and produce some initialization routines.

@param sigma_s spatial standard deviation.

@param sigma_r color space standard deviation, it is similar to the sigma in the color space into
bilateralFilter.

@param adjust_outliers optional, specify perform outliers adjust operation or not, (Eq. 9) in the
original paper.

For more details about Adaptive Manifold Filter parameters, see the original article @cite Gastal12 .

@note Joint images with CV_8U and CV_16U depth converted to images with CV_32F depth and [0; 1]
color range before processing. Hence color space sigma sigma_r must be in [0; 1] range, unlike same
sigmas in bilateralFilter and dtFilter functions.
*/
CV_EXPORTS_W Ptr<AdaptiveManifoldFilter> createAMFilter(double sigma_s, double sigma_r, bool adjust_outliers = false);

/** @brief Simple one-line Adaptive Manifold Filter call.

@param joint joint (also called as guided) image or array of images with any numbers of channels.

@param src filtering image with any numbers of channels.

@param dst output image.

@param sigma_s spatial standard deviation.

@param sigma_r color space standard deviation, it is similar to the sigma in the color space into
bilateralFilter.

@param adjust_outliers optional, specify perform outliers adjust operation or not, (Eq. 9) in the
original paper.

@note Joint images with CV_8U and CV_16U depth converted to images with CV_32F depth and [0; 1]
color range before processing. Hence color space sigma sigma_r must be in [0; 1] range, unlike same
sigmas in bilateralFilter and dtFilter functions. @sa bilateralFilter, dtFilter, guidedFilter
*/
CV_EXPORTS_W void amFilter(InputArray joint, InputArray src, OutputArray dst, double sigma_s, double sigma_r, bool adjust_outliers = false);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/** @brief Applies the joint bilateral filter to an image.

@param joint Joint 8-bit or floating-point, 1-channel or 3-channel image.

@param src Source 8-bit or floating-point, 1-channel or 3-channel image with the same depth as joint
image.

@param dst Destination image of the same size and type as src .

@param d Diameter of each pixel neighborhood that is used during filtering. If it is non-positive,
it is computed from sigmaSpace .

@param sigmaColor Filter sigma in the color space. A larger value of the parameter means that
farther colors within the pixel neighborhood (see sigmaSpace ) will be mixed together, resulting in
larger areas of semi-equal color.

@param sigmaSpace Filter sigma in the coordinate space. A larger value of the parameter means that
farther pixels will influence each other as long as their colors are close enough (see sigmaColor ).
When d\>0 , it specifies the neighborhood size regardless of sigmaSpace . Otherwise, d is
proportional to sigmaSpace .

@param borderType

@note bilateralFilter and jointBilateralFilter use L1 norm to compute difference between colors.

@sa bilateralFilter, amFilter
*/
CV_EXPORTS_W
void jointBilateralFilter(InputArray joint, InputArray src, OutputArray dst, int d, double sigmaColor, double sigmaSpace, int borderType = BORDER_DEFAULT);

/** @brief Applies the bilateral texture filter to an image. It performs structure-preserving texture filter.
For more details about this filter see @cite Cho2014.

@param src Source image whose depth is 8-bit UINT or 32-bit FLOAT

@param dst Destination image of the same size and type as src.

@param fr Radius of kernel to be used for filtering. It should be positive integer

@param numIter Number of iterations of algorithm, It should be positive integer

@param sigmaAlpha Controls the sharpness of the weight transition from edges to smooth/texture regions, where
a bigger value means sharper transition. When the value is negative, it is automatically calculated.

@param sigmaAvg Range blur parameter for texture blurring. Larger value makes result to be more blurred. When the
value is negative, it is automatically calculated as described in the paper.

@sa rollingGuidanceFilter, bilateralFilter
*/
CV_EXPORTS_W
void bilateralTextureFilter(InputArray src, OutputArray dst, int fr = 3, int numIter = 1, double sigmaAlpha = -1., double sigmaAvg = -1.);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/** @brief Applies the rolling guidance filter to an image.

For more details, please see @cite zhang2014rolling

@param src Source 8-bit or floating-point, 1-channel or 3-channel image.

@param dst Destination image of the same size and type as src.

@param d Diameter of each pixel neighborhood that is used during filtering. If it is non-positive,
it is computed from sigmaSpace .

@param sigmaColor Filter sigma in the color space. A larger value of the parameter means that
farther colors within the pixel neighborhood (see sigmaSpace ) will be mixed together, resulting in
larger areas of semi-equal color.

@param sigmaSpace Filter sigma in the coordinate space. A larger value of the parameter means that
farther pixels will influence each other as long as their colors are close enough (see sigmaColor ).
When d\>0 , it specifies the neighborhood size regardless of sigmaSpace . Otherwise, d is
proportional to sigmaSpace .

@param numOfIter Number of iterations of joint edge-preserving filtering applied on the source image.

@param borderType

@note  rollingGuidanceFilter uses jointBilateralFilter as the edge-preserving filter.

@sa jointBilateralFilter, bilateralFilter, amFilter
*/
CV_EXPORTS_W
void rollingGuidanceFilter(InputArray src, OutputArray dst, int d = -1, double sigmaColor = 25, double sigmaSpace = 3, int numOfIter = 4, int borderType = BORDER_DEFAULT);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/** @brief Interface for implementations of Fast Bilateral Solver.

For more details about this solver see @cite BarronPoole2016 .
*/
class CV_EXPORTS_W FastBilateralSolverFilter : public Algorithm
{
public:
    /** @brief Apply smoothing operation to the source image.

    @param src source image for filtering with unsigned 8-bit or signed 16-bit or floating-point 32-bit depth and up to 3 channels.

    @param confidence confidence image with unsigned 8-bit or floating-point 32-bit confidence and 1 channel.

    @param dst destination image.

    @note Confidence images with CV_8U depth are expected to in [0, 255] and CV_32F in [0, 1] range.
    */
    CV_WRAP virtual void filter(InputArray src, InputArray confidence, OutputArray dst) = 0;
};

/** @brief Factory method, create instance of FastBilateralSolverFilter and execute the initialization routines.

@param guide image serving as guide for filtering. It should have 8-bit depth and either 1 or 3 channels.

@param sigma_spatial parameter, that is similar to spatial space sigma (bandwidth) in bilateralFilter.

@param sigma_luma parameter, that is similar to luma space sigma (bandwidth) in bilateralFilter.

@param sigma_chroma parameter, that is similar to chroma space sigma (bandwidth) in bilateralFilter.

@param lambda smoothness strength parameter for solver.

@param num_iter number of iterations used for solver, 25 is usually enough.

@param max_tol convergence tolerance used for solver.

For more details about the Fast Bilateral Solver parameters, see the original paper @cite BarronPoole2016.

*/
CV_EXPORTS_W Ptr<FastBilateralSolverFilter> createFastBilateralSolverFilter(InputArray guide, double sigma_spatial, double sigma_luma, double sigma_chroma, double lambda = 128.0, int num_iter = 25, double max_tol = 1e-5);


/** @brief Simple one-line Fast Bilateral Solver filter call. If you have multiple images to filter with the same
guide then use FastBilateralSolverFilter interface to avoid extra computations.

@param guide image serving as guide for filtering. It should have 8-bit depth and either 1 or 3 channels.

@param src source image for filtering with unsigned 8-bit or signed 16-bit or floating-point 32-bit depth and up to 4 channels.

@param confidence confidence image with unsigned 8-bit or floating-point 32-bit confidence and 1 channel.

@param dst destination image.

@param sigma_spatial parameter, that is similar to spatial space sigma (bandwidth) in bilateralFilter.

@param sigma_luma parameter, that is similar to luma space sigma (bandwidth) in bilateralFilter.

@param sigma_chroma parameter, that is similar to chroma space sigma (bandwidth) in bilateralFilter.

@param lambda smoothness strength parameter for solver.

@param num_iter number of iterations used for solver, 25 is usually enough.

@param max_tol convergence tolerance used for solver.

For more details about the Fast Bilateral Solver parameters, see the original paper @cite BarronPoole2016.

@note Confidence images with CV_8U depth are expected to in [0, 255] and CV_32F in [0, 1] range.
*/
CV_EXPORTS_W void fastBilateralSolverFilter(InputArray guide, InputArray src, InputArray confidence, OutputArray dst, double sigma_spatial = 8, double sigma_luma = 8, double sigma_chroma = 8, double lambda = 128.0, int num_iter = 25, double max_tol = 1e-5);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/** @brief Interface for implementations of Fast Global Smoother filter.

For more details about this filter see @cite Min2014 and @cite Farbman2008 .
*/
class CV_EXPORTS_W FastGlobalSmootherFilter : public Algorithm
{
public:
    /** @brief Apply smoothing operation to the source image.

    @param src source image for filtering with unsigned 8-bit or signed 16-bit or floating-point 32-bit depth and up to 4 channels.

    @param dst destination image.
    */
    CV_WRAP virtual void filter(InputArray src, OutputArray dst) = 0;
};

/** @brief Factory method, create instance of FastGlobalSmootherFilter and execute the initialization routines.

@param guide image serving as guide for filtering. It should have 8-bit depth and either 1 or 3 channels.

@param lambda parameter defining the amount of regularization

@param sigma_color parameter, that is similar to color space sigma in bilateralFilter.

@param lambda_attenuation internal parameter, defining how much lambda decreases after each iteration. Normally,
it should be 0.25. Setting it to 1.0 may lead to streaking artifacts.

@param num_iter number of iterations used for filtering, 3 is usually enough.

For more details about Fast Global Smoother parameters, see the original paper @cite Min2014. However, please note that
there are several differences. Lambda attenuation described in the paper is implemented a bit differently so do not
expect the results to be identical to those from the paper; sigma_color values from the paper should be multiplied by 255.0 to
achieve the same effect. Also, in case of image filtering where source and guide image are the same, authors
propose to dynamically update the guide image after each iteration. To maximize the performance this feature
was not implemented here.
*/
CV_EXPORTS_W Ptr<FastGlobalSmootherFilter> createFastGlobalSmootherFilter(InputArray guide, double lambda, double sigma_color, double lambda_attenuation=0.25, int num_iter=3);

/** @brief Simple one-line Fast Global Smoother filter call. If you have multiple images to filter with the same
guide then use FastGlobalSmootherFilter interface to avoid extra computations.

@param guide image serving as guide for filtering. It should have 8-bit depth and either 1 or 3 channels.

@param src source image for filtering with unsigned 8-bit or signed 16-bit or floating-point 32-bit depth and up to 4 channels.

@param dst destination image.

@param lambda parameter defining the amount of regularization

@param sigma_color parameter, that is similar to color space sigma in bilateralFilter.

@param lambda_attenuation internal parameter, defining how much lambda decreases after each iteration. Normally,
it should be 0.25. Setting it to 1.0 may lead to streaking artifacts.

@param num_iter number of iterations used for filtering, 3 is usually enough.
*/
CV_EXPORTS_W void fastGlobalSmootherFilter(InputArray guide, InputArray src, OutputArray dst, double lambda, double sigma_color, double lambda_attenuation=0.25, int num_iter=3);

/** @brief Global image smoothing via L0 gradient minimization.

@param src source image for filtering with unsigned 8-bit or signed 16-bit or floating-point depth.

@param dst destination image.

@param lambda parameter defining the smooth term weight.

@param kappa parameter defining the increasing factor of the weight of the gradient data term.

For more details about L0 Smoother, see the original paper @cite xu2011image.
*/
CV_EXPORTS_W void l0Smooth(InputArray src, OutputArray dst, double lambda = 0.02, double kappa = 2.0);
//! @}
}
}
#endif
#endif
