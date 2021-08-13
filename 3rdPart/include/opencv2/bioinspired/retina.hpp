/*#******************************************************************************
 ** IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
 **
 ** By downloading, copying, installing or using the software you agree to this license.
 ** If you do not agree to this license, do not download, install,
 ** copy or use the software.
 **
 **
 ** bioinspired : interfaces allowing OpenCV users to integrate Human Vision System models. Presented models originate from Jeanny Herault's original research and have been reused and adapted by the author&collaborators for computed vision applications since his thesis with Alice Caplier at Gipsa-Lab.
 ** Use: extract still images & image sequences features, from contours details to motion spatio-temporal features, etc. for high level visual scene analysis. Also contribute to image enhancement/compression such as tone mapping.
 **
 ** Maintainers : Listic lab (code author current affiliation & applications) and Gipsa Lab (original research origins & applications)
 **
 **  Creation - enhancement process 2007-2015
 **      Author: Alexandre Benoit (benoit.alexandre.vision@gmail.com), LISTIC lab, Annecy le vieux, France
 **
 ** Theses algorithm have been developped by Alexandre BENOIT since his thesis with Alice Caplier at Gipsa-Lab (www.gipsa-lab.inpg.fr) and the research he pursues at LISTIC Lab (www.listic.univ-savoie.fr).
 ** Refer to the following research paper for more information:
 ** Benoit A., Caplier A., Durette B., Herault, J., "USING HUMAN VISUAL SYSTEM MODELING FOR BIO-INSPIRED LOW LEVEL IMAGE PROCESSING", Elsevier, Computer Vision and Image Understanding 114 (2010), pp. 758-773, DOI: http://dx.doi.org/10.1016/j.cviu.2010.01.011
 ** This work have been carried out thanks to Jeanny Herault who's research and great discussions are the basis of all this work, please take a look at his book:
 ** Vision: Images, Signals and Neural Networks: Models of Neural Processing in Visual Perception (Progress in Neural Processing),By: Jeanny Herault, ISBN: 9814273686. WAPI (Tower ID): 113266891.
 **
 ** The retina filter includes the research contributions of phd/research collegues from which code has been redrawn by the author :
 ** _take a look at the retinacolor.hpp module to discover Brice Chaix de Lavarene color mosaicing/demosaicing and the reference paper:
 ** ====> B. Chaix de Lavarene, D. Alleysson, B. Durette, J. Herault (2007). "Efficient demosaicing through recursive filtering", IEEE International Conference on Image Processing ICIP 2007
 ** _take a look at imagelogpolprojection.hpp to discover retina spatial log sampling which originates from Barthelemy Durette phd with Jeanny Herault. A Retina / V1 cortex projection is also proposed and originates from Jeanny's discussions.
 ** ====> more informations in the above cited Jeanny Heraults's book.
 **
 **                          License Agreement
 **               For Open Source Computer Vision Library
 **
 ** Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
 ** Copyright (C) 2008-2011, Willow Garage Inc., all rights reserved.
 **
 **               For Human Visual System tools (bioinspired)
 ** Copyright (C) 2007-2015, LISTIC Lab, Annecy le Vieux and GIPSA Lab, Grenoble, France, all rights reserved.
 **
 ** Third party copyrights are property of their respective owners.
 **
 ** Redistribution and use in source and binary forms, with or without modification,
 ** are permitted provided that the following conditions are met:
 **
 ** * Redistributions of source code must retain the above copyright notice,
 **    this list of conditions and the following disclaimer.
 **
 ** * Redistributions in binary form must reproduce the above copyright notice,
 **    this list of conditions and the following disclaimer in the documentation
 **    and/or other materials provided with the distribution.
 **
 ** * The name of the copyright holders may not be used to endorse or promote products
 **    derived from this software without specific prior written permission.
 **
 ** This software is provided by the copyright holders and contributors "as is" and
 ** any express or implied warranties, including, but not limited to, the implied
 ** warranties of merchantability and fitness for a particular purpose are disclaimed.
 ** In no event shall the Intel Corporation or contributors be liable for any direct,
 ** indirect, incidental, special, exemplary, or consequential damages
 ** (including, but not limited to, procurement of substitute goods or services;
 ** loss of use, data, or profits; or business interruption) however caused
 ** and on any theory of liability, whether in contract, strict liability,
 ** or tort (including negligence or otherwise) arising in any way out of
 ** the use of this software, even if advised of the possibility of such damage.
 *******************************************************************************/

#ifndef __OPENCV_BIOINSPIRED_RETINA_HPP__
#define __OPENCV_BIOINSPIRED_RETINA_HPP__

/**
@file
@date Jul 19, 2011
@author Alexandre Benoit
*/

#include "opencv2/core.hpp" // for all OpenCV core functionalities access, including cv::Exception support


namespace cv{
namespace bioinspired{

//! @addtogroup bioinspired
//! @{

enum {
    RETINA_COLOR_RANDOM, //!< each pixel position is either R, G or B in a random choice
    RETINA_COLOR_DIAGONAL,//!< color sampling is RGBRGBRGB..., line 2 BRGBRGBRG..., line 3, GBRGBRGBR...
    RETINA_COLOR_BAYER//!< standard bayer sampling
};


/** @brief retina model parameters structure

    For better clarity, check explenations on the comments of methods : setupOPLandIPLParvoChannel and setupIPLMagnoChannel

    Here is the default configuration file of the retina module. It gives results such as the first
    retina output shown on the top of this page.

    @code{xml}
    <?xml version="1.0"?>
    <opencv_storage>
    <OPLandIPLparvo>
        <colorMode>1</colorMode>
        <normaliseOutput>1</normaliseOutput>
        <photoreceptorsLocalAdaptationSensitivity>7.5e-01</photoreceptorsLocalAdaptationSensitivity>
        <photoreceptorsTemporalConstant>9.0e-01</photoreceptorsTemporalConstant>
        <photoreceptorsSpatialConstant>5.3e-01</photoreceptorsSpatialConstant>
        <horizontalCellsGain>0.01</horizontalCellsGain>
        <hcellsTemporalConstant>0.5</hcellsTemporalConstant>
        <hcellsSpatialConstant>7.</hcellsSpatialConstant>
        <ganglionCellsSensitivity>7.5e-01</ganglionCellsSensitivity></OPLandIPLparvo>
    <IPLmagno>
        <normaliseOutput>1</normaliseOutput>
        <parasolCells_beta>0.</parasolCells_beta>
        <parasolCells_tau>0.</parasolCells_tau>
        <parasolCells_k>7.</parasolCells_k>
        <amacrinCellsTemporalCutFrequency>2.0e+00</amacrinCellsTemporalCutFrequency>
        <V0CompressionParameter>9.5e-01</V0CompressionParameter>
        <localAdaptintegration_tau>0.</localAdaptintegration_tau>
        <localAdaptintegration_k>7.</localAdaptintegration_k></IPLmagno>
    </opencv_storage>
    @endcode

    Here is the 'realistic" setup used to obtain the second retina output shown on the top of this page.

    @code{xml}
    <?xml version="1.0"?>
    <opencv_storage>
    <OPLandIPLparvo>
      <colorMode>1</colorMode>
      <normaliseOutput>1</normaliseOutput>
      <photoreceptorsLocalAdaptationSensitivity>8.9e-01</photoreceptorsLocalAdaptationSensitivity>
      <photoreceptorsTemporalConstant>9.0e-01</photoreceptorsTemporalConstant>
      <photoreceptorsSpatialConstant>5.3e-01</photoreceptorsSpatialConstant>
      <horizontalCellsGain>0.3</horizontalCellsGain>
      <hcellsTemporalConstant>0.5</hcellsTemporalConstant>
      <hcellsSpatialConstant>7.</hcellsSpatialConstant>
      <ganglionCellsSensitivity>8.9e-01</ganglionCellsSensitivity></OPLandIPLparvo>
    <IPLmagno>
      <normaliseOutput>1</normaliseOutput>
      <parasolCells_beta>0.</parasolCells_beta>
      <parasolCells_tau>0.</parasolCells_tau>
      <parasolCells_k>7.</parasolCells_k>
      <amacrinCellsTemporalCutFrequency>2.0e+00</amacrinCellsTemporalCutFrequency>
      <V0CompressionParameter>9.5e-01</V0CompressionParameter>
      <localAdaptintegration_tau>0.</localAdaptintegration_tau>
      <localAdaptintegration_k>7.</localAdaptintegration_k></IPLmagno>
    </opencv_storage>
    @endcode
      */
    struct RetinaParameters{
        //! Outer Plexiform Layer (OPL) and Inner Plexiform Layer Parvocellular (IplParvo) parameters
        struct OPLandIplParvoParameters{
               OPLandIplParvoParameters():colorMode(true),
                                 normaliseOutput(true),
                                 photoreceptorsLocalAdaptationSensitivity(0.75f),
                                 photoreceptorsTemporalConstant(0.9f),
                                 photoreceptorsSpatialConstant(0.53f),
                                 horizontalCellsGain(0.01f),
                                 hcellsTemporalConstant(0.5f),
                                 hcellsSpatialConstant(7.f),
                                 ganglionCellsSensitivity(0.75f) { } // default setup
               bool colorMode, normaliseOutput;
               float photoreceptorsLocalAdaptationSensitivity, photoreceptorsTemporalConstant, photoreceptorsSpatialConstant, horizontalCellsGain, hcellsTemporalConstant, hcellsSpatialConstant, ganglionCellsSensitivity;
        };
        //! Inner Plexiform Layer Magnocellular channel (IplMagno)
        struct IplMagnoParameters{
            IplMagnoParameters():
                          normaliseOutput(true),
                          parasolCells_beta(0.f),
                          parasolCells_tau(0.f),
                          parasolCells_k(7.f),
                          amacrinCellsTemporalCutFrequency(2.0f),
                          V0CompressionParameter(0.95f),
                          localAdaptintegration_tau(0.f),
                          localAdaptintegration_k(7.f) { } // default setup
            bool normaliseOutput;
            float parasolCells_beta, parasolCells_tau, parasolCells_k, amacrinCellsTemporalCutFrequency, V0CompressionParameter, localAdaptintegration_tau, localAdaptintegration_k;
        };
        OPLandIplParvoParameters OPLandIplParvo;
        IplMagnoParameters IplMagno;
    };



/** @brief class which allows the Gipsa/Listic Labs model to be used with OpenCV.

This retina model allows spatio-temporal image processing (applied on still images, video sequences).
As a summary, these are the retina model properties:
- It applies a spectral whithening (mid-frequency details enhancement)
- high frequency spatio-temporal noise reduction
- low frequency luminance to be reduced (luminance range compression)
- local logarithmic luminance compression allows details to be enhanced in low light conditions

USE : this model can be used basically for spatio-temporal video effects but also for :
     _using the getParvo method output matrix : texture analysiswith enhanced signal to noise ratio and enhanced details robust against input images luminance ranges
     _using the getMagno method output matrix : motion analysis also with the previously cited properties

for more information, reer to the following papers :
Benoit A., Caplier A., Durette B., Herault, J., "USING HUMAN VISUAL SYSTEM MODELING FOR BIO-INSPIRED LOW LEVEL IMAGE PROCESSING", Elsevier, Computer Vision and Image Understanding 114 (2010), pp. 758-773, DOI: http://dx.doi.org/10.1016/j.cviu.2010.01.011
Vision: Images, Signals and Neural Networks: Models of Neural Processing in Visual Perception (Progress in Neural Processing),By: Jeanny Herault, ISBN: 9814273686. WAPI (Tower ID): 113266891.

The retina filter includes the research contributions of phd/research collegues from which code has been redrawn by the author :
take a look at the retinacolor.hpp module to discover Brice Chaix de Lavarene color mosaicing/demosaicing and the reference paper:
B. Chaix de Lavarene, D. Alleysson, B. Durette, J. Herault (2007). "Efficient demosaicing through recursive filtering", IEEE International Conference on Image Processing ICIP 2007
take a look at imagelogpolprojection.hpp to discover retina spatial log sampling which originates from Barthelemy Durette phd with Jeanny Herault. A Retina / V1 cortex projection is also proposed and originates from Jeanny's discussions.
more informations in the above cited Jeanny Heraults's book.
 */
class CV_EXPORTS_W Retina : public Algorithm {

public:


    /** @brief Retreive retina input buffer size
    @return the retina input buffer size
     */
    CV_WRAP virtual Size getInputSize()=0;

    /** @brief Retreive retina output buffer size that can be different from the input if a spatial log
    transformation is applied
    @return the retina output buffer size
     */
    CV_WRAP virtual Size getOutputSize()=0;

    /** @brief Try to open an XML retina parameters file to adjust current retina instance setup

    - if the xml file does not exist, then default setup is applied
    - warning, Exceptions are thrown if read XML file is not valid
    @param retinaParameterFile the parameters filename
    @param applyDefaultSetupOnFailure set to true if an error must be thrown on error

    You can retrieve the current parameters structure using the method Retina::getParameters and update
    it before running method Retina::setup.
     */
    CV_WRAP virtual void setup(String retinaParameterFile="", const bool applyDefaultSetupOnFailure=true)=0;

    /** @overload
    @param fs the open Filestorage which contains retina parameters
    @param applyDefaultSetupOnFailure set to true if an error must be thrown on error
    */
    virtual void setup(cv::FileStorage &fs, const bool applyDefaultSetupOnFailure=true)=0;

    /** @overload
    @param newParameters a parameters structures updated with the new target configuration.
    */
    virtual void setup(RetinaParameters newParameters)=0;

    /**
    @return the current parameters setup
    */
    virtual RetinaParameters getParameters()=0;

    /** @brief Outputs a string showing the used parameters setup
    @return a string which contains formated parameters information
     */
    CV_WRAP virtual const String printSetup()=0;

    /** @brief Write xml/yml formated parameters information
    @param fs the filename of the xml file that will be open and writen with formatted parameters
    information
     */
    CV_WRAP virtual void write( String fs ) const=0;

    /** @overload */
    virtual void write( FileStorage& fs ) const CV_OVERRIDE = 0;

    /** @brief Setup the OPL and IPL parvo channels (see biologocal model)

    OPL is referred as Outer Plexiform Layer of the retina, it allows the spatio-temporal filtering
    which withens the spectrum and reduces spatio-temporal noise while attenuating global luminance
    (low frequency energy) IPL parvo is the OPL next processing stage, it refers to a part of the
    Inner Plexiform layer of the retina, it allows high contours sensitivity in foveal vision. See
    reference papers for more informations.
    for more informations, please have a look at the paper Benoit A., Caplier A., Durette B., Herault, J., "USING HUMAN VISUAL SYSTEM MODELING FOR BIO-INSPIRED LOW LEVEL IMAGE PROCESSING", Elsevier, Computer Vision and Image Understanding 114 (2010), pp. 758-773, DOI: http://dx.doi.org/10.1016/j.cviu.2010.01.011
    @param colorMode specifies if (true) color is processed of not (false) to then processing gray
    level image
    @param normaliseOutput specifies if (true) output is rescaled between 0 and 255 of not (false)
    @param photoreceptorsLocalAdaptationSensitivity the photoreceptors sensitivity renage is 0-1
    (more log compression effect when value increases)
    @param photoreceptorsTemporalConstant the time constant of the first order low pass filter of
    the photoreceptors, use it to cut high temporal frequencies (noise or fast motion), unit is
    frames, typical value is 1 frame
    @param photoreceptorsSpatialConstant the spatial constant of the first order low pass filter of
    the photoreceptors, use it to cut high spatial frequencies (noise or thick contours), unit is
    pixels, typical value is 1 pixel
    @param horizontalCellsGain gain of the horizontal cells network, if 0, then the mean value of
    the output is zero, if the parameter is near 1, then, the luminance is not filtered and is
    still reachable at the output, typicall value is 0
    @param HcellsTemporalConstant the time constant of the first order low pass filter of the
    horizontal cells, use it to cut low temporal frequencies (local luminance variations), unit is
    frames, typical value is 1 frame, as the photoreceptors
    @param HcellsSpatialConstant the spatial constant of the first order low pass filter of the
    horizontal cells, use it to cut low spatial frequencies (local luminance), unit is pixels,
    typical value is 5 pixel, this value is also used for local contrast computing when computing
    the local contrast adaptation at the ganglion cells level (Inner Plexiform Layer parvocellular
    channel model)
    @param ganglionCellsSensitivity the compression strengh of the ganglion cells local adaptation
    output, set a value between 0.6 and 1 for best results, a high value increases more the low
    value sensitivity... and the output saturates faster, recommended value: 0.7
     */
    CV_WRAP virtual void setupOPLandIPLParvoChannel(const bool colorMode=true, const bool normaliseOutput = true, const float photoreceptorsLocalAdaptationSensitivity=0.7f, const float photoreceptorsTemporalConstant=0.5f, const float photoreceptorsSpatialConstant=0.53f, const float horizontalCellsGain=0.f, const float HcellsTemporalConstant=1.f, const float HcellsSpatialConstant=7.f, const float ganglionCellsSensitivity=0.7f)=0;

    /** @brief Set parameters values for the Inner Plexiform Layer (IPL) magnocellular channel

    this channel processes signals output from OPL processing stage in peripheral vision, it allows
    motion information enhancement. It is decorrelated from the details channel. See reference
    papers for more details.

    @param normaliseOutput specifies if (true) output is rescaled between 0 and 255 of not (false)
    @param parasolCells_beta the low pass filter gain used for local contrast adaptation at the
    IPL level of the retina (for ganglion cells local adaptation), typical value is 0
    @param parasolCells_tau the low pass filter time constant used for local contrast adaptation
    at the IPL level of the retina (for ganglion cells local adaptation), unit is frame, typical
    value is 0 (immediate response)
    @param parasolCells_k the low pass filter spatial constant used for local contrast adaptation
    at the IPL level of the retina (for ganglion cells local adaptation), unit is pixels, typical
    value is 5
    @param amacrinCellsTemporalCutFrequency the time constant of the first order high pass fiter of
    the magnocellular way (motion information channel), unit is frames, typical value is 1.2
    @param V0CompressionParameter the compression strengh of the ganglion cells local adaptation
    output, set a value between 0.6 and 1 for best results, a high value increases more the low
    value sensitivity... and the output saturates faster, recommended value: 0.95
    @param localAdaptintegration_tau specifies the temporal constant of the low pas filter
    involved in the computation of the local "motion mean" for the local adaptation computation
    @param localAdaptintegration_k specifies the spatial constant of the low pas filter involved
    in the computation of the local "motion mean" for the local adaptation computation
     */
    CV_WRAP virtual void setupIPLMagnoChannel(const bool normaliseOutput = true, const float parasolCells_beta=0.f, const float parasolCells_tau=0.f, const float parasolCells_k=7.f, const float amacrinCellsTemporalCutFrequency=1.2f, const float V0CompressionParameter=0.95f, const float localAdaptintegration_tau=0.f, const float localAdaptintegration_k=7.f)=0;

    /** @brief Method which allows retina to be applied on an input image,

    after run, encapsulated retina module is ready to deliver its outputs using dedicated
    acccessors, see getParvo and getMagno methods
    @param inputImage the input Mat image to be processed, can be gray level or BGR coded in any
    format (from 8bit to 16bits)
     */
    CV_WRAP virtual void run(InputArray inputImage)=0;

    /** @brief Method which processes an image in the aim to correct its luminance correct
    backlight problems, enhance details in shadows.

    This method is designed to perform High Dynamic Range image tone mapping (compress \>8bit/pixel
    images to 8bit/pixel). This is a simplified version of the Retina Parvocellular model
    (simplified version of the run/getParvo methods call) since it does not include the
    spatio-temporal filter modelling the Outer Plexiform Layer of the retina that performs spectral
    whitening and many other stuff. However, it works great for tone mapping and in a faster way.

    Check the demos and experiments section to see examples and the way to perform tone mapping
    using the original retina model and the method.

    @param inputImage the input image to process (should be coded in float format : CV_32F,
    CV_32FC1, CV_32F_C3, CV_32F_C4, the 4th channel won't be considered).
    @param outputToneMappedImage the output 8bit/channel tone mapped image (CV_8U or CV_8UC3 format).
     */
    CV_WRAP virtual void applyFastToneMapping(InputArray inputImage, OutputArray outputToneMappedImage)=0;

    /** @brief Accessor of the details channel of the retina (models foveal vision).

    Warning, getParvoRAW methods return buffers that are not rescaled within range [0;255] while
    the non RAW method allows a normalized matrix to be retrieved.

    @param retinaOutput_parvo the output buffer (reallocated if necessary), format can be :
    -   a Mat, this output is rescaled for standard 8bits image processing use in OpenCV
    -   RAW methods actually return a 1D matrix (encoding is R1, R2, ... Rn, G1, G2, ..., Gn, B1,
    B2, ...Bn), this output is the original retina filter model output, without any
    quantification or rescaling.
    @see getParvoRAW
     */
    CV_WRAP virtual void getParvo(OutputArray retinaOutput_parvo)=0;

    /** @brief Accessor of the details channel of the retina (models foveal vision).
    @see getParvo
     */
    CV_WRAP virtual void getParvoRAW(OutputArray retinaOutput_parvo)=0;

    /** @brief Accessor of the motion channel of the retina (models peripheral vision).

    Warning, getMagnoRAW methods return buffers that are not rescaled within range [0;255] while
    the non RAW method allows a normalized matrix to be retrieved.
    @param retinaOutput_magno the output buffer (reallocated if necessary), format can be :
    -   a Mat, this output is rescaled for standard 8bits image processing use in OpenCV
    -   RAW methods actually return a 1D matrix (encoding is M1, M2,... Mn), this output is the
    original retina filter model output, without any quantification or rescaling.
    @see getMagnoRAW
     */
    CV_WRAP virtual void getMagno(OutputArray retinaOutput_magno)=0;

    /** @brief Accessor of the motion channel of the retina (models peripheral vision).
    @see getMagno
     */
    CV_WRAP virtual void getMagnoRAW(OutputArray retinaOutput_magno)=0;

    /** @overload */
    CV_WRAP virtual const Mat getMagnoRAW() const=0;
    /** @overload */
    CV_WRAP virtual const Mat getParvoRAW() const=0;

    /** @brief Activate color saturation as the final step of the color demultiplexing process -\> this
    saturation is a sigmoide function applied to each channel of the demultiplexed image.
    @param saturateColors boolean that activates color saturation (if true) or desactivate (if false)
    @param colorSaturationValue the saturation factor : a simple factor applied on the chrominance
    buffers
     */
    CV_WRAP virtual void setColorSaturation(const bool saturateColors=true, const float colorSaturationValue=4.0f)=0;

    /** @brief Clears all retina buffers

    (equivalent to opening the eyes after a long period of eye close ;o) whatchout the temporal
    transition occuring just after this method call.
     */
    CV_WRAP virtual void clearBuffers()=0;

    /** @brief Activate/desactivate the Magnocellular pathway processing (motion information extraction), by
    default, it is activated
    @param activate true if Magnocellular output should be activated, false if not... if activated,
    the Magnocellular output can be retrieved using the **getMagno** methods
     */
    CV_WRAP virtual void activateMovingContoursProcessing(const bool activate)=0;

    /** @brief Activate/desactivate the Parvocellular pathway processing (contours information extraction), by
    default, it is activated
    @param activate true if Parvocellular (contours information extraction) output should be
    activated, false if not... if activated, the Parvocellular output can be retrieved using the
    Retina::getParvo methods
     */
    CV_WRAP virtual void activateContoursProcessing(const bool activate)=0;

    /** @overload */
    CV_WRAP static Ptr<Retina> create(Size inputSize);
    /** @brief Constructors from standardized interfaces : retreive a smart pointer to a Retina instance

    @param inputSize the input frame size
    @param colorMode the chosen processing mode : with or without color processing
    @param colorSamplingMethod specifies which kind of color sampling will be used :
    -   cv::bioinspired::RETINA_COLOR_RANDOM: each pixel position is either R, G or B in a random choice
    -   cv::bioinspired::RETINA_COLOR_DIAGONAL: color sampling is RGBRGBRGB..., line 2 BRGBRGBRG..., line 3, GBRGBRGBR...
    -   cv::bioinspired::RETINA_COLOR_BAYER: standard bayer sampling
    @param useRetinaLogSampling activate retina log sampling, if true, the 2 following parameters can
    be used
    @param reductionFactor only usefull if param useRetinaLogSampling=true, specifies the reduction
    factor of the output frame (as the center (fovea) is high resolution and corners can be
    underscaled, then a reduction of the output is allowed without precision leak
    @param samplingStrength only usefull if param useRetinaLogSampling=true, specifies the strength of
    the log scale that is applied
     */
    CV_WRAP static Ptr<Retina> create(Size inputSize, const bool colorMode,
                                           int colorSamplingMethod=RETINA_COLOR_BAYER,
                                           const bool useRetinaLogSampling=false,
                                           const float reductionFactor=1.0f, const float samplingStrength=10.0f);
};

//! @}

}
}
#endif /* __OPENCV_BIOINSPIRED_RETINA_HPP__ */
