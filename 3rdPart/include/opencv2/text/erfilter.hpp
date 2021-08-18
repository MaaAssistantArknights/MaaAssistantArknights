/*M///////////////////////////////////////////////////////////////////////////////////////
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

#ifndef __OPENCV_TEXT_ERFILTER_HPP__
#define __OPENCV_TEXT_ERFILTER_HPP__

#include "opencv2/core.hpp"
#include <vector>
#include <deque>
#include <string>

namespace cv
{
namespace text
{

//! @addtogroup text_detect
//! @{

/** @brief The ERStat structure represents a class-specific Extremal Region (ER).

An ER is a 4-connected set of pixels with all its grey-level values smaller than the values in its
outer boundary. A class-specific ER is selected (using a classifier) from all the ER's in the
component tree of the image. :
 */
struct CV_EXPORTS ERStat
{
public:
    //! Constructor
    explicit ERStat(int level = 256, int pixel = 0, int x = 0, int y = 0);
    //! Destructor
    ~ERStat() { }

    //! seed point and the threshold (max grey-level value)
    int pixel;
    int level;

    //! incrementally computable features
    int area;
    int perimeter;
    int euler;                 //!< Euler's number
    Rect rect;
    double raw_moments[2];     //!< order 1 raw moments to derive the centroid
    double central_moments[3]; //!< order 2 central moments to construct the covariance matrix
    Ptr<std::deque<int> > crossings;//!< horizontal crossings
    float med_crossings;       //!< median of the crossings at three different height levels

    //! 2nd stage features
    float hole_area_ratio;
    float convex_hull_ratio;
    float num_inflexion_points;

    // TODO Other features can be added (average color, standard deviation, and such)


    // TODO shall we include the pixel list whenever available (i.e. after 2nd stage) ?
    std::vector<int> *pixels;

    //! probability that the ER belongs to the class we are looking for
    double probability;

    //! pointers preserving the tree structure of the component tree
    ERStat* parent;
    ERStat* child;
    ERStat* next;
    ERStat* prev;

    //! whenever the regions is a local maxima of the probability
    bool local_maxima;
    ERStat* max_probability_ancestor;
    ERStat* min_probability_ancestor;
};

/** @brief Base class for 1st and 2nd stages of Neumann and Matas scene text detection algorithm @cite Neumann12. :

Extracts the component tree (if needed) and filter the extremal regions (ER's) by using a given classifier.
 */
class CV_EXPORTS_W ERFilter : public Algorithm
{
public:

    /** @brief Callback with the classifier is made a class.

    By doing it we hide SVM, Boost etc. Developers can provide their own classifiers to the
    ERFilter algorithm.
     */
    class CV_EXPORTS_W Callback
    {
    public:
        virtual ~Callback() { }
        /** @brief The classifier must return probability measure for the region.

        @param  stat :   The region to be classified
         */
        virtual double eval(const ERStat& stat) = 0; //const = 0; //TODO why cannot use const = 0 here?
    };

    /** @brief The key method of ERFilter algorithm.

    Takes image on input and returns the selected regions in a vector of ERStat only distinctive
    ERs which correspond to characters are selected by a sequential classifier

    @param image Single channel image CV_8UC1

    @param regions Output for the 1st stage and Input/Output for the 2nd. The selected Extremal Regions
    are stored here.

    Extracts the component tree (if needed) and filter the extremal regions (ER's) by using a given
    classifier.
     */
    virtual void run( InputArray image, std::vector<ERStat>& regions ) = 0;


    //! set/get methods to set the algorithm properties,
    virtual void setCallback(const Ptr<ERFilter::Callback>& cb) = 0;
    virtual void setThresholdDelta(int thresholdDelta) = 0;
    virtual void setMinArea(float minArea) = 0;
    virtual void setMaxArea(float maxArea) = 0;
    virtual void setMinProbability(float minProbability) = 0;
    virtual void setMinProbabilityDiff(float minProbabilityDiff) = 0;
    virtual void setNonMaxSuppression(bool nonMaxSuppression) = 0;
    virtual int  getNumRejected() const = 0;
};



/** @brief Create an Extremal Region Filter for the 1st stage classifier of N&M algorithm @cite Neumann12.

@param  cb :   Callback with the classifier. Default classifier can be implicitly load with function
loadClassifierNM1, e.g. from file in samples/cpp/trained_classifierNM1.xml
@param  thresholdDelta :   Threshold step in subsequent thresholds when extracting the component tree
@param  minArea :   The minimum area (% of image size) allowed for retreived ER's
@param  maxArea :   The maximum area (% of image size) allowed for retreived ER's
@param  minProbability :   The minimum probability P(er|character) allowed for retreived ER's
@param  nonMaxSuppression :   Whenever non-maximum suppression is done over the branch probabilities
@param  minProbabilityDiff :   The minimum probability difference between local maxima and local minima ERs

The component tree of the image is extracted by a threshold increased step by step from 0 to 255,
incrementally computable descriptors (aspect_ratio, compactness, number of holes, and number of
horizontal crossings) are computed for each ER and used as features for a classifier which estimates
the class-conditional probability P(er|character). The value of P(er|character) is tracked using the
inclusion relation of ER across all thresholds and only the ERs which correspond to local maximum of
the probability P(er|character) are selected (if the local maximum of the probability is above a
global limit pmin and the difference between local maximum and local minimum is greater than
minProbabilityDiff).
 */
CV_EXPORTS_W Ptr<ERFilter> createERFilterNM1(const Ptr<ERFilter::Callback>& cb,
                                                  int thresholdDelta = 1, float minArea = (float)0.00025,
                                                  float maxArea = (float)0.13, float minProbability = (float)0.4,
                                                  bool nonMaxSuppression = true,
                                                  float minProbabilityDiff = (float)0.1);

/** @brief Create an Extremal Region Filter for the 2nd stage classifier of N&M algorithm @cite Neumann12.

@param  cb :   Callback with the classifier. Default classifier can be implicitly load with function
loadClassifierNM2, e.g. from file in samples/cpp/trained_classifierNM2.xml
@param  minProbability :   The minimum probability P(er|character) allowed for retreived ER's

In the second stage, the ERs that passed the first stage are classified into character and
non-character classes using more informative but also more computationally expensive features. The
classifier uses all the features calculated in the first stage and the following additional
features: hole area ratio, convex hull ratio, and number of outer inflexion points.
 */
CV_EXPORTS_W Ptr<ERFilter> createERFilterNM2(const Ptr<ERFilter::Callback>& cb,
                                                  float minProbability = (float)0.3);

/** @brief Reads an Extremal Region Filter for the 1st stage classifier of N&M algorithm
    from the provided path e.g. /path/to/cpp/trained_classifierNM1.xml

@overload
 */
CV_EXPORTS_W  Ptr<ERFilter> createERFilterNM1(const String& filename,
                                                  int thresholdDelta = 1, float minArea = (float)0.00025,
                                                  float maxArea = (float)0.13, float minProbability = (float)0.4,
                                                  bool nonMaxSuppression = true,
                                                  float minProbabilityDiff = (float)0.1);

/** @brief Reads an Extremal Region Filter for the 2nd stage classifier of N&M algorithm
    from the provided path e.g. /path/to/cpp/trained_classifierNM2.xml

@overload
 */
CV_EXPORTS_W Ptr<ERFilter> createERFilterNM2(const String& filename,
                                                  float minProbability = (float)0.3);

/** @brief Allow to implicitly load the default classifier when creating an ERFilter object.

@param filename The XML or YAML file with the classifier model (e.g. trained_classifierNM1.xml)

returns a pointer to ERFilter::Callback.
 */
CV_EXPORTS_W Ptr<ERFilter::Callback> loadClassifierNM1(const String& filename);

/** @brief Allow to implicitly load the default classifier when creating an ERFilter object.

@param filename The XML or YAML file with the classifier model (e.g. trained_classifierNM2.xml)

returns a pointer to ERFilter::Callback.
 */
CV_EXPORTS_W Ptr<ERFilter::Callback> loadClassifierNM2(const String& filename);


//! computeNMChannels operation modes
enum { ERFILTER_NM_RGBLGrad,
       ERFILTER_NM_IHSGrad
     };

/** @brief Compute the different channels to be processed independently in the N&M algorithm @cite Neumann12.

@param _src Source image. Must be RGB CV_8UC3.

@param _channels Output vector\<Mat\> where computed channels are stored.

@param _mode Mode of operation. Currently the only available options are:
**ERFILTER_NM_RGBLGrad** (used by default) and **ERFILTER_NM_IHSGrad**.

In N&M algorithm, the combination of intensity (I), hue (H), saturation (S), and gradient magnitude
channels (Grad) are used in order to obtain high localization recall. This implementation also
provides an alternative combination of red (R), green (G), blue (B), lightness (L), and gradient
magnitude (Grad).
 */
CV_EXPORTS_W void computeNMChannels(InputArray _src, CV_OUT OutputArrayOfArrays _channels, int _mode = ERFILTER_NM_RGBLGrad);



//! text::erGrouping operation modes
enum erGrouping_Modes {

    /** Exhaustive Search algorithm proposed in @cite Neumann11 for grouping horizontally aligned text.
    The algorithm models a verification function for all the possible ER sequences. The
    verification fuction for ER pairs consists in a set of threshold-based pairwise rules which
    compare measurements of two regions (height ratio, centroid angle, and region distance). The
    verification function for ER triplets creates a word text line estimate using Least
    Median-Squares fitting for a given triplet and then verifies that the estimate is valid (based
    on thresholds created during training). Verification functions for sequences larger than 3 are
    approximated by verifying that the text line parameters of all (sub)sequences of length 3 are
    consistent.
    */
    ERGROUPING_ORIENTATION_HORIZ,
    /** Text grouping method proposed in @cite Gomez13 @cite Gomez14 for grouping arbitrary oriented text. Regions
    are agglomerated by Single Linkage Clustering in a weighted feature space that combines proximity
    (x,y coordinates) and similarity measures (color, size, gradient magnitude, stroke width, etc.).
    SLC provides a dendrogram where each node represents a text group hypothesis. Then the algorithm
    finds the branches corresponding to text groups by traversing this dendrogram with a stopping rule
    that combines the output of a rotation invariant text group classifier and a probabilistic measure
    for hierarchical clustering validity assessment.

    @note This mode is not supported due NFA code removal ( https://github.com/opencv/opencv_contrib/issues/2235 )
     */
    ERGROUPING_ORIENTATION_ANY
};

/** @brief Find groups of Extremal Regions that are organized as text blocks.

@param img Original RGB or Greyscale image from wich the regions were extracted.

@param channels Vector of single channel images CV_8UC1 from wich the regions were extracted.

@param regions Vector of ER's retrieved from the ERFilter algorithm from each channel.

@param groups The output of the algorithm is stored in this parameter as set of lists of indexes to
provided regions.

@param groups_rects The output of the algorithm are stored in this parameter as list of rectangles.

@param method Grouping method (see text::erGrouping_Modes). Can be one of ERGROUPING_ORIENTATION_HORIZ,
ERGROUPING_ORIENTATION_ANY.

@param filename The XML or YAML file with the classifier model (e.g.
samples/trained_classifier_erGrouping.xml). Only to use when grouping method is
ERGROUPING_ORIENTATION_ANY.

@param minProbablity The minimum probability for accepting a group. Only to use when grouping
method is ERGROUPING_ORIENTATION_ANY.
 */
CV_EXPORTS void erGrouping(InputArray img, InputArrayOfArrays channels,
                                           std::vector<std::vector<ERStat> > &regions,
                                           std::vector<std::vector<Vec2i> > &groups,
                                           std::vector<Rect> &groups_rects,
                                           int method = ERGROUPING_ORIENTATION_HORIZ,
                                           const std::string& filename = std::string(),
                                           float minProbablity = 0.5);

CV_EXPORTS_W void erGrouping(InputArray image, InputArray channel,
                                           std::vector<std::vector<Point> > regions,
                                           CV_OUT std::vector<Rect> &groups_rects,
                                           int method = ERGROUPING_ORIENTATION_HORIZ,
                                           const String& filename = String(),
                                           float minProbablity = (float)0.5);

/** @brief Converts MSER contours (vector\<Point\>) to ERStat regions.

@param image Source image CV_8UC1 from which the MSERs where extracted.

@param contours Input vector with all the contours (vector\<Point\>).

@param regions Output where the ERStat regions are stored.

It takes as input the contours provided by the OpenCV MSER feature detector and returns as output
two vectors of ERStats. This is because MSER() output contains both MSER+ and MSER- regions in a
single vector\<Point\>, the function separates them in two different vectors (this is as if the
ERStats where extracted from two different channels).

An example of MSERsToERStats in use can be found in the text detection webcam_demo:
<https://github.com/opencv/opencv_contrib/blob/master/modules/text/samples/webcam_demo.cpp>
 */
CV_EXPORTS void MSERsToERStats(InputArray image, std::vector<std::vector<Point> > &contours,
                               std::vector<std::vector<ERStat> > &regions);

// Utility funtion for scripting
CV_EXPORTS_W void detectRegions(InputArray image, const Ptr<ERFilter>& er_filter1, const Ptr<ERFilter>& er_filter2, CV_OUT std::vector< std::vector<Point> >& regions);


/** @brief Extracts text regions from image.

@param image Source image where text blocks needs to be extracted from.  Should be CV_8UC3 (color).
@param er_filter1 Extremal Region Filter for the 1st stage classifier of N&M algorithm @cite Neumann12
@param er_filter2 Extremal Region Filter for the 2nd stage classifier of N&M algorithm @cite Neumann12
@param groups_rects Output list of rectangle blocks with text
@param method Grouping method (see text::erGrouping_Modes). Can be one of ERGROUPING_ORIENTATION_HORIZ, ERGROUPING_ORIENTATION_ANY.
@param filename The XML or YAML file with the classifier model (e.g. samples/trained_classifier_erGrouping.xml). Only to use when grouping method is ERGROUPING_ORIENTATION_ANY.
@param minProbability The minimum probability for accepting a group. Only to use when grouping method is ERGROUPING_ORIENTATION_ANY.


 */
CV_EXPORTS_W void detectRegions(InputArray image, const Ptr<ERFilter>& er_filter1, const Ptr<ERFilter>& er_filter2, CV_OUT std::vector<Rect> &groups_rects,
                                           int method = ERGROUPING_ORIENTATION_HORIZ,
                                           const String& filename = String(),
                                           float minProbability = (float)0.5);

//! @}

}
}
#endif // _OPENCV_TEXT_ERFILTER_HPP_
