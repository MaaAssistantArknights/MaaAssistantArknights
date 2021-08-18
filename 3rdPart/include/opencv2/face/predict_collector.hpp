/*
By downloading, copying, installing or using the software you agree to this license.
If you do not agree to this license, do not download, install,
copy or use the software.


                          License Agreement
               For Open Source Computer Vision Library
                       (3-clause BSD License)

Copyright (C) 2000-2015, Intel Corporation, all rights reserved.
Copyright (C) 2009-2011, Willow Garage Inc., all rights reserved.
Copyright (C) 2009-2015, NVIDIA Corporation, all rights reserved.
Copyright (C) 2010-2013, Advanced Micro Devices, Inc., all rights reserved.
Copyright (C) 2015, OpenCV Foundation, all rights reserved.
Copyright (C) 2015, Itseez Inc., all rights reserved.
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
warranties of merchantability and fitness for a particular purpose are disclaimed.
In no event shall copyright holders or contributors be liable for any direct,
indirect, incidental, special, exemplary, or consequential damages
(including, but not limited to, procurement of substitute goods or services;
loss of use, data, or profits; or business interruption) however caused
and on any theory of liability, whether in contract, strict liability,
or tort (including negligence or otherwise) arising in any way out of
the use of this software, even if advised of the possibility of such damage.
*/

#ifndef __OPENCV_PREDICT_COLLECTOR_HPP__
#define __OPENCV_PREDICT_COLLECTOR_HPP__

#include <vector>
#include <map>
#include <utility>
#include <cfloat>

#include "opencv2/core/base.hpp"

namespace cv {
namespace face {
//! @addtogroup face
//! @{
/** @brief Abstract base class for all strategies of prediction result handling
*/
class CV_EXPORTS_W PredictCollector
{
public:
    virtual ~PredictCollector() {}

    /** @brief Interface method called by face recognizer before results processing
    @param size total size of prediction evaluation that recognizer could perform
    */
    virtual void init(size_t size) { CV_UNUSED(size); }

    /** @brief Interface method called by face recognizer for each result
    @param label current prediction label
    @param dist current prediction distance (confidence)
    */
    virtual bool collect(int label, double dist) = 0;
};

/** @brief Default predict collector

Trace minimal distance with treshhold checking (that is default behavior for most predict logic)
*/
class CV_EXPORTS_W StandardCollector : public PredictCollector
{
public:
    struct PredictResult
    {
        int label;
        double distance;
        PredictResult(int label_ = -1, double distance_ = DBL_MAX) : label(label_), distance(distance_) {}
    };
protected:
    double threshold;
    PredictResult minRes;
    std::vector<PredictResult> data;
public:
    /** @brief Constructor
    @param threshold_ set threshold
    */
    StandardCollector(double threshold_ = DBL_MAX);
    /** @brief overloaded interface method */
    void init(size_t size) CV_OVERRIDE;
    /** @brief overloaded interface method */
    bool collect(int label, double dist) CV_OVERRIDE;
    /** @brief Returns label with minimal distance */
    CV_WRAP int getMinLabel() const;
    /** @brief Returns minimal distance value */
    CV_WRAP double getMinDist() const;
    /** @brief Return results as vector
    @param sorted If set, results will be sorted by distance
    Each values is a pair of label and distance.
    */
    CV_WRAP std::vector< std::pair<int, double> > getResults(bool sorted = false) const;
    /** @brief Return results as map
    Labels are keys, values are minimal distances
    */
    std::map<int, double> getResultsMap() const;
    /** @brief Static constructor
    @param threshold set threshold
    */
    CV_WRAP static Ptr<StandardCollector> create(double threshold = DBL_MAX);
};

//! @}
}
}

#endif
