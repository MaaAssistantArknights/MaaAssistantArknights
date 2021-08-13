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

#ifndef __OPENCV_BGSEGM_HPP__
#define __OPENCV_BGSEGM_HPP__

#include "opencv2/video.hpp"

#ifdef __cplusplus

/** @defgroup bgsegm Improved Background-Foreground Segmentation Methods
*/

namespace cv
{
namespace bgsegm
{

//! @addtogroup bgsegm
//! @{

/** @brief Gaussian Mixture-based Background/Foreground Segmentation Algorithm.

The class implements the algorithm described in @cite KB2001 .
 */
class CV_EXPORTS_W BackgroundSubtractorMOG : public BackgroundSubtractor
{
public:
    CV_WRAP virtual int getHistory() const = 0;
    CV_WRAP virtual void setHistory(int nframes) = 0;

    CV_WRAP virtual int getNMixtures() const = 0;
    CV_WRAP virtual void setNMixtures(int nmix) = 0;

    CV_WRAP virtual double getBackgroundRatio() const = 0;
    CV_WRAP virtual void setBackgroundRatio(double backgroundRatio) = 0;

    CV_WRAP virtual double getNoiseSigma() const = 0;
    CV_WRAP virtual void setNoiseSigma(double noiseSigma) = 0;
};

/** @brief Creates mixture-of-gaussian background subtractor

@param history Length of the history.
@param nmixtures Number of Gaussian mixtures.
@param backgroundRatio Background ratio.
@param noiseSigma Noise strength (standard deviation of the brightness or each color channel). 0
means some automatic value.
 */
CV_EXPORTS_W Ptr<BackgroundSubtractorMOG>
    createBackgroundSubtractorMOG(int history=200, int nmixtures=5,
                                  double backgroundRatio=0.7, double noiseSigma=0);


/** @brief Background Subtractor module based on the algorithm given in @cite Gold2012 .

 Takes a series of images and returns a sequence of mask (8UC1)
 images of the same size, where 255 indicates Foreground and 0 represents Background.
 This class implements an algorithm described in "Visual Tracking of Human Visitors under
 Variable-Lighting Conditions for a Responsive Audio Art Installation," A. Godbehere,
 A. Matsukawa, K. Goldberg, American Control Conference, Montreal, June 2012.
 */
class CV_EXPORTS_W BackgroundSubtractorGMG : public BackgroundSubtractor
{
public:
    /** @brief Returns total number of distinct colors to maintain in histogram.
    */
    CV_WRAP virtual int getMaxFeatures() const = 0;
    /** @brief Sets total number of distinct colors to maintain in histogram.
    */
    CV_WRAP virtual void setMaxFeatures(int maxFeatures) = 0;

    /** @brief Returns the learning rate of the algorithm.

    It lies between 0.0 and 1.0. It determines how quickly features are "forgotten" from
    histograms.
     */
    CV_WRAP virtual double getDefaultLearningRate() const = 0;
    /** @brief Sets the learning rate of the algorithm.
    */
    CV_WRAP virtual void setDefaultLearningRate(double lr) = 0;

    /** @brief Returns the number of frames used to initialize background model.
    */
    CV_WRAP virtual int getNumFrames() const = 0;
    /** @brief Sets the number of frames used to initialize background model.
    */
    CV_WRAP virtual void setNumFrames(int nframes) = 0;

    /** @brief Returns the parameter used for quantization of color-space.

    It is the number of discrete levels in each channel to be used in histograms.
     */
    CV_WRAP virtual int getQuantizationLevels() const = 0;
    /** @brief Sets the parameter used for quantization of color-space
    */
    CV_WRAP virtual void setQuantizationLevels(int nlevels) = 0;

    /** @brief Returns the prior probability that each individual pixel is a background pixel.
    */
    CV_WRAP virtual double getBackgroundPrior() const = 0;
    /** @brief Sets the prior probability that each individual pixel is a background pixel.
    */
    CV_WRAP virtual void setBackgroundPrior(double bgprior) = 0;

    /** @brief Returns the kernel radius used for morphological operations
    */
    CV_WRAP virtual int getSmoothingRadius() const = 0;
    /** @brief Sets the kernel radius used for morphological operations
    */
    CV_WRAP virtual void setSmoothingRadius(int radius) = 0;

    /** @brief Returns the value of decision threshold.

    Decision value is the value above which pixel is determined to be FG.
     */
    CV_WRAP virtual double getDecisionThreshold() const = 0;
    /** @brief Sets the value of decision threshold.
    */
    CV_WRAP virtual void setDecisionThreshold(double thresh) = 0;

    /** @brief Returns the status of background model update
    */
    CV_WRAP virtual bool getUpdateBackgroundModel() const = 0;
    /** @brief Sets the status of background model update
    */
    CV_WRAP virtual void setUpdateBackgroundModel(bool update) = 0;

    /** @brief Returns the minimum value taken on by pixels in image sequence. Usually 0.
    */
    CV_WRAP virtual double getMinVal() const = 0;
    /** @brief Sets the minimum value taken on by pixels in image sequence.
    */
    CV_WRAP virtual void setMinVal(double val) = 0;

    /** @brief Returns the maximum value taken on by pixels in image sequence. e.g. 1.0 or 255.
    */
    CV_WRAP virtual double getMaxVal() const = 0;
    /** @brief Sets the maximum value taken on by pixels in image sequence.
    */
    CV_WRAP virtual void setMaxVal(double val) = 0;
};

/** @brief Creates a GMG Background Subtractor

@param initializationFrames number of frames used to initialize the background models.
@param decisionThreshold Threshold value, above which it is marked foreground, else background.
 */
CV_EXPORTS_W Ptr<BackgroundSubtractorGMG> createBackgroundSubtractorGMG(int initializationFrames=120,
                                                                        double decisionThreshold=0.8);

/** @brief Background subtraction based on counting.

  About as fast as MOG2 on a high end system.
  More than twice faster than MOG2 on cheap hardware (benchmarked on Raspberry Pi3).

  %Algorithm by Sagi Zeevi ( https://github.com/sagi-z/BackgroundSubtractorCNT )
*/
class CV_EXPORTS_W BackgroundSubtractorCNT  : public BackgroundSubtractor
{
public:
    // BackgroundSubtractor interface
    CV_WRAP virtual void apply(InputArray image, OutputArray fgmask, double learningRate=-1) CV_OVERRIDE = 0;
    CV_WRAP virtual void getBackgroundImage(OutputArray backgroundImage) const CV_OVERRIDE = 0;

    /** @brief Returns number of frames with same pixel color to consider stable.
    */
    CV_WRAP virtual int getMinPixelStability() const = 0;
    /** @brief Sets the number of frames with same pixel color to consider stable.
    */
    CV_WRAP virtual void setMinPixelStability(int value) = 0;

    /** @brief Returns maximum allowed credit for a pixel in history.
    */
    CV_WRAP virtual int getMaxPixelStability() const = 0;
    /** @brief Sets the maximum allowed credit for a pixel in history.
    */
    CV_WRAP virtual void setMaxPixelStability(int value) = 0;

    /** @brief Returns if we're giving a pixel credit for being stable for a long time.
    */
    CV_WRAP virtual bool getUseHistory() const = 0;
    /** @brief Sets if we're giving a pixel credit for being stable for a long time.
    */
    CV_WRAP virtual void setUseHistory(bool value) = 0;

    /** @brief Returns if we're parallelizing the algorithm.
    */
    CV_WRAP virtual bool getIsParallel() const = 0;
    /** @brief Sets if we're parallelizing the algorithm.
    */
    CV_WRAP virtual void setIsParallel(bool value) = 0;
};

/** @brief Creates a CNT Background Subtractor

@param minPixelStability number of frames with same pixel color to consider stable
@param useHistory determines if we're giving a pixel credit for being stable for a long time
@param maxPixelStability maximum allowed credit for a pixel in history
@param isParallel determines if we're parallelizing the algorithm
 */

CV_EXPORTS_W Ptr<BackgroundSubtractorCNT>
createBackgroundSubtractorCNT(int minPixelStability = 15,
                              bool useHistory = true,
                              int maxPixelStability = 15*60,
                              bool isParallel = true);

enum LSBPCameraMotionCompensation {
    LSBP_CAMERA_MOTION_COMPENSATION_NONE = 0,
    LSBP_CAMERA_MOTION_COMPENSATION_LK
};

/** @brief Implementation of the different yet better algorithm which is called GSOC, as it was implemented during GSOC and was not originated from any paper.

This algorithm demonstrates better performance on CDNET 2014 dataset compared to other algorithms in OpenCV.
 */
class CV_EXPORTS_W BackgroundSubtractorGSOC : public BackgroundSubtractor
{
public:
    // BackgroundSubtractor interface
    CV_WRAP virtual void apply(InputArray image, OutputArray fgmask, double learningRate=-1) CV_OVERRIDE = 0;

    CV_WRAP virtual void getBackgroundImage(OutputArray backgroundImage) const CV_OVERRIDE = 0;
};

/** @brief Background Subtraction using Local SVD Binary Pattern. More details about the algorithm can be found at @cite LGuo2016
 */
class CV_EXPORTS_W BackgroundSubtractorLSBP : public BackgroundSubtractor
{
public:
    // BackgroundSubtractor interface
    CV_WRAP virtual void apply(InputArray image, OutputArray fgmask, double learningRate=-1) CV_OVERRIDE = 0;

    CV_WRAP virtual void getBackgroundImage(OutputArray backgroundImage) const CV_OVERRIDE = 0;
};

/** @brief This is for calculation of the LSBP descriptors.
 */
class CV_EXPORTS_W BackgroundSubtractorLSBPDesc
{
public:
    static void calcLocalSVDValues(OutputArray localSVDValues, const Mat& frame);

    static void computeFromLocalSVDValues(OutputArray desc, const Mat& localSVDValues, const Point2i* LSBPSamplePoints);

    static void compute(OutputArray desc, const Mat& frame, const Point2i* LSBPSamplePoints);
};

/** @brief Creates an instance of BackgroundSubtractorGSOC algorithm.

Implementation of the different yet better algorithm which is called GSOC, as it was implemented during GSOC and was not originated from any paper.

@param mc Whether to use camera motion compensation.
@param nSamples Number of samples to maintain at each point of the frame.
@param replaceRate Probability of replacing the old sample - how fast the model will update itself.
@param propagationRate Probability of propagating to neighbors.
@param hitsThreshold How many positives the sample must get before it will be considered as a possible replacement.
@param alpha Scale coefficient for threshold.
@param beta Bias coefficient for threshold.
@param blinkingSupressionDecay Blinking supression decay factor.
@param blinkingSupressionMultiplier Blinking supression multiplier.
@param noiseRemovalThresholdFacBG Strength of the noise removal for background points.
@param noiseRemovalThresholdFacFG Strength of the noise removal for foreground points.
 */
CV_EXPORTS_W Ptr<BackgroundSubtractorGSOC> createBackgroundSubtractorGSOC(int mc = LSBP_CAMERA_MOTION_COMPENSATION_NONE, int nSamples = 20, float replaceRate = 0.003f, float propagationRate = 0.01f, int hitsThreshold = 32, float alpha = 0.01f, float beta = 0.0022f, float blinkingSupressionDecay = 0.1f, float blinkingSupressionMultiplier = 0.1f, float noiseRemovalThresholdFacBG = 0.0004f, float noiseRemovalThresholdFacFG = 0.0008f);

/** @brief Creates an instance of BackgroundSubtractorLSBP algorithm.

Background Subtraction using Local SVD Binary Pattern. More details about the algorithm can be found at @cite LGuo2016

@param mc Whether to use camera motion compensation.
@param nSamples Number of samples to maintain at each point of the frame.
@param LSBPRadius LSBP descriptor radius.
@param Tlower Lower bound for T-values. See @cite LGuo2016 for details.
@param Tupper Upper bound for T-values. See @cite LGuo2016 for details.
@param Tinc Increase step for T-values. See @cite LGuo2016 for details.
@param Tdec Decrease step for T-values. See @cite LGuo2016 for details.
@param Rscale Scale coefficient for threshold values.
@param Rincdec Increase/Decrease step for threshold values.
@param noiseRemovalThresholdFacBG Strength of the noise removal for background points.
@param noiseRemovalThresholdFacFG Strength of the noise removal for foreground points.
@param LSBPthreshold Threshold for LSBP binary string.
@param minCount Minimal number of matches for sample to be considered as foreground.
 */
CV_EXPORTS_W Ptr<BackgroundSubtractorLSBP> createBackgroundSubtractorLSBP(int mc = LSBP_CAMERA_MOTION_COMPENSATION_NONE, int nSamples = 20, int LSBPRadius = 16, float Tlower = 2.0f, float Tupper = 32.0f, float Tinc = 1.0f, float Tdec = 0.05f, float Rscale = 10.0f, float Rincdec = 0.005f, float noiseRemovalThresholdFacBG = 0.0004f, float noiseRemovalThresholdFacFG = 0.0008f, int LSBPthreshold = 8, int minCount = 2);

/** @brief Synthetic frame sequence generator for testing background subtraction algorithms.

 It will generate the moving object on top of the background.
 It will apply some distortion to the background to make the test more complex.
 */
class CV_EXPORTS_W SyntheticSequenceGenerator : public Algorithm
{
private:
    const double amplitude;
    const double wavelength;
    const double wavespeed;
    const double objspeed;
    unsigned timeStep;
    Point2d pos;
    Point2d dir;
    Mat background;
    Mat object;
    RNG rng;

public:
    /** @brief Creates an instance of SyntheticSequenceGenerator.

    @param background Background image for object.
    @param object Object image which will move slowly over the background.
    @param amplitude Amplitude of wave distortion applied to background.
    @param wavelength Length of waves in distortion applied to background.
    @param wavespeed How fast waves will move.
    @param objspeed How fast object will fly over background.
     */
    CV_WRAP SyntheticSequenceGenerator(InputArray background, InputArray object, double amplitude, double wavelength, double wavespeed, double objspeed);

    /** @brief Obtain the next frame in the sequence.

    @param frame Output frame.
    @param gtMask Output ground-truth (reference) segmentation mask object/background.
     */
    CV_WRAP void getNextFrame(OutputArray frame, OutputArray gtMask);
};

/** @brief Creates an instance of SyntheticSequenceGenerator.

@param background Background image for object.
@param object Object image which will move slowly over the background.
@param amplitude Amplitude of wave distortion applied to background.
@param wavelength Length of waves in distortion applied to background.
@param wavespeed How fast waves will move.
@param objspeed How fast object will fly over background.
 */
CV_EXPORTS_W Ptr<SyntheticSequenceGenerator> createSyntheticSequenceGenerator(InputArray background, InputArray object, double amplitude = 2.0, double wavelength = 20.0, double wavespeed = 0.2, double objspeed = 6.0);

//! @}

}
}

#endif
#endif
