// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#ifndef __OPENCV_TEXT_TEXTDETECTOR_HPP__
#define __OPENCV_TEXT_TEXTDETECTOR_HPP__

#include "ocr.hpp"

namespace cv
{
namespace text
{

//! @addtogroup text_detect
//! @{

/** @brief An abstract class providing interface for text detection algorithms
 */
class CV_EXPORTS_W TextDetector
{
public:
    /**
    @brief Method that provides a quick and simple interface to detect text inside an image

    @param inputImage an image to process
    @param Bbox a vector of Rect that will store the detected word bounding box
    @param confidence a vector of float that will be updated with the confidence the classifier has for the selected bounding box
    */
    CV_WRAP virtual void detect(InputArray inputImage, CV_OUT std::vector<Rect>& Bbox, CV_OUT std::vector<float>& confidence) = 0;
    virtual ~TextDetector() {}
};

/** @brief TextDetectorCNN class provides the functionallity of text bounding box detection.
 This class is representing to find bounding boxes of text words given an input image.
 This class uses OpenCV dnn module to load pre-trained model described in @cite LiaoSBWL17.
 The original repository with the modified SSD Caffe version: https://github.com/MhLiao/TextBoxes.
 Model can be downloaded from [DropBox](https://www.dropbox.com/s/g8pjzv2de9gty8g/TextBoxes_icdar13.caffemodel?dl=0).
 Modified .prototxt file with the model description can be found in `opencv_contrib/modules/text/samples/textbox.prototxt`.
 */
class CV_EXPORTS_W TextDetectorCNN : public TextDetector
{
public:
    /**
    @overload

    @param inputImage an image expected to be a CV_U8C3 of any size
    @param Bbox a vector of Rect that will store the detected word bounding box
    @param confidence a vector of float that will be updated with the confidence the classifier has for the selected bounding box
    */
    CV_WRAP virtual void detect(InputArray inputImage, CV_OUT std::vector<Rect>& Bbox, CV_OUT std::vector<float>& confidence) CV_OVERRIDE = 0;

    /** @brief Creates an instance of the TextDetectorCNN class using the provided parameters.

    @param modelArchFilename the relative or absolute path to the prototxt file describing the classifiers architecture.
    @param modelWeightsFilename the relative or absolute path to the file containing the pretrained weights of the model in caffe-binary form.
    @param detectionSizes a list of sizes for multiscale detection. The values`[(300,300),(700,500),(700,300),(700,700),(1600,1600)]` are
    recommended in @cite LiaoSBWL17 to achieve the best quality.
    */
    static Ptr<TextDetectorCNN> create(const String& modelArchFilename, const String& modelWeightsFilename,
                                               std::vector<Size> detectionSizes);
    /**
      @overload
    */
    CV_WRAP static Ptr<TextDetectorCNN> create(const String& modelArchFilename, const String& modelWeightsFilename);
};

//! @}
}//namespace text
}//namespace cv


#endif // _OPENCV_TEXT_OCR_HPP_
