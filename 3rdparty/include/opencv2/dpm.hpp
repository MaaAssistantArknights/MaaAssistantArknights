/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2015, Itseez Inc, all rights reserved.
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
// In no event shall the Itseez Inc or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
// Implementation authors:
// Jiaolong Xu - jiaolongxu@gmail.com
// Evgeniy Kozinov - evgeniy.kozinov@gmail.com
// Valentina Kustikova - valentina.kustikova@gmail.com
// Nikolai Zolotykh - Nikolai.Zolotykh@gmail.com
// Iosif Meyerov - meerov@vmk.unn.ru
// Alexey Polovinkin - polovinkin.alexey@gmail.com
//
//M*/

#ifndef __OPENCV_LATENTSVM_HPP__
#define __OPENCV_LATENTSVM_HPP__

#include "opencv2/core.hpp"

#include <map>
#include <vector>
#include <string>

/** @defgroup dpm Deformable Part-based Models

Discriminatively Trained Part Based Models for Object Detection
---------------------------------------------------------------

The object detector described below has been initially proposed by P.F. Felzenszwalb in
@cite Felzenszwalb2010a . It is based on a Dalal-Triggs detector that uses a single filter on histogram
of oriented gradients (HOG) features to represent an object category. This detector uses a sliding
window approach, where a filter is applied at all positions and scales of an image. The first
innovation is enriching the Dalal-Triggs model using a star-structured part-based model defined by a
"root" filter (analogous to the Dalal-Triggs filter) plus a set of parts filters and associated
deformation models. The score of one of star models at a particular position and scale within an
image is the score of the root filter at the given location plus the sum over parts of the maximum,
over placements of that part, of the part filter score on its location minus a deformation cost
easuring the deviation of the part from its ideal location relative to the root. Both root and part
filter scores are defined by the dot product between a filter (a set of weights) and a subwindow of
a feature pyramid computed from the input image. Another improvement is a representation of the
class of models by a mixture of star models. The score of a mixture model at a particular position
and scale is the maximum over components, of the score of that component model at the given
location.

The detector was dramatically speeded-up with cascade algorithm proposed by P.F. Felzenszwalb in
@cite Felzenszwalb2010b . The algorithm prunes partial hypotheses using thresholds on their scores.The
basic idea of the algorithm is to use a hierarchy of models defined by an ordering of the original
model's parts. For a model with (n+1) parts, including the root, a sequence of (n+1) models is
obtained. The i-th model in this sequence is defined by the first i parts from the original model.
Using this hierarchy, low scoring hypotheses can be pruned after looking at the best configuration
of a subset of the parts. Hypotheses that score high under a weak model are evaluated further using
a richer model.

In OpenCV there is an C++ implementation of DPM cascade detector.

*/

namespace cv
{

namespace dpm
{

//! @addtogroup dpm
//! @{

/** @brief This is a C++ abstract class, it provides external user API to work with DPM.
 */
class CV_EXPORTS_W DPMDetector
{
public:

    struct CV_EXPORTS_W ObjectDetection
    {
        ObjectDetection();
        ObjectDetection( const Rect& rect, float score, int classID=-1 );
        Rect rect;
        float score;
        int classID;
    };

    virtual bool isEmpty() const = 0;

    /** @brief Find rectangular regions in the given image that are likely to contain objects of loaded classes
    (models) and corresponding confidence levels.
    @param image An image.
    @param objects The detections: rectangulars, scores and class IDs.
    */
    virtual void detect(cv::Mat &image, CV_OUT std::vector<ObjectDetection> &objects) = 0;

    /** @brief Return the class (model) names that were passed in constructor or method load or extracted from
    models filenames in those methods.
     */
    virtual std::vector<std::string> const& getClassNames() const = 0;

    /** @brief Return a count of loaded models (classes).
     */
    virtual size_t getClassCount() const = 0;

    /** @brief Load the trained models from given .xml files and return cv::Ptr\<DPMDetector\>.
    @param filenames A set of filenames storing the trained detectors (models). Each file contains one
    model. See examples of such files here `/opencv_extra/testdata/cv/dpm/VOC2007_Cascade/`.
    @param classNames A set of trained models names. If it's empty then the name of each model will be
    constructed from the name of file containing the model. E.g. the model stored in
    "/home/user/cat.xml" will get the name "cat".
     */
    static cv::Ptr<DPMDetector> create(std::vector<std::string> const &filenames,
            std::vector<std::string> const &classNames = std::vector<std::string>());

    virtual ~DPMDetector(){}
};

//! @}

} // namespace dpm
} // namespace cv

#endif
