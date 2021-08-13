/*
By downloading, copying, installing or using the software you agree to this
license. If you do not agree to this license, do not download, install,
copy or use the software.


                          License Agreement
               For Open Source Computer Vision Library
                       (3-clause BSD License)

Copyright (C) 2013, OpenCV Foundation, all rights reserved.
Third party copyrights are property of their respective owners.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  * Neither the names of the copyright holders nor the names of the contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

This software is provided by the copyright holders and contributors "as is" and
any express or implied warranties, including, but not limited to, the implied
warranties of merchantability and fitness for a particular purpose are
disclaimed. In no event shall copyright holders or contributors be liable for
any direct, indirect, incidental, special, exemplary, or consequential damages
(including, but not limited to, procurement of substitute goods or services;
loss of use, data, or profits; or business interruption) however caused
and on any theory of liability, whether in contract, strict liability,
or tort (including negligence or otherwise) arising in any way out of
the use of this software, even if advised of the possibility of such damage.
*/

#ifndef __OPENCV_OPTFLOW_HPP__
#define __OPENCV_OPTFLOW_HPP__

#include "opencv2/core.hpp"
#include "opencv2/video.hpp"

/**
@defgroup optflow Optical Flow Algorithms

Dense optical flow algorithms compute motion for each point:

- cv::optflow::calcOpticalFlowSF
- cv::optflow::createOptFlow_DeepFlow

Motion templates is alternative technique for detecting motion and computing its direction.
See samples/motempl.py.

- cv::motempl::updateMotionHistory
- cv::motempl::calcMotionGradient
- cv::motempl::calcGlobalOrientation
- cv::motempl::segmentMotion

Functions reading and writing .flo files in "Middlebury" format, see: <http://vision.middlebury.edu/flow/code/flow-code/README.txt>

- cv::optflow::readOpticalFlow
- cv::optflow::writeOpticalFlow

 */

#include "opencv2/optflow/pcaflow.hpp"
#include "opencv2/optflow/sparse_matching_gpc.hpp"

namespace cv
{
namespace optflow
{
    
//! @addtogroup optflow
//! @{

/** @overload */
CV_EXPORTS_W void calcOpticalFlowSF( InputArray from, InputArray to, OutputArray flow,
                                     int layers, int averaging_block_size, int max_flow);

/** @brief Calculate an optical flow using "SimpleFlow" algorithm.

@param from First 8-bit 3-channel image.
@param to Second 8-bit 3-channel image of the same size as prev
@param flow computed flow image that has the same size as prev and type CV_32FC2
@param layers Number of layers
@param averaging_block_size Size of block through which we sum up when calculate cost function
for pixel
@param max_flow maximal flow that we search at each level
@param sigma_dist vector smooth spatial sigma parameter
@param sigma_color vector smooth color sigma parameter
@param postprocess_window window size for postprocess cross bilateral filter
@param sigma_dist_fix spatial sigma for postprocess cross bilateralf filter
@param sigma_color_fix color sigma for postprocess cross bilateral filter
@param occ_thr threshold for detecting occlusions
@param upscale_averaging_radius window size for bilateral upscale operation
@param upscale_sigma_dist spatial sigma for bilateral upscale operation
@param upscale_sigma_color color sigma for bilateral upscale operation
@param speed_up_thr threshold to detect point with irregular flow - where flow should be
recalculated after upscale

See @cite Tao2012 . And site of project - <http://graphics.berkeley.edu/papers/Tao-SAN-2012-05/>.

@note
   -   An example using the simpleFlow algorithm can be found at samples/simpleflow_demo.cpp
 */
CV_EXPORTS_W void calcOpticalFlowSF( InputArray from, InputArray to, OutputArray flow, int layers,
                                     int averaging_block_size, int max_flow,
                                     double sigma_dist, double sigma_color, int postprocess_window,
                                     double sigma_dist_fix, double sigma_color_fix, double occ_thr,
                                     int upscale_averaging_radius, double upscale_sigma_dist,
                                     double upscale_sigma_color, double speed_up_thr );

/** @brief Fast dense optical flow based on PyrLK sparse matches interpolation.

@param from first 8-bit 3-channel or 1-channel image.
@param to  second 8-bit 3-channel or 1-channel image of the same size as from
@param flow computed flow image that has the same size as from and CV_32FC2 type
@param grid_step stride used in sparse match computation. Lower values usually
       result in higher quality but slow down the algorithm.
@param k number of nearest-neighbor matches considered, when fitting a locally affine
       model. Lower values can make the algorithm noticeably faster at the cost of
       some quality degradation.
@param sigma parameter defining how fast the weights decrease in the locally-weighted affine
       fitting. Higher values can help preserve fine details, lower values can help to get rid
       of the noise in the output flow.
@param use_post_proc defines whether the ximgproc::fastGlobalSmootherFilter() is used
       for post-processing after interpolation
@param fgs_lambda see the respective parameter of the ximgproc::fastGlobalSmootherFilter()
@param fgs_sigma  see the respective parameter of the ximgproc::fastGlobalSmootherFilter()
 */
CV_EXPORTS_W void calcOpticalFlowSparseToDense ( InputArray from, InputArray to, OutputArray flow,
                                                 int grid_step = 8, int k = 128, float sigma = 0.05f,
                                                 bool use_post_proc = true, float fgs_lambda = 500.0f,
                                                 float fgs_sigma = 1.5f );

/** @brief Read a .flo file

@param path Path to the file to be loaded

The function readOpticalFlow loads a flow field from a file and returns it as a single matrix.
Resulting Mat has a type CV_32FC2 - floating-point, 2-channel. First channel corresponds to the
flow in the horizontal direction (u), second - vertical (v).
 */
CV_EXPORTS_W Mat readOpticalFlow( const String& path );
/** @brief Write a .flo to disk

@param path Path to the file to be written
@param flow Flow field to be stored

The function stores a flow field in a file, returns true on success, false otherwise.
The flow field must be a 2-channel, floating-point matrix (CV_32FC2). First channel corresponds
to the flow in the horizontal direction (u), second - vertical (v).
 */
CV_EXPORTS_W bool writeOpticalFlow( const String& path, InputArray flow );

/** @brief Variational optical flow refinement

This class implements variational refinement of the input flow field, i.e.
it uses input flow to initialize the minimization of the following functional:
\f$E(U) = \int_{\Omega} \delta \Psi(E_I) + \gamma \Psi(E_G) + \alpha \Psi(E_S) \f$,
where \f$E_I,E_G,E_S\f$ are color constancy, gradient constancy and smoothness terms
respectively. \f$\Psi(s^2)=\sqrt{s^2+\epsilon^2}\f$ is a robust penalizer to limit the
influence of outliers. A complete formulation and a description of the minimization
procedure can be found in @cite Brox2004
*/
class CV_EXPORTS_W VariationalRefinement : public DenseOpticalFlow
{
public:
    /** @brief @ref calc function overload to handle separate horizontal (u) and vertical (v) flow components
    (to avoid extra splits/merges) */
    CV_WRAP virtual void calcUV(InputArray I0, InputArray I1, InputOutputArray flow_u, InputOutputArray flow_v) = 0;

    /** @brief Number of outer (fixed-point) iterations in the minimization procedure.
    @see setFixedPointIterations */
    CV_WRAP virtual int getFixedPointIterations() const = 0;
    /** @copybrief getFixedPointIterations @see getFixedPointIterations */
    CV_WRAP virtual void setFixedPointIterations(int val) = 0;

    /** @brief Number of inner successive over-relaxation (SOR) iterations
        in the minimization procedure to solve the respective linear system.
    @see setSorIterations */
    CV_WRAP virtual int getSorIterations() const = 0;
    /** @copybrief getSorIterations @see getSorIterations */
    CV_WRAP virtual void setSorIterations(int val) = 0;

    /** @brief Relaxation factor in SOR
    @see setOmega */
    CV_WRAP virtual float getOmega() const = 0;
    /** @copybrief getOmega @see getOmega */
    CV_WRAP virtual void setOmega(float val) = 0;

    /** @brief Weight of the smoothness term
    @see setAlpha */
    CV_WRAP virtual float getAlpha() const = 0;
    /** @copybrief getAlpha @see getAlpha */
    CV_WRAP virtual void setAlpha(float val) = 0;

    /** @brief Weight of the color constancy term
    @see setDelta */
    CV_WRAP virtual float getDelta() const = 0;
    /** @copybrief getDelta @see getDelta */
    CV_WRAP virtual void setDelta(float val) = 0;

    /** @brief Weight of the gradient constancy term
    @see setGamma */
    CV_WRAP virtual float getGamma() const = 0;
    /** @copybrief getGamma @see getGamma */
    CV_WRAP virtual void setGamma(float val) = 0;
};

/** @brief Creates an instance of VariationalRefinement
*/
CV_EXPORTS_W Ptr<VariationalRefinement> createVariationalFlowRefinement();

/** @brief DeepFlow optical flow algorithm implementation.

The class implements the DeepFlow optical flow algorithm described in @cite Weinzaepfel2013 . See
also <http://lear.inrialpes.fr/src/deepmatching/> .
Parameters - class fields - that may be modified after creating a class instance:
-   member float alpha
Smoothness assumption weight
-   member float delta
Color constancy assumption weight
-   member float gamma
Gradient constancy weight
-   member float sigma
Gaussian smoothing parameter
-   member int minSize
Minimal dimension of an image in the pyramid (next, smaller images in the pyramid are generated
until one of the dimensions reaches this size)
-   member float downscaleFactor
Scaling factor in the image pyramid (must be \< 1)
-   member int fixedPointIterations
How many iterations on each level of the pyramid
-   member int sorIterations
Iterations of Succesive Over-Relaxation (solver)
-   member float omega
Relaxation factor in SOR
 */
CV_EXPORTS_W Ptr<DenseOpticalFlow> createOptFlow_DeepFlow();

//! Additional interface to the SimpleFlow algorithm - calcOpticalFlowSF()
CV_EXPORTS_W Ptr<DenseOpticalFlow> createOptFlow_SimpleFlow();

//! Additional interface to the Farneback's algorithm - calcOpticalFlowFarneback()
CV_EXPORTS_W Ptr<DenseOpticalFlow> createOptFlow_Farneback();

//! Additional interface to the SparseToDenseFlow algorithm - calcOpticalFlowSparseToDense()
CV_EXPORTS_W Ptr<DenseOpticalFlow> createOptFlow_SparseToDense();

/** @brief DIS optical flow algorithm.

This class implements the Dense Inverse Search (DIS) optical flow algorithm. More
details about the algorithm can be found at @cite Kroeger2016 . Includes three presets with preselected
parameters to provide reasonable trade-off between speed and quality. However, even the slowest preset is
still relatively fast, use DeepFlow if you need better quality and don't care about speed.

This implementation includes several additional features compared to the algorithm described in the paper,
including spatial propagation of flow vectors (@ref getUseSpatialPropagation), as well as an option to
utilize an initial flow approximation passed to @ref calc (which is, essentially, temporal propagation,
if the previous frame's flow field is passed).
*/
class CV_EXPORTS_W DISOpticalFlow : public DenseOpticalFlow
{
public:
    enum
    {
        PRESET_ULTRAFAST = 0,
        PRESET_FAST = 1,
        PRESET_MEDIUM = 2
    };

    /** @brief Finest level of the Gaussian pyramid on which the flow is computed (zero level
        corresponds to the original image resolution). The final flow is obtained by bilinear upscaling.
        @see setFinestScale */
    CV_WRAP virtual int getFinestScale() const = 0;
    /** @copybrief getFinestScale @see getFinestScale */
    CV_WRAP virtual void setFinestScale(int val) = 0;

    /** @brief Size of an image patch for matching (in pixels). Normally, default 8x8 patches work well
        enough in most cases.
        @see setPatchSize */
    CV_WRAP virtual int getPatchSize() const = 0;
    /** @copybrief getPatchSize @see getPatchSize */
    CV_WRAP virtual void setPatchSize(int val) = 0;

    /** @brief Stride between neighbor patches. Must be less than patch size. Lower values correspond
        to higher flow quality.
        @see setPatchStride */
    CV_WRAP virtual int getPatchStride() const = 0;
    /** @copybrief getPatchStride @see getPatchStride */
    CV_WRAP virtual void setPatchStride(int val) = 0;

    /** @brief Maximum number of gradient descent iterations in the patch inverse search stage. Higher values
        may improve quality in some cases.
        @see setGradientDescentIterations */
    CV_WRAP virtual int getGradientDescentIterations() const = 0;
    /** @copybrief getGradientDescentIterations @see getGradientDescentIterations */
    CV_WRAP virtual void setGradientDescentIterations(int val) = 0;

    /** @brief Number of fixed point iterations of variational refinement per scale. Set to zero to
        disable variational refinement completely. Higher values will typically result in more smooth and
        high-quality flow.
    @see setGradientDescentIterations */
    CV_WRAP virtual int getVariationalRefinementIterations() const = 0;
    /** @copybrief getGradientDescentIterations @see getGradientDescentIterations */
    CV_WRAP virtual void setVariationalRefinementIterations(int val) = 0;

    /** @brief Weight of the smoothness term
    @see setVariationalRefinementAlpha */
    CV_WRAP virtual float getVariationalRefinementAlpha() const = 0;
    /** @copybrief getVariationalRefinementAlpha @see getVariationalRefinementAlpha */
    CV_WRAP virtual void setVariationalRefinementAlpha(float val) = 0;

    /** @brief Weight of the color constancy term
    @see setVariationalRefinementDelta */
    CV_WRAP virtual float getVariationalRefinementDelta() const = 0;
    /** @copybrief getVariationalRefinementDelta @see getVariationalRefinementDelta */
    CV_WRAP virtual void setVariationalRefinementDelta(float val) = 0;

    /** @brief Weight of the gradient constancy term
    @see setVariationalRefinementGamma */
    CV_WRAP virtual float getVariationalRefinementGamma() const = 0;
    /** @copybrief getVariationalRefinementGamma @see getVariationalRefinementGamma */
    CV_WRAP virtual void setVariationalRefinementGamma(float val) = 0;


    /** @brief Whether to use mean-normalization of patches when computing patch distance. It is turned on
        by default as it typically provides a noticeable quality boost because of increased robustness to
        illumination variations. Turn it off if you are certain that your sequence doesn't contain any changes
        in illumination.
    @see setUseMeanNormalization */
    CV_WRAP virtual bool getUseMeanNormalization() const = 0;
    /** @copybrief getUseMeanNormalization @see getUseMeanNormalization */
    CV_WRAP virtual void setUseMeanNormalization(bool val) = 0;

    /** @brief Whether to use spatial propagation of good optical flow vectors. This option is turned on by
        default, as it tends to work better on average and can sometimes help recover from major errors
        introduced by the coarse-to-fine scheme employed by the DIS optical flow algorithm. Turning this
        option off can make the output flow field a bit smoother, however.
    @see setUseSpatialPropagation */
    CV_WRAP virtual bool getUseSpatialPropagation() const = 0;
    /** @copybrief getUseSpatialPropagation @see getUseSpatialPropagation */
    CV_WRAP virtual void setUseSpatialPropagation(bool val) = 0;
};

/** @brief Creates an instance of DISOpticalFlow

@param preset one of PRESET_ULTRAFAST, PRESET_FAST and PRESET_MEDIUM
*/
CV_EXPORTS_W Ptr<DISOpticalFlow> createOptFlow_DIS(int preset = DISOpticalFlow::PRESET_FAST);

//! @}

} //optflow
}

#include "opencv2/optflow/motempl.hpp"

#endif
