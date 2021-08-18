/*M//////////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                          License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Copyright (C) 2013, OpenCV Foundation, all rights reserved.
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

#ifndef __OPENCV_TEXT_OCR_HPP__
#define __OPENCV_TEXT_OCR_HPP__

#include <opencv2/core.hpp>

#include <vector>
#include <string>

namespace cv
{
namespace text
{

//! @addtogroup text_recognize
//! @{

enum
{
    OCR_LEVEL_WORD,
    OCR_LEVEL_TEXTLINE
};

//! Tesseract.PageSegMode Enumeration
enum page_seg_mode
{
    PSM_OSD_ONLY,
    PSM_AUTO_OSD,
    PSM_AUTO_ONLY,
    PSM_AUTO,
    PSM_SINGLE_COLUMN,
    PSM_SINGLE_BLOCK_VERT_TEXT,
    PSM_SINGLE_BLOCK,
    PSM_SINGLE_LINE,
    PSM_SINGLE_WORD,
    PSM_CIRCLE_WORD,
    PSM_SINGLE_CHAR
};

//! Tesseract.OcrEngineMode Enumeration
enum ocr_engine_mode
{
    OEM_TESSERACT_ONLY,
    OEM_CUBE_ONLY,
    OEM_TESSERACT_CUBE_COMBINED,
    OEM_DEFAULT
};

//base class BaseOCR declares a common API that would be used in a typical text recognition scenario
class CV_EXPORTS_W BaseOCR
{
public:
    virtual ~BaseOCR() {};
    virtual void run(Mat& image, std::string& output_text, std::vector<Rect>* component_rects=NULL,
                     std::vector<std::string>* component_texts=NULL, std::vector<float>* component_confidences=NULL,
                     int component_level=0) = 0;
    virtual void run(Mat& image, Mat& mask, std::string& output_text, std::vector<Rect>* component_rects=NULL,
                     std::vector<std::string>* component_texts=NULL, std::vector<float>* component_confidences=NULL,
                     int component_level=0) = 0;
};

/** @brief OCRTesseract class provides an interface with the tesseract-ocr API (v3.02.02) in C++.

Notice that it is compiled only when tesseract-ocr is correctly installed.

@note
   -   (C++) An example of OCRTesseract recognition combined with scene text detection can be found
        at the end_to_end_recognition demo:
        <https://github.com/opencv/opencv_contrib/blob/master/modules/text/samples/end_to_end_recognition.cpp>
    -   (C++) Another example of OCRTesseract recognition combined with scene text detection can be
        found at the webcam_demo:
        <https://github.com/opencv/opencv_contrib/blob/master/modules/text/samples/webcam_demo.cpp>
 */
class CV_EXPORTS_W OCRTesseract : public BaseOCR
{
public:
    /** @brief Recognize text using the tesseract-ocr API.

    Takes image on input and returns recognized text in the output_text parameter. Optionally
    provides also the Rects for individual text elements found (e.g. words), and the list of those
    text elements with their confidence values.

    @param image Input image CV_8UC1 or CV_8UC3
    @param output_text Output text of the tesseract-ocr.
    @param component_rects If provided the method will output a list of Rects for the individual
    text elements found (e.g. words or text lines).
    @param component_texts If provided the method will output a list of text strings for the
    recognition of individual text elements found (e.g. words or text lines).
    @param component_confidences If provided the method will output a list of confidence values
    for the recognition of individual text elements found (e.g. words or text lines).
    @param component_level OCR_LEVEL_WORD (by default), or OCR_LEVEL_TEXTLINE.
     */
    virtual void run(Mat& image, std::string& output_text, std::vector<Rect>* component_rects=NULL,
                     std::vector<std::string>* component_texts=NULL, std::vector<float>* component_confidences=NULL,
                     int component_level=0) CV_OVERRIDE;

    virtual void run(Mat& image, Mat& mask, std::string& output_text, std::vector<Rect>* component_rects=NULL,
                     std::vector<std::string>* component_texts=NULL, std::vector<float>* component_confidences=NULL,
                     int component_level=0) CV_OVERRIDE;

    // aliases for scripting
    CV_WRAP String run(InputArray image, int min_confidence, int component_level=0);

    CV_WRAP String run(InputArray image, InputArray mask, int min_confidence, int component_level=0);

    CV_WRAP virtual void setWhiteList(const String& char_whitelist) = 0;


    /** @brief Creates an instance of the OCRTesseract class. Initializes Tesseract.

    @param datapath the name of the parent directory of tessdata ended with "/", or NULL to use the
    system's default directory.
    @param language an ISO 639-3 code or NULL will default to "eng".
    @param char_whitelist specifies the list of characters used for recognition. NULL defaults to
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ".
    @param oem tesseract-ocr offers different OCR Engine Modes (OEM), by default
    tesseract::OEM_DEFAULT is used. See the tesseract-ocr API documentation for other possible
    values.
    @param psmode tesseract-ocr offers different Page Segmentation Modes (PSM) tesseract::PSM_AUTO
    (fully automatic layout analysis) is used. See the tesseract-ocr API documentation for other
    possible values.
     */
    CV_WRAP static Ptr<OCRTesseract> create(const char* datapath=NULL, const char* language=NULL,
                                    const char* char_whitelist=NULL, int oem=OEM_DEFAULT, int psmode=PSM_AUTO);
};


/* OCR HMM Decoder */

enum decoder_mode
{
    OCR_DECODER_VITERBI = 0 // Other algorithms may be added
};

/* OCR classifier type*/
enum classifier_type
{
    OCR_KNN_CLASSIFIER = 0,
    OCR_CNN_CLASSIFIER = 1
};

/** @brief OCRHMMDecoder class provides an interface for OCR using Hidden Markov Models.

@note
   -   (C++) An example on using OCRHMMDecoder recognition combined with scene text detection can
        be found at the webcam_demo sample:
        <https://github.com/opencv/opencv_contrib/blob/master/modules/text/samples/webcam_demo.cpp>
 */
class CV_EXPORTS_W OCRHMMDecoder : public BaseOCR
{
public:

    /** @brief Callback with the character classifier is made a class.

    This way it hides the feature extractor and the classifier itself, so developers can write
    their own OCR code.

    The default character classifier and feature extractor can be loaded using the utility function
    loadOCRHMMClassifierNM and KNN model provided in
    <https://github.com/opencv/opencv_contrib/blob/master/modules/text/samples/OCRHMM_knn_model_data.xml.gz>.
     */
    class CV_EXPORTS_W ClassifierCallback
    {
    public:
        virtual ~ClassifierCallback() { }
        /** @brief The character classifier must return a (ranked list of) class(es) id('s)

        @param image Input image CV_8UC1 or CV_8UC3 with a single letter.
        @param out_class The classifier returns the character class categorical label, or list of
        class labels, to which the input image corresponds.
        @param out_confidence The classifier returns the probability of the input image
        corresponding to each classes in out_class.
         */
        virtual void eval( InputArray image, std::vector<int>& out_class, std::vector<double>& out_confidence);
    };

public:
    /** @brief Recognize text using HMM.

    Takes binary image on input and returns recognized text in the output_text parameter. Optionally
    provides also the Rects for individual text elements found (e.g. words), and the list of those
    text elements with their confidence values.

    @param image Input binary image CV_8UC1 with a single text line (or word).

    @param output_text Output text. Most likely character sequence found by the HMM decoder.

    @param component_rects If provided the method will output a list of Rects for the individual
    text elements found (e.g. words).

    @param component_texts If provided the method will output a list of text strings for the
    recognition of individual text elements found (e.g. words).

    @param component_confidences If provided the method will output a list of confidence values
    for the recognition of individual text elements found (e.g. words).

    @param component_level Only OCR_LEVEL_WORD is supported.
     */
    virtual void run(Mat& image, std::string& output_text, std::vector<Rect>* component_rects=NULL,
                     std::vector<std::string>* component_texts=NULL, std::vector<float>* component_confidences=NULL,
                     int component_level=0) CV_OVERRIDE;

    /** @brief Recognize text using HMM.

    Takes an image and a mask (where each connected component corresponds to a segmented character)
    on input and returns recognized text in the output_text parameter. Optionally
    provides also the Rects for individual text elements found (e.g. words), and the list of those
    text elements with their confidence values.

    @param image Input image CV_8UC1 or CV_8UC3 with a single text line (or word).
    @param mask Input binary image CV_8UC1 same size as input image. Each connected component in mask corresponds to a segmented character in the input image.

    @param output_text Output text. Most likely character sequence found by the HMM decoder.

    @param component_rects If provided the method will output a list of Rects for the individual
    text elements found (e.g. words).

    @param component_texts If provided the method will output a list of text strings for the
    recognition of individual text elements found (e.g. words).

    @param component_confidences If provided the method will output a list of confidence values
    for the recognition of individual text elements found (e.g. words).

    @param component_level Only OCR_LEVEL_WORD is supported.
     */
    virtual void run(Mat& image, Mat& mask, std::string& output_text, std::vector<Rect>* component_rects=NULL,
                     std::vector<std::string>* component_texts=NULL, std::vector<float>* component_confidences=NULL,
                     int component_level=0) CV_OVERRIDE;

    // aliases for scripting
    CV_WRAP String run(InputArray image, int min_confidence, int component_level=0);

    CV_WRAP String run(InputArray image, InputArray mask, int min_confidence, int component_level=0);

    /** @brief Creates an instance of the OCRHMMDecoder class. Initializes HMMDecoder.

    @param classifier The character classifier with built in feature extractor.

    @param vocabulary The language vocabulary (chars when ascii english text). vocabulary.size()
    must be equal to the number of classes of the classifier.

    @param transition_probabilities_table Table with transition probabilities between character
    pairs. cols == rows == vocabulary.size().

    @param emission_probabilities_table Table with observation emission probabilities. cols ==
    rows == vocabulary.size().

    @param mode HMM Decoding algorithm. Only OCR_DECODER_VITERBI is available for the moment
    (<http://en.wikipedia.org/wiki/Viterbi_algorithm>).
     */
    static Ptr<OCRHMMDecoder> create(const Ptr<OCRHMMDecoder::ClassifierCallback> classifier,// The character classifier with built in feature extractor
                                     const std::string& vocabulary,                    // The language vocabulary (chars when ASCII English text)
                                                                                       //     size() must be equal to the number of classes
                                     InputArray transition_probabilities_table,        // Table with transition probabilities between character pairs
                                                                                       //     cols == rows == vocabulary.size()
                                     InputArray emission_probabilities_table,          // Table with observation emission probabilities
                                                                                       //     cols == rows == vocabulary.size()
                                     decoder_mode mode = OCR_DECODER_VITERBI);         // HMM Decoding algorithm (only Viterbi for the moment)

    CV_WRAP static Ptr<OCRHMMDecoder> create(const Ptr<OCRHMMDecoder::ClassifierCallback> classifier,// The character classifier with built in feature extractor
                                     const String& vocabulary,                    // The language vocabulary (chars when ASCII English text)
                                                                                       //     size() must be equal to the number of classes
                                     InputArray transition_probabilities_table,        // Table with transition probabilities between character pairs
                                                                                       //     cols == rows == vocabulary.size()
                                     InputArray emission_probabilities_table,          // Table with observation emission probabilities
                                                                                       //     cols == rows == vocabulary.size()
                                     int mode = OCR_DECODER_VITERBI);         // HMM Decoding algorithm (only Viterbi for the moment)

    /** @brief Creates an instance of the OCRHMMDecoder class. Loads and initializes HMMDecoder from the specified path

     @overload
     */
    CV_WRAP static Ptr<OCRHMMDecoder> create(const String& filename,

                                     const String& vocabulary,                    // The language vocabulary (chars when ASCII English text)
                                                                                       //     size() must be equal to the number of classes
                                     InputArray transition_probabilities_table,        // Table with transition probabilities between character pairs
                                                                                       //     cols == rows == vocabulary.size()
                                     InputArray emission_probabilities_table,          // Table with observation emission probabilities
                                                                                       //     cols == rows == vocabulary.size()
                                     int mode = OCR_DECODER_VITERBI,                    // HMM Decoding algorithm (only Viterbi for the moment)

                                     int classifier = OCR_KNN_CLASSIFIER);              // The character classifier type
protected:

    Ptr<OCRHMMDecoder::ClassifierCallback> classifier;
    std::string vocabulary;
    Mat transition_p;
    Mat emission_p;
    decoder_mode mode;
};

/** @brief Allow to implicitly load the default character classifier when creating an OCRHMMDecoder object.

@param filename The XML or YAML file with the classifier model (e.g. OCRHMM_knn_model_data.xml)

The KNN default classifier is based in the scene text recognition method proposed by Lukás Neumann &
Jiri Matas in [Neumann11b]. Basically, the region (contour) in the input image is normalized to a
fixed size, while retaining the centroid and aspect ratio, in order to extract a feature vector
based on gradient orientations along the chain-code of its perimeter. Then, the region is classified
using a KNN model trained with synthetic data of rendered characters with different standard font
types.

@deprecated loadOCRHMMClassifier instead
 */

CV_EXPORTS_W Ptr<OCRHMMDecoder::ClassifierCallback> loadOCRHMMClassifierNM(const String& filename);

/** @brief Allow to implicitly load the default character classifier when creating an OCRHMMDecoder object.

@param filename The XML or YAML file with the classifier model (e.g. OCRBeamSearch_CNN_model_data.xml.gz)

The CNN default classifier is based in the scene text recognition method proposed by Adam Coates &
Andrew NG in [Coates11a]. The character classifier consists in a Single Layer Convolutional Neural Network and
a linear classifier. It is applied to the input image in a sliding window fashion, providing a set of recognitions
at each window location.

@deprecated use loadOCRHMMClassifier instead
 */
CV_EXPORTS_W Ptr<OCRHMMDecoder::ClassifierCallback> loadOCRHMMClassifierCNN(const String& filename);

/** @brief Allow to implicitly load the default character classifier when creating an OCRHMMDecoder object.

 @param filename The XML or YAML file with the classifier model (e.g. OCRBeamSearch_CNN_model_data.xml.gz)

 @param classifier Can be one of classifier_type enum values.

 */
CV_EXPORTS_W Ptr<OCRHMMDecoder::ClassifierCallback> loadOCRHMMClassifier(const String& filename, int classifier);
//! @}

/** @brief Utility function to create a tailored language model transitions table from a given list of words (lexicon).
 *
 * @param vocabulary The language vocabulary (chars when ASCII English text).
 *
 * @param lexicon The list of words that are expected to be found in a particular image.
 *
 * @param transition_probabilities_table Output table with transition probabilities between character pairs. cols == rows == vocabulary.size().
 *
 * The function calculate frequency statistics of character pairs from the given lexicon and fills the output transition_probabilities_table with them. The transition_probabilities_table can be used as input in the OCRHMMDecoder::create() and OCRBeamSearchDecoder::create() methods.
 * @note
 *    -   (C++) An alternative would be to load the default generic language transition table provided in the text module samples folder (created from ispell 42869 english words list) :
 *            <https://github.com/opencv/opencv_contrib/blob/master/modules/text/samples/OCRHMM_transitions_table.xml>
 **/
CV_EXPORTS void createOCRHMMTransitionsTable(std::string& vocabulary, std::vector<std::string>& lexicon, OutputArray transition_probabilities_table);

CV_EXPORTS_W Mat createOCRHMMTransitionsTable(const String& vocabulary, std::vector<cv::String>& lexicon);


/* OCR BeamSearch Decoder */

/** @brief OCRBeamSearchDecoder class provides an interface for OCR using Beam Search algorithm.

@note
   -   (C++) An example on using OCRBeamSearchDecoder recognition combined with scene text detection can
        be found at the demo sample:
        <https://github.com/opencv/opencv_contrib/blob/master/modules/text/samples/word_recognition.cpp>
 */
class CV_EXPORTS_W OCRBeamSearchDecoder : public BaseOCR
{
public:

    /** @brief Callback with the character classifier is made a class.

    This way it hides the feature extractor and the classifier itself, so developers can write
    their own OCR code.

    The default character classifier and feature extractor can be loaded using the utility function
    loadOCRBeamSearchClassifierCNN with all its parameters provided in
    <https://github.com/opencv/opencv_contrib/blob/master/modules/text/samples/OCRBeamSearch_CNN_model_data.xml.gz>.
     */
    class CV_EXPORTS_W ClassifierCallback
    {
    public:
        virtual ~ClassifierCallback() { }
        /** @brief The character classifier must return a (ranked list of) class(es) id('s)

        @param image Input image CV_8UC1 or CV_8UC3 with a single letter.
        @param recognition_probabilities For each of the N characters found the classifier returns a list with
        class probabilities for each class.
        @param oversegmentation The classifier returns a list of N+1 character locations' x-coordinates,
        including 0 as start-sequence location.
         */
        virtual void eval( InputArray image, std::vector< std::vector<double> >& recognition_probabilities, std::vector<int>& oversegmentation );

        int getWindowSize() {return 0;}
        int getStepSize() {return 0;}
    };

public:
    /** @brief Recognize text using Beam Search.

    Takes image on input and returns recognized text in the output_text parameter. Optionally
    provides also the Rects for individual text elements found (e.g. words), and the list of those
    text elements with their confidence values.

    @param image Input binary image CV_8UC1 with a single text line (or word).

    @param output_text Output text. Most likely character sequence found by the HMM decoder.

    @param component_rects If provided the method will output a list of Rects for the individual
    text elements found (e.g. words).

    @param component_texts If provided the method will output a list of text strings for the
    recognition of individual text elements found (e.g. words).

    @param component_confidences If provided the method will output a list of confidence values
    for the recognition of individual text elements found (e.g. words).

    @param component_level Only OCR_LEVEL_WORD is supported.
     */
    virtual void run(Mat& image, std::string& output_text, std::vector<Rect>* component_rects=NULL,
                     std::vector<std::string>* component_texts=NULL, std::vector<float>* component_confidences=NULL,
                     int component_level=0) CV_OVERRIDE;

    virtual void run(Mat& image, Mat& mask, std::string& output_text, std::vector<Rect>* component_rects=NULL,
                     std::vector<std::string>* component_texts=NULL, std::vector<float>* component_confidences=NULL,
                     int component_level=0) CV_OVERRIDE;

    // aliases for scripting
    CV_WRAP String run(InputArray image, int min_confidence, int component_level=0);

    CV_WRAP String run(InputArray image, InputArray mask, int min_confidence, int component_level=0);

    /** @brief Creates an instance of the OCRBeamSearchDecoder class. Initializes HMMDecoder.

    @param classifier The character classifier with built in feature extractor.

    @param vocabulary The language vocabulary (chars when ASCII English text). vocabulary.size()
    must be equal to the number of classes of the classifier.

    @param transition_probabilities_table Table with transition probabilities between character
    pairs. cols == rows == vocabulary.size().

    @param emission_probabilities_table Table with observation emission probabilities. cols ==
    rows == vocabulary.size().

    @param mode HMM Decoding algorithm. Only OCR_DECODER_VITERBI is available for the moment
    (<http://en.wikipedia.org/wiki/Viterbi_algorithm>).

    @param beam_size Size of the beam in Beam Search algorithm.
     */
    static Ptr<OCRBeamSearchDecoder> create(const Ptr<OCRBeamSearchDecoder::ClassifierCallback> classifier,// The character classifier with built in feature extractor
                                     const std::string& vocabulary,                    // The language vocabulary (chars when ASCII English text)
                                                                                       //     size() must be equal to the number of classes
                                     InputArray transition_probabilities_table,        // Table with transition probabilities between character pairs
                                                                                       //     cols == rows == vocabulary.size()
                                     InputArray emission_probabilities_table,          // Table with observation emission probabilities
                                                                                       //     cols == rows == vocabulary.size()
                                     decoder_mode mode = OCR_DECODER_VITERBI,          // HMM Decoding algorithm (only Viterbi for the moment)
                                     int beam_size = 500);                              // Size of the beam in Beam Search algorithm

    CV_WRAP static Ptr<OCRBeamSearchDecoder> create(const Ptr<OCRBeamSearchDecoder::ClassifierCallback> classifier, // The character classifier with built in feature extractor
                                     const String& vocabulary,                    // The language vocabulary (chars when ASCII English text)
                                                                                       //     size() must be equal to the number of classes
                                     InputArray transition_probabilities_table,        // Table with transition probabilities between character pairs
                                                                                       //     cols == rows == vocabulary.size()
                                     InputArray emission_probabilities_table,          // Table with observation emission probabilities
                                                                                       //     cols == rows == vocabulary.size()
                                     int mode = OCR_DECODER_VITERBI,          // HMM Decoding algorithm (only Viterbi for the moment)
                                     int beam_size = 500);                              // Size of the beam in Beam Search algorithm

    /** @brief Creates an instance of the OCRBeamSearchDecoder class. Initializes HMMDecoder from the specified path.

    @overload

     */
    CV_WRAP static Ptr<OCRBeamSearchDecoder> create(const String& filename, // The character classifier file
                                     const String& vocabulary,                    // The language vocabulary (chars when ASCII English text)
                                                                                       //     size() must be equal to the number of classes
                                     InputArray transition_probabilities_table,        // Table with transition probabilities between character pairs
                                                                                       //     cols == rows == vocabulary.size()
                                     InputArray emission_probabilities_table,          // Table with observation emission probabilities
                                                                                       //     cols == rows == vocabulary.size()
                                     int mode = OCR_DECODER_VITERBI,          // HMM Decoding algorithm (only Viterbi for the moment)
                                     int beam_size = 500);
protected:

    Ptr<OCRBeamSearchDecoder::ClassifierCallback> classifier;
    std::string vocabulary;
    Mat transition_p;
    Mat emission_p;
    decoder_mode mode;
    int beam_size;
};

/** @brief Allow to implicitly load the default character classifier when creating an OCRBeamSearchDecoder object.

@param filename The XML or YAML file with the classifier model (e.g. OCRBeamSearch_CNN_model_data.xml.gz)

The CNN default classifier is based in the scene text recognition method proposed by Adam Coates &
Andrew NG in [Coates11a]. The character classifier consists in a Single Layer Convolutional Neural Network and
a linear classifier. It is applied to the input image in a sliding window fashion, providing a set of recognitions
at each window location.
 */

CV_EXPORTS_W Ptr<OCRBeamSearchDecoder::ClassifierCallback> loadOCRBeamSearchClassifierCNN(const String& filename);


/** @brief OCRHolisticWordRecognizer class provides the functionallity of segmented wordspotting.
 * Given a predefined vocabulary , a DictNet is employed to select the most probable
 * word given an input image.
 *
 * DictNet is described in detail in:
 * Max Jaderberg et al.: Reading Text in the Wild with Convolutional Neural Networks, IJCV 2015
 * http://arxiv.org/abs/1412.1842
 */
class CV_EXPORTS OCRHolisticWordRecognizer : public BaseOCR
{
public:
    virtual void run(Mat& image,
                     std::string& output_text,
                     std::vector<Rect>* component_rects = NULL,
                     std::vector<std::string>* component_texts = NULL,
                     std::vector<float>* component_confidences = NULL,
                     int component_level = OCR_LEVEL_WORD) CV_OVERRIDE = 0;

    /** @brief Recognize text using a segmentation based word-spotting/classifier cnn.

    Takes image on input and returns recognized text in the output_text parameter. Optionally
    provides also the Rects for individual text elements found (e.g. words), and the list of those
    text elements with their confidence values.

    @param image Input image CV_8UC1 or CV_8UC3

    @param mask is totally ignored and is only available for compatibillity reasons

    @param output_text Output text of the the word spoting, always one that exists in the dictionary.

    @param component_rects Not applicable for word spotting can be be NULL if not, a single elemnt will
        be put in the vector.

    @param component_texts Not applicable for word spotting can be be NULL if not, a single elemnt will
        be put in the vector.

    @param component_confidences Not applicable for word spotting can be be NULL if not, a single elemnt will
        be put in the vector.

    @param component_level must be OCR_LEVEL_WORD.
     */
    virtual void run(Mat& image,
                     Mat& mask,
                     std::string& output_text,
                     std::vector<Rect>* component_rects = NULL,
                     std::vector<std::string>* component_texts = NULL,
                     std::vector<float>* component_confidences = NULL,
                     int component_level = OCR_LEVEL_WORD) CV_OVERRIDE = 0;

    /** @brief Creates an instance of the OCRHolisticWordRecognizer class.
     */
    static Ptr<OCRHolisticWordRecognizer> create(const std::string &archFilename,
                                                 const std::string &weightsFilename,
                                                 const std::string &wordsFilename);
};

//! @}

}} // cv::text::


#endif // _OPENCV_TEXT_OCR_HPP_
