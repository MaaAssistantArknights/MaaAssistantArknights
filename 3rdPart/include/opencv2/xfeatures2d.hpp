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

#ifndef __OPENCV_XFEATURES2D_HPP__
#define __OPENCV_XFEATURES2D_HPP__

#include "opencv2/features2d.hpp"
#include "opencv2/xfeatures2d/nonfree.hpp"

/** @defgroup xfeatures2d Extra 2D Features Framework
@{
    @defgroup xfeatures2d_experiment Experimental 2D Features Algorithms

This section describes experimental algorithms for 2d feature detection.

    @defgroup xfeatures2d_nonfree Non-free 2D Features Algorithms

This section describes two popular algorithms for 2d feature detection, SIFT and SURF, that are
known to be patented. You need to set the OPENCV_ENABLE_NONFREE option in cmake to use those. Use them at your own risk.

    @defgroup xfeatures2d_match Experimental 2D Features Matching Algorithm

This section describes the GMS (Grid-based Motion Statistics) matching strategy.

@}
*/

namespace cv
{
namespace xfeatures2d
{

//! @addtogroup xfeatures2d_experiment
//! @{

/** @brief Class implementing the FREAK (*Fast Retina Keypoint*) keypoint descriptor, described in @cite AOV12 .

The algorithm propose a novel keypoint descriptor inspired by the human visual system and more
precisely the retina, coined Fast Retina Key- point (FREAK). A cascade of binary strings is
computed by efficiently comparing image intensities over a retinal sampling pattern. FREAKs are in
general faster to compute with lower memory load and also more robust than SIFT, SURF or BRISK.
They are competitive alternatives to existing keypoints in particular for embedded applications.

@note
   -   An example on how to use the FREAK descriptor can be found at
        opencv_source_code/samples/cpp/freak_demo.cpp
 */
class CV_EXPORTS_W FREAK : public Feature2D
{
public:

    enum
    {
        NB_SCALES = 64, NB_PAIRS = 512, NB_ORIENPAIRS = 45
    };

    /**
    @param orientationNormalized Enable orientation normalization.
    @param scaleNormalized Enable scale normalization.
    @param patternScale Scaling of the description pattern.
    @param nOctaves Number of octaves covered by the detected keypoints.
    @param selectedPairs (Optional) user defined selected pairs indexes,
     */
    CV_WRAP static Ptr<FREAK> create(bool orientationNormalized = true,
                             bool scaleNormalized = true,
                             float patternScale = 22.0f,
                             int nOctaves = 4,
                             const std::vector<int>& selectedPairs = std::vector<int>());
};


/** @brief The class implements the keypoint detector introduced by @cite Agrawal08, synonym of StarDetector. :
 */
class CV_EXPORTS_W StarDetector : public Feature2D
{
public:
    //! the full constructor
    CV_WRAP static Ptr<StarDetector> create(int maxSize=45, int responseThreshold=30,
                         int lineThresholdProjected=10,
                         int lineThresholdBinarized=8,
                         int suppressNonmaxSize=5);
};

/*
 * BRIEF Descriptor
 */

/** @brief Class for computing BRIEF descriptors described in @cite calon2010 .

@param bytes legth of the descriptor in bytes, valid values are: 16, 32 (default) or 64 .
@param use_orientation sample patterns using keypoints orientation, disabled by default.

 */
class CV_EXPORTS_W BriefDescriptorExtractor : public Feature2D
{
public:
    CV_WRAP static Ptr<BriefDescriptorExtractor> create( int bytes = 32, bool use_orientation = false );
};

/** @brief Class implementing the locally uniform comparison image descriptor, described in @cite LUCID

An image descriptor that can be computed very fast, while being
about as robust as, for example, SURF or BRIEF.

@note It requires a color image as input.
 */
class CV_EXPORTS_W LUCID : public Feature2D
{
public:
    /**
     * @param lucid_kernel kernel for descriptor construction, where 1=3x3, 2=5x5, 3=7x7 and so forth
     * @param blur_kernel kernel for blurring image prior to descriptor construction, where 1=3x3, 2=5x5, 3=7x7 and so forth
     */
    CV_WRAP static Ptr<LUCID> create(const int lucid_kernel = 1, const int blur_kernel = 2);
};


/*
* LATCH Descriptor
*/

/** latch Class for computing the LATCH descriptor.
If you find this code useful, please add a reference to the following paper in your work:
Gil Levi and Tal Hassner, "LATCH: Learned Arrangements of Three Patch Codes", arXiv preprint arXiv:1501.03719, 15 Jan. 2015

LATCH is a binary descriptor based on learned comparisons of triplets of image patches.

* bytes is the size of the descriptor - can be 64, 32, 16, 8, 4, 2 or 1
* rotationInvariance - whether or not the descriptor should compansate for orientation changes.
* half_ssd_size - the size of half of the mini-patches size. For example, if we would like to compare triplets of patches of size 7x7x
    then the half_ssd_size should be (7-1)/2 = 3.
* sigma - sigma value for GaussianBlur smoothing of the source image. Source image will be used without smoothing in case sigma value is 0.

Note: the descriptor can be coupled with any keypoint extractor. The only demand is that if you use set rotationInvariance = True then
    you will have to use an extractor which estimates the patch orientation (in degrees). Examples for such extractors are ORB and SIFT.

Note: a complete example can be found under /samples/cpp/tutorial_code/xfeatures2D/latch_match.cpp

*/
class CV_EXPORTS_W LATCH : public Feature2D
{
public:
    CV_WRAP static Ptr<LATCH> create(int bytes = 32, bool rotationInvariance = true, int half_ssd_size = 3, double sigma = 2.0);
};

/** @brief Class implementing DAISY descriptor, described in @cite Tola10

@param radius radius of the descriptor at the initial scale
@param q_radius amount of radial range division quantity
@param q_theta amount of angular range division quantity
@param q_hist amount of gradient orientations range division quantity
@param norm choose descriptors normalization type, where
DAISY::NRM_NONE will not do any normalization (default),
DAISY::NRM_PARTIAL mean that histograms are normalized independently for L2 norm equal to 1.0,
DAISY::NRM_FULL mean that descriptors are normalized for L2 norm equal to 1.0,
DAISY::NRM_SIFT mean that descriptors are normalized for L2 norm equal to 1.0 but no individual one is bigger than 0.154 as in SIFT
@param H optional 3x3 homography matrix used to warp the grid of daisy but sampling keypoints remains unwarped on image
@param interpolation switch to disable interpolation for speed improvement at minor quality loss
@param use_orientation sample patterns using keypoints orientation, disabled by default.

 */
class CV_EXPORTS_W DAISY : public Feature2D
{
public:
    enum
    {
        NRM_NONE = 100, NRM_PARTIAL = 101, NRM_FULL = 102, NRM_SIFT = 103,
    };
    CV_WRAP static Ptr<DAISY> create( float radius = 15, int q_radius = 3, int q_theta = 8,
                int q_hist = 8, int norm = DAISY::NRM_NONE, InputArray H = noArray(),
                bool interpolation = true, bool use_orientation = false );

    /** @overload
     * @param image image to extract descriptors
     * @param keypoints of interest within image
     * @param descriptors resulted descriptors array
     */
    virtual void compute( InputArray image, std::vector<KeyPoint>& keypoints, OutputArray descriptors ) CV_OVERRIDE = 0;

    virtual void compute( InputArrayOfArrays images,
                          std::vector<std::vector<KeyPoint> >& keypoints,
                          OutputArrayOfArrays descriptors ) CV_OVERRIDE;

    /** @overload
     * @param image image to extract descriptors
     * @param roi region of interest within image
     * @param descriptors resulted descriptors array for roi image pixels
     */
    virtual void compute( InputArray image, Rect roi, OutputArray descriptors ) = 0;

    /**@overload
     * @param image image to extract descriptors
     * @param descriptors resulted descriptors array for all image pixels
     */
    virtual void compute( InputArray image, OutputArray descriptors ) = 0;

    /**
     * @param y position y on image
     * @param x position x on image
     * @param orientation orientation on image (0->360)
     * @param descriptor supplied array for descriptor storage
     */
    virtual void GetDescriptor( double y, double x, int orientation, float* descriptor ) const = 0;

    /**
     * @param y position y on image
     * @param x position x on image
     * @param orientation orientation on image (0->360)
     * @param descriptor supplied array for descriptor storage
     * @param H homography matrix for warped grid
     */
    virtual bool GetDescriptor( double y, double x, int orientation, float* descriptor, double* H ) const = 0;

    /**
     * @param y position y on image
     * @param x position x on image
     * @param orientation orientation on image (0->360)
     * @param descriptor supplied array for descriptor storage
     */
    virtual void GetUnnormalizedDescriptor( double y, double x, int orientation, float* descriptor ) const = 0;

    /**
     * @param y position y on image
     * @param x position x on image
     * @param orientation orientation on image (0->360)
     * @param descriptor supplied array for descriptor storage
     * @param H homography matrix for warped grid
     */
    virtual bool GetUnnormalizedDescriptor( double y, double x, int orientation, float* descriptor , double *H ) const = 0;

};

/** @brief Class implementing the MSD (*Maximal Self-Dissimilarity*) keypoint detector, described in @cite Tombari14.

The algorithm implements a novel interest point detector stemming from the intuition that image patches
which are highly dissimilar over a relatively large extent of their surroundings hold the property of
being repeatable and distinctive. This concept of "contextual self-dissimilarity" reverses the key
paradigm of recent successful techniques such as the Local Self-Similarity descriptor and the Non-Local
Means filter, which build upon the presence of similar - rather than dissimilar - patches. Moreover,
it extends to contextual information the local self-dissimilarity notion embedded in established
detectors of corner-like interest points, thereby achieving enhanced repeatability, distinctiveness and
localization accuracy.

*/

class CV_EXPORTS_W MSDDetector : public Feature2D {

public:

    static Ptr<MSDDetector> create(int m_patch_radius = 3, int m_search_area_radius = 5,
            int m_nms_radius = 5, int m_nms_scale_radius = 0, float m_th_saliency = 250.0f, int m_kNN = 4,
            float m_scale_factor = 1.25f, int m_n_scales = -1, bool m_compute_orientation = false);
};

/** @brief Class implementing VGG (Oxford Visual Geometry Group) descriptor trained end to end
using "Descriptor Learning Using Convex Optimisation" (DLCO) aparatus described in @cite Simonyan14.

@param desc type of descriptor to use, VGG::VGG_120 is default (120 dimensions float)
Available types are VGG::VGG_120, VGG::VGG_80, VGG::VGG_64, VGG::VGG_48
@param isigma gaussian kernel value for image blur (default is 1.4f)
@param img_normalize use image sample intensity normalization (enabled by default)
@param use_orientation sample patterns using keypoints orientation, enabled by default
@param scale_factor adjust the sampling window of detected keypoints to 64.0f (VGG sampling window)
6.25f is default and fits for KAZE, SURF detected keypoints window ratio
6.75f should be the scale for SIFT detected keypoints window ratio
5.00f should be the scale for AKAZE, MSD, AGAST, FAST, BRISK keypoints window ratio
0.75f should be the scale for ORB keypoints ratio

@param dsc_normalize clamp descriptors to 255 and convert to uchar CV_8UC1 (disabled by default)

 */
class CV_EXPORTS_W VGG : public Feature2D
{
public:

    CV_WRAP enum
    {
        VGG_120 = 100, VGG_80 = 101, VGG_64 = 102, VGG_48 = 103,
    };

    CV_WRAP static Ptr<VGG> create( int desc = VGG::VGG_120, float isigma = 1.4f,
                                    bool img_normalize = true, bool use_scale_orientation = true,
                                    float scale_factor = 6.25f, bool dsc_normalize = false );

    CV_WRAP virtual void setSigma(const float isigma) = 0;
    CV_WRAP virtual float getSigma() const = 0;

    CV_WRAP virtual void setUseNormalizeImage(const bool img_normalize) = 0;
    CV_WRAP virtual bool getUseNormalizeImage() const = 0;

    CV_WRAP virtual void setUseScaleOrientation(const bool use_scale_orientation) = 0;
    CV_WRAP virtual bool getUseScaleOrientation() const = 0;

    CV_WRAP virtual void setScaleFactor(const float scale_factor) = 0;
    CV_WRAP virtual float getScaleFactor() const = 0;

    CV_WRAP virtual void setUseNormalizeDescriptor(const bool dsc_normalize) = 0;
    CV_WRAP virtual bool getUseNormalizeDescriptor() const = 0;
};

/** @brief Class implementing BoostDesc (Learning Image Descriptors with Boosting), described in
@cite Trzcinski13a and @cite Trzcinski13b.

@param desc type of descriptor to use, BoostDesc::BINBOOST_256 is default (256 bit long dimension)
Available types are: BoostDesc::BGM, BoostDesc::BGM_HARD, BoostDesc::BGM_BILINEAR, BoostDesc::LBGM,
BoostDesc::BINBOOST_64, BoostDesc::BINBOOST_128, BoostDesc::BINBOOST_256
@param use_orientation sample patterns using keypoints orientation, enabled by default
@param scale_factor adjust the sampling window of detected keypoints
6.25f is default and fits for KAZE, SURF detected keypoints window ratio
6.75f should be the scale for SIFT detected keypoints window ratio
5.00f should be the scale for AKAZE, MSD, AGAST, FAST, BRISK keypoints window ratio
0.75f should be the scale for ORB keypoints ratio
1.50f was the default in original implementation

@note BGM is the base descriptor where each binary dimension is computed as the output of a single weak learner.
BGM_HARD and BGM_BILINEAR refers to same BGM but use different type of gradient binning. In the BGM_HARD that
use ASSIGN_HARD binning type the gradient is assigned to the nearest orientation bin. In the BGM_BILINEAR that use
ASSIGN_BILINEAR binning type the gradient is assigned to the two neighbouring bins. In the BGM and all other modes that use
ASSIGN_SOFT binning type the gradient is assigned to 8 nearest bins according to the cosine value between the gradient
angle and the bin center. LBGM (alias FP-Boost) is the floating point extension where each dimension is computed
as a linear combination of the weak learner responses. BINBOOST and subvariants are the binary extensions of LBGM
where each bit is computed as a thresholded linear combination of a set of weak learners.
BoostDesc header files (boostdesc_*.i) was exported from original binaries with export-boostdesc.py script from
samples subfolder.

*/

class CV_EXPORTS_W BoostDesc : public Feature2D
{
public:

    CV_WRAP enum
    {
       BGM = 100, BGM_HARD = 101, BGM_BILINEAR = 102, LBGM = 200,
       BINBOOST_64 = 300, BINBOOST_128 = 301, BINBOOST_256 = 302
    };

    CV_WRAP static Ptr<BoostDesc> create( int desc = BoostDesc::BINBOOST_256,
                    bool use_scale_orientation = true, float scale_factor = 6.25f );

    CV_WRAP virtual void setUseScaleOrientation(const bool use_scale_orientation) = 0;
    CV_WRAP virtual bool getUseScaleOrientation() const = 0;

    CV_WRAP virtual void setScaleFactor(const float scale_factor) = 0;
    CV_WRAP virtual float getScaleFactor() const = 0;
};


/*
* Position-Color-Texture signatures
*/

/**
* @brief Class implementing PCT (position-color-texture) signature extraction
*       as described in @cite KrulisLS16.
*       The algorithm is divided to a feature sampler and a clusterizer.
*       Feature sampler produces samples at given set of coordinates.
*       Clusterizer then produces clusters of these samples using k-means algorithm.
*       Resulting set of clusters is the signature of the input image.
*
*       A signature is an array of SIGNATURE_DIMENSION-dimensional points.
*       Used dimensions are:
*       weight, x, y position; lab color, contrast, entropy.
* @cite KrulisLS16
* @cite BeecksUS10
*/
class CV_EXPORTS_W PCTSignatures : public Algorithm
{
public:
    /**
    * @brief Lp distance function selector.
    */
    enum DistanceFunction
    {
        L0_25, L0_5, L1, L2, L2SQUARED, L5, L_INFINITY
    };

    /**
    * @brief Point distributions supported by random point generator.
    */
    enum PointDistribution
    {
        UNIFORM,    //!< Generate numbers uniformly.
        REGULAR,    //!< Generate points in a regular grid.
        NORMAL      //!< Generate points with normal (gaussian) distribution.
    };

    /**
    * @brief Similarity function selector.
    * @see
    *       Christian Beecks, Merih Seran Uysal, Thomas Seidl.
    *       Signature quadratic form distance.
    *       In Proceedings of the ACM International Conference on Image and Video Retrieval, pages 438-445.
    *       ACM, 2010.
    * @cite BeecksUS10
    * @note For selected distance function: \f[ d(c_i, c_j) \f]  and parameter: \f[ \alpha \f]
    */
    enum SimilarityFunction
    {
        MINUS,      //!< \f[ -d(c_i, c_j) \f]
        GAUSSIAN,   //!< \f[ e^{ -\alpha * d^2(c_i, c_j)} \f]
        HEURISTIC   //!< \f[ \frac{1}{\alpha + d(c_i, c_j)} \f]
    };


    /**
    * @brief Creates PCTSignatures algorithm using sample and seed count.
    *       It generates its own sets of sampling points and clusterization seed indexes.
    * @param initSampleCount Number of points used for image sampling.
    * @param initSeedCount Number of initial clusterization seeds.
    *       Must be lower or equal to initSampleCount
    * @param pointDistribution Distribution of generated points. Default: UNIFORM.
    *       Available: UNIFORM, REGULAR, NORMAL.
    * @return Created algorithm.
    */
    CV_WRAP static Ptr<PCTSignatures> create(
        const int initSampleCount = 2000,
        const int initSeedCount = 400,
        const int pointDistribution = 0);

    /**
    * @brief Creates PCTSignatures algorithm using pre-generated sampling points
    *       and number of clusterization seeds. It uses the provided
    *       sampling points and generates its own clusterization seed indexes.
    * @param initSamplingPoints Sampling points used in image sampling.
    * @param initSeedCount Number of initial clusterization seeds.
    *       Must be lower or equal to initSamplingPoints.size().
    * @return Created algorithm.
    */
    CV_WRAP static Ptr<PCTSignatures> create(
        const std::vector<Point2f>& initSamplingPoints,
        const int initSeedCount);

    /**
    * @brief Creates PCTSignatures algorithm using pre-generated sampling points
    *       and clusterization seeds indexes.
    * @param initSamplingPoints Sampling points used in image sampling.
    * @param initClusterSeedIndexes Indexes of initial clusterization seeds.
    *       Its size must be lower or equal to initSamplingPoints.size().
    * @return Created algorithm.
    */
    CV_WRAP static Ptr<PCTSignatures> create(
        const std::vector<Point2f>& initSamplingPoints,
        const std::vector<int>& initClusterSeedIndexes);



    /**
    * @brief Computes signature of given image.
    * @param image Input image of CV_8U type.
    * @param signature Output computed signature.
    */
    CV_WRAP virtual void computeSignature(
        InputArray image,
        OutputArray signature) const = 0;

    /**
    * @brief Computes signatures for multiple images in parallel.
    * @param images Vector of input images of CV_8U type.
    * @param signatures Vector of computed signatures.
    */
    CV_WRAP virtual void computeSignatures(
        const std::vector<Mat>& images,
        std::vector<Mat>& signatures) const = 0;

    /**
    * @brief Draws signature in the source image and outputs the result.
    *       Signatures are visualized as a circle
    *       with radius based on signature weight
    *       and color based on signature color.
    *       Contrast and entropy are not visualized.
    * @param source Source image.
    * @param signature Image signature.
    * @param result Output result.
    * @param radiusToShorterSideRatio Determines maximal radius of signature in the output image.
    * @param borderThickness Border thickness of the visualized signature.
    */
    CV_WRAP static void drawSignature(
        InputArray source,
        InputArray signature,
        OutputArray result,
        float radiusToShorterSideRatio = 1.0 / 8,
        int borderThickness = 1);

    /**
    * @brief Generates initial sampling points according to selected point distribution.
    * @param initPoints Output vector where the generated points will be saved.
    * @param count Number of points to generate.
    * @param pointDistribution Point distribution selector.
    *       Available: UNIFORM, REGULAR, NORMAL.
    * @note Generated coordinates are in range [0..1)
    */
    CV_WRAP static void generateInitPoints(
        std::vector<Point2f>& initPoints,
        const int count,
        int pointDistribution);


    /**** sampler ****/

    /**
    * @brief Number of initial samples taken from the image.
    */
    CV_WRAP virtual int getSampleCount() const = 0;

    /**
    * @brief Color resolution of the greyscale bitmap represented in allocated bits
    *       (i.e., value 4 means that 16 shades of grey are used).
    *       The greyscale bitmap is used for computing contrast and entropy values.
    */
    CV_WRAP virtual int getGrayscaleBits() const = 0;
    /**
    * @brief Color resolution of the greyscale bitmap represented in allocated bits
    *       (i.e., value 4 means that 16 shades of grey are used).
    *       The greyscale bitmap is used for computing contrast and entropy values.
    */
    CV_WRAP virtual void setGrayscaleBits(int grayscaleBits) = 0;

    /**
    * @brief Size of the texture sampling window used to compute contrast and entropy
    *       (center of the window is always in the pixel selected by x,y coordinates
    *       of the corresponding feature sample).
    */
    CV_WRAP virtual int getWindowRadius() const = 0;
    /**
    * @brief Size of the texture sampling window used to compute contrast and entropy
    *       (center of the window is always in the pixel selected by x,y coordinates
    *       of the corresponding feature sample).
    */
    CV_WRAP virtual void setWindowRadius(int radius) = 0;


    /**
    * @brief Weights (multiplicative constants) that linearly stretch individual axes of the feature space
    *       (x,y = position; L,a,b = color in CIE Lab space; c = contrast. e = entropy)
    */
    CV_WRAP virtual float getWeightX() const = 0;
    /**
    * @brief Weights (multiplicative constants) that linearly stretch individual axes of the feature space
    *       (x,y = position; L,a,b = color in CIE Lab space; c = contrast. e = entropy)
    */
    CV_WRAP virtual void setWeightX(float weight) = 0;

    /**
    * @brief Weights (multiplicative constants) that linearly stretch individual axes of the feature space
    *       (x,y = position; L,a,b = color in CIE Lab space; c = contrast. e = entropy)
    */
    CV_WRAP virtual float getWeightY() const = 0;
    /**
    * @brief Weights (multiplicative constants) that linearly stretch individual axes of the feature space
    *       (x,y = position; L,a,b = color in CIE Lab space; c = contrast. e = entropy)
    */
    CV_WRAP virtual void setWeightY(float weight) = 0;

    /**
    * @brief Weights (multiplicative constants) that linearly stretch individual axes of the feature space
    *       (x,y = position; L,a,b = color in CIE Lab space; c = contrast. e = entropy)
    */
    CV_WRAP virtual float getWeightL() const = 0;
    /**
    * @brief Weights (multiplicative constants) that linearly stretch individual axes of the feature space
    *       (x,y = position; L,a,b = color in CIE Lab space; c = contrast. e = entropy)
    */
    CV_WRAP virtual void setWeightL(float weight) = 0;

    /**
    * @brief Weights (multiplicative constants) that linearly stretch individual axes of the feature space
    *       (x,y = position; L,a,b = color in CIE Lab space; c = contrast. e = entropy)
    */
    CV_WRAP virtual float getWeightA() const = 0;
    /**
    * @brief Weights (multiplicative constants) that linearly stretch individual axes of the feature space
    *       (x,y = position; L,a,b = color in CIE Lab space; c = contrast. e = entropy)
    */
    CV_WRAP virtual void setWeightA(float weight) = 0;

    /**
    * @brief Weights (multiplicative constants) that linearly stretch individual axes of the feature space
    *       (x,y = position; L,a,b = color in CIE Lab space; c = contrast. e = entropy)
    */
    CV_WRAP virtual float getWeightB() const = 0;
    /**
    * @brief Weights (multiplicative constants) that linearly stretch individual axes of the feature space
    *       (x,y = position; L,a,b = color in CIE Lab space; c = contrast. e = entropy)
    */
    CV_WRAP virtual void setWeightB(float weight) = 0;

    /**
    * @brief Weights (multiplicative constants) that linearly stretch individual axes of the feature space
    *       (x,y = position; L,a,b = color in CIE Lab space; c = contrast. e = entropy)
    */
    CV_WRAP virtual float getWeightContrast() const = 0;
    /**
    * @brief Weights (multiplicative constants) that linearly stretch individual axes of the feature space
    *       (x,y = position; L,a,b = color in CIE Lab space; c = contrast. e = entropy)
    */
    CV_WRAP virtual void setWeightContrast(float weight) = 0;

    /**
    * @brief Weights (multiplicative constants) that linearly stretch individual axes of the feature space
    *       (x,y = position; L,a,b = color in CIE Lab space; c = contrast. e = entropy)
    */
    CV_WRAP virtual float getWeightEntropy() const = 0;
    /**
    * @brief Weights (multiplicative constants) that linearly stretch individual axes of the feature space
    *       (x,y = position; L,a,b = color in CIE Lab space; c = contrast. e = entropy)
    */
    CV_WRAP virtual void setWeightEntropy(float weight) = 0;

    /**
    * @brief Initial samples taken from the image.
    *       These sampled features become the input for clustering.
    */
    CV_WRAP virtual std::vector<Point2f> getSamplingPoints() const = 0;



    /**
    * @brief Weights (multiplicative constants) that linearly stretch individual axes of the feature space.
    * @param idx ID of the weight
    * @param value Value of the weight
    * @note
    *       WEIGHT_IDX = 0;
    *       X_IDX = 1;
    *       Y_IDX = 2;
    *       L_IDX = 3;
    *       A_IDX = 4;
    *       B_IDX = 5;
    *       CONTRAST_IDX = 6;
    *       ENTROPY_IDX = 7;
    */
    CV_WRAP virtual void setWeight(int idx, float value) = 0;
    /**
    * @brief Weights (multiplicative constants) that linearly stretch individual axes of the feature space.
    * @param weights Values of all weights.
    * @note
    *       WEIGHT_IDX = 0;
    *       X_IDX = 1;
    *       Y_IDX = 2;
    *       L_IDX = 3;
    *       A_IDX = 4;
    *       B_IDX = 5;
    *       CONTRAST_IDX = 6;
    *       ENTROPY_IDX = 7;
    */
    CV_WRAP virtual void setWeights(const std::vector<float>& weights) = 0;

    /**
    * @brief Translations of the individual axes of the feature space.
    * @param idx ID of the translation
    * @param value Value of the translation
    * @note
    *       WEIGHT_IDX = 0;
    *       X_IDX = 1;
    *       Y_IDX = 2;
    *       L_IDX = 3;
    *       A_IDX = 4;
    *       B_IDX = 5;
    *       CONTRAST_IDX = 6;
    *       ENTROPY_IDX = 7;
    */
    CV_WRAP virtual void setTranslation(int idx, float value) = 0;
    /**
    * @brief Translations of the individual axes of the feature space.
    * @param translations Values of all translations.
    * @note
    *       WEIGHT_IDX = 0;
    *       X_IDX = 1;
    *       Y_IDX = 2;
    *       L_IDX = 3;
    *       A_IDX = 4;
    *       B_IDX = 5;
    *       CONTRAST_IDX = 6;
    *       ENTROPY_IDX = 7;
    */
    CV_WRAP virtual void setTranslations(const std::vector<float>& translations) = 0;

    /**
    * @brief Sets sampling points used to sample the input image.
    * @param samplingPoints Vector of sampling points in range [0..1)
    * @note Number of sampling points must be greater or equal to clusterization seed count.
    */
    CV_WRAP virtual void setSamplingPoints(std::vector<Point2f> samplingPoints) = 0;



    /**** clusterizer ****/
    /**
    * @brief Initial seeds (initial number of clusters) for the k-means algorithm.
    */
    CV_WRAP virtual std::vector<int> getInitSeedIndexes() const = 0;
    /**
    * @brief Initial seed indexes for the k-means algorithm.
    */
    CV_WRAP virtual void setInitSeedIndexes(std::vector<int> initSeedIndexes) = 0;
    /**
    * @brief Number of initial seeds (initial number of clusters) for the k-means algorithm.
    */
    CV_WRAP virtual int getInitSeedCount() const = 0;

    /**
    * @brief Number of iterations of the k-means clustering.
    *       We use fixed number of iterations, since the modified clustering is pruning clusters
    *       (not iteratively refining k clusters).
    */
    CV_WRAP virtual int getIterationCount() const = 0;
    /**
    * @brief Number of iterations of the k-means clustering.
    *       We use fixed number of iterations, since the modified clustering is pruning clusters
    *       (not iteratively refining k clusters).
    */
    CV_WRAP virtual void setIterationCount(int iterationCount) = 0;

    /**
    * @brief Maximal number of generated clusters. If the number is exceeded,
    *       the clusters are sorted by their weights and the smallest clusters are cropped.
    */
    CV_WRAP virtual int getMaxClustersCount() const = 0;
    /**
    * @brief Maximal number of generated clusters. If the number is exceeded,
    *       the clusters are sorted by their weights and the smallest clusters are cropped.
    */
    CV_WRAP virtual void setMaxClustersCount(int maxClustersCount) = 0;

    /**
    * @brief This parameter multiplied by the index of iteration gives lower limit for cluster size.
    *       Clusters containing fewer points than specified by the limit have their centroid dismissed
    *       and points are reassigned.
    */
    CV_WRAP virtual int getClusterMinSize() const = 0;
    /**
    * @brief This parameter multiplied by the index of iteration gives lower limit for cluster size.
    *       Clusters containing fewer points than specified by the limit have their centroid dismissed
    *       and points are reassigned.
    */
    CV_WRAP virtual void setClusterMinSize(int clusterMinSize) = 0;

    /**
    * @brief Threshold euclidean distance between two centroids.
    *       If two cluster centers are closer than this distance,
    *       one of the centroid is dismissed and points are reassigned.
    */
    CV_WRAP virtual float getJoiningDistance() const = 0;
    /**
    * @brief Threshold euclidean distance between two centroids.
    *       If two cluster centers are closer than this distance,
    *       one of the centroid is dismissed and points are reassigned.
    */
    CV_WRAP virtual void setJoiningDistance(float joiningDistance) = 0;

    /**
    * @brief Remove centroids in k-means whose weight is lesser or equal to given threshold.
    */
    CV_WRAP virtual float getDropThreshold() const = 0;
    /**
    * @brief Remove centroids in k-means whose weight is lesser or equal to given threshold.
    */
    CV_WRAP virtual void setDropThreshold(float dropThreshold) = 0;

    /**
    * @brief Distance function selector used for measuring distance between two points in k-means.
    */
    CV_WRAP virtual int getDistanceFunction() const = 0;
    /**
    * @brief Distance function selector used for measuring distance between two points in k-means.
    *       Available: L0_25, L0_5, L1, L2, L2SQUARED, L5, L_INFINITY.
    */
    CV_WRAP virtual void setDistanceFunction(int distanceFunction) = 0;

};

/**
* @brief Class implementing Signature Quadratic Form Distance (SQFD).
* @see Christian Beecks, Merih Seran Uysal, Thomas Seidl.
*   Signature quadratic form distance.
*   In Proceedings of the ACM International Conference on Image and Video Retrieval, pages 438-445.
*   ACM, 2010.
* @cite BeecksUS10
*/
class CV_EXPORTS_W PCTSignaturesSQFD : public Algorithm
{
public:

    /**
    * @brief Creates the algorithm instance using selected distance function,
    *       similarity function and similarity function parameter.
    * @param distanceFunction Distance function selector. Default: L2
    *       Available: L0_25, L0_5, L1, L2, L2SQUARED, L5, L_INFINITY
    * @param similarityFunction Similarity function selector. Default: HEURISTIC
    *       Available: MINUS, GAUSSIAN, HEURISTIC
    * @param similarityParameter Parameter of the similarity function.
    */
    CV_WRAP static Ptr<PCTSignaturesSQFD> create(
        const int distanceFunction = 3,
        const int similarityFunction = 2,
        const float similarityParameter = 1.0f);

    /**
    * @brief Computes Signature Quadratic Form Distance of two signatures.
    * @param _signature0 The first signature.
    * @param _signature1 The second signature.
    */
    CV_WRAP virtual float computeQuadraticFormDistance(
        InputArray _signature0,
        InputArray _signature1) const = 0;

    /**
    * @brief Computes Signature Quadratic Form Distance between the reference signature
    *       and each of the other image signatures.
    * @param sourceSignature The signature to measure distance of other signatures from.
    * @param imageSignatures Vector of signatures to measure distance from the source signature.
    * @param distances Output vector of measured distances.
    */
    CV_WRAP virtual void computeQuadraticFormDistances(
        const Mat& sourceSignature,
        const std::vector<Mat>& imageSignatures,
        std::vector<float>& distances) const = 0;

};

/**
* @brief Elliptic region around an interest point.
*/
class CV_EXPORTS Elliptic_KeyPoint : public KeyPoint
{
public:
    Size_<float> axes; //!< the lengths of the major and minor ellipse axes
    float si;  //!< the integration scale at which the parameters were estimated
    Matx23f transf; //!< the transformation between image space and local patch space
    Elliptic_KeyPoint();
    Elliptic_KeyPoint(Point2f pt, float angle, Size axes, float size, float si);
    virtual ~Elliptic_KeyPoint();
};

/**
 * @brief Class implementing the Harris-Laplace feature detector as described in @cite Mikolajczyk2004.
 */
class CV_EXPORTS_W HarrisLaplaceFeatureDetector : public Feature2D
{
public:
    /**
     * @brief Creates a new implementation instance.
     *
     * @param numOctaves the number of octaves in the scale-space pyramid
     * @param corn_thresh the threshold for the Harris cornerness measure
     * @param DOG_thresh the threshold for the Difference-of-Gaussians scale selection
     * @param maxCorners the maximum number of corners to consider
     * @param num_layers the number of intermediate scales per octave
     */
    CV_WRAP static Ptr<HarrisLaplaceFeatureDetector> create(
            int numOctaves=6,
            float corn_thresh=0.01f,
            float DOG_thresh=0.01f,
            int maxCorners=5000,
            int num_layers=4);
};

/**
 * @brief Class implementing affine adaptation for key points.
 *
 * A @ref FeatureDetector and a @ref DescriptorExtractor are wrapped to augment the
 * detected points with their affine invariant elliptic region and to compute
 * the feature descriptors on the regions after warping them into circles.
 *
 * The interface is equivalent to @ref Feature2D, adding operations for
 * @ref Elliptic_KeyPoint "Elliptic_KeyPoints" instead of @ref KeyPoint "KeyPoints".
 */
class CV_EXPORTS AffineFeature2D : public Feature2D
{
public:
    /**
     * @brief Creates an instance wrapping the given keypoint detector and
     * descriptor extractor.
     */
    static Ptr<AffineFeature2D> create(
        Ptr<FeatureDetector> keypoint_detector,
        Ptr<DescriptorExtractor> descriptor_extractor);

    /**
     * @brief Creates an instance where keypoint detector and descriptor
     * extractor are identical.
     */
    static Ptr<AffineFeature2D> create(
        Ptr<FeatureDetector> keypoint_detector)
    {
        return create(keypoint_detector, keypoint_detector);
    }

    using Feature2D::detect; // overload, don't hide
    /**
     * @brief Detects keypoints in the image using the wrapped detector and
     * performs affine adaptation to augment them with their elliptic regions.
     */
    virtual void detect(
        InputArray image,
        CV_OUT std::vector<Elliptic_KeyPoint>& keypoints,
        InputArray mask=noArray() ) = 0;

    using Feature2D::detectAndCompute; // overload, don't hide
    /**
     * @brief Detects keypoints and computes descriptors for their surrounding
     * regions, after warping them into circles.
     */
    virtual void detectAndCompute(
        InputArray image,
        InputArray mask,
        CV_OUT std::vector<Elliptic_KeyPoint>& keypoints,
        OutputArray descriptors,
        bool useProvidedKeypoints=false ) = 0;
};


/** @brief Estimates cornerness for prespecified KeyPoints using the FAST algorithm

@param image grayscale image where keypoints (corners) are detected.
@param keypoints keypoints which should be tested to fit the FAST criteria. Keypoints not being
detected as corners are removed.
@param threshold threshold on difference between intensity of the central pixel and pixels of a
circle around this pixel.
@param nonmaxSuppression if true, non-maximum suppression is applied to detected corners
(keypoints).
@param type one of the three neighborhoods as defined in the paper:
FastFeatureDetector::TYPE_9_16, FastFeatureDetector::TYPE_7_12,
FastFeatureDetector::TYPE_5_8

Detects corners using the FAST algorithm by @cite Rosten06 .
 */
CV_EXPORTS void FASTForPointSet( InputArray image, CV_IN_OUT std::vector<KeyPoint>& keypoints,
                      int threshold, bool nonmaxSuppression=true, int type=FastFeatureDetector::TYPE_9_16);


//! @}


//! @addtogroup xfeatures2d_match
//! @{

/** @brief GMS  (Grid-based Motion Statistics) feature matching strategy by @cite Bian2017gms .
    @param size1 Input size of image1.
    @param size2 Input size of image2.
    @param keypoints1 Input keypoints of image1.
    @param keypoints2 Input keypoints of image2.
    @param matches1to2 Input 1-nearest neighbor matches.
    @param matchesGMS Matches returned by the GMS matching strategy.
    @param withRotation Take rotation transformation into account.
    @param withScale Take scale transformation into account.
    @param thresholdFactor The higher, the less matches.
    @note
        Since GMS works well when the number of features is large, we recommend to use the ORB feature and set FastThreshold to 0 to get as many as possible features quickly.
        If matching results are not satisfying, please add more features. (We use 10000 for images with 640 X 480).
        If your images have big rotation and scale changes, please set withRotation or withScale to true.
 */

CV_EXPORTS_W void matchGMS( const Size& size1, const Size& size2, const std::vector<KeyPoint>& keypoints1, const std::vector<KeyPoint>& keypoints2,
                          const std::vector<DMatch>& matches1to2, CV_OUT std::vector<DMatch>& matchesGMS, const bool withRotation = false,
                          const bool withScale = false, const double thresholdFactor = 6.0 );

//! @}

}
}

#endif
