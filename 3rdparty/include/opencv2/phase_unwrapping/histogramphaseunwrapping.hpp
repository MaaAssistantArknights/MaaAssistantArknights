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
 // Copyright (C) 2015, OpenCV Foundation, all rights reserved.
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

#ifndef __OPENCV_HISTOGRAM_PHASE_UNWRAPPING_HPP__
#define __OPENCV_HISTOGRAM_PHASE_UNWRAPPING_HPP__

#include "opencv2/core.hpp"
#include <opencv2/imgproc.hpp>
#include "opencv2/phase_unwrapping/phase_unwrapping.hpp"

namespace cv {
namespace phase_unwrapping {
//! @addtogroup phase_unwrapping
//! @{

    /** @brief Class implementing two-dimensional phase unwrapping based on @cite histogramUnwrapping
     * This algorithm belongs to the quality-guided phase unwrapping methods.
     * First, it computes a reliability map from second differences between a pixel and its eight neighbours.
     * Reliability values lie between 0 and 16*pi*pi. Then, this reliability map is used to compute
     * the reliabilities of "edges". An edge is an entity defined by two pixels that are connected
     * horizontally or vertically. Its reliability is found by adding the the reliabilities of the
     * two pixels connected through it. Edges are sorted in a histogram based on their reliability values.
     * This histogram is then used to unwrap pixels, starting from the highest quality pixel.

     * The wrapped phase map and the unwrapped result are stored in CV_32FC1 Mat.
     */
class CV_EXPORTS_W HistogramPhaseUnwrapping : public PhaseUnwrapping
{

public:
    /**
     * @brief Parameters of phaseUnwrapping constructor.

     * @param width Phase map width.
     * @param height Phase map height.
     * @param histThresh Bins in the histogram are not of equal size. Default value is 3*pi*pi. The one before "histThresh" value are smaller.
     * @param nbrOfSmallBins Number of bins between 0 and "histThresh". Default value is 10.
     * @param nbrOfLargeBins Number of bins between "histThresh" and 32*pi*pi (highest edge reliability value). Default value is 5.
     */
    struct CV_EXPORTS_W_SIMPLE Params
    {
        CV_WRAP Params();
        CV_PROP_RW int width;
        CV_PROP_RW int height;
        CV_PROP_RW float histThresh;
        CV_PROP_RW int nbrOfSmallBins;
        CV_PROP_RW int nbrOfLargeBins;
    };
    /**
     * @brief Constructor

     * @param parameters HistogramPhaseUnwrapping parameters HistogramPhaseUnwrapping::Params: width,height of the phase map and histogram characteristics.
     */
    CV_WRAP
    static Ptr<HistogramPhaseUnwrapping> create( const HistogramPhaseUnwrapping::Params &parameters =
                                                 HistogramPhaseUnwrapping::Params() );

    /**
     * @brief Get the reliability map computed from the wrapped phase map.

     * @param reliabilityMap Image where the reliability map is stored.
     */
    CV_WRAP
    virtual void getInverseReliabilityMap( OutputArray reliabilityMap ) = 0;
};

//! @}
}
}
#endif