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
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009-2011, Willow Garage Inc., all rights reserved.
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

#ifndef __OPENCV_EDGEBOXES_HPP__
#define __OPENCV_EDGEBOXES_HPP__

#include <opencv2/core.hpp>

namespace cv
{
namespace ximgproc
{

//! @addtogroup ximgproc_edgeboxes
//! @{

// bounding box data structures
typedef struct
{
  int x, y, w, h;
  float score;
} Box;

typedef std::vector<Box> Boxes;

/** @brief Class implementing EdgeBoxes algorithm from @cite ZitnickECCV14edgeBoxes :
 */
class CV_EXPORTS_W EdgeBoxes : public Algorithm
{

public:

    /** @brief Returns array containing proposal boxes.

    @param edge_map edge image.
    @param orientation_map orientation map.
    @param boxes proposal boxes.
    @param scores of the proposal boxes, provided a vector of float types.
    */
    CV_WRAP virtual void getBoundingBoxes(InputArray edge_map, InputArray orientation_map, CV_OUT std::vector<Rect> &boxes, OutputArray scores = noArray()) = 0;

    /** @brief Returns the step size of sliding window search.
    */
    CV_WRAP virtual float getAlpha() const = 0;
    /** @brief Sets the step size of sliding window search.
    */
    CV_WRAP virtual void setAlpha(float value) = 0;

    /** @brief Returns the nms threshold for object proposals.
    */
    CV_WRAP virtual float getBeta() const = 0;
    /** @brief Sets the nms threshold for object proposals.
    */
    CV_WRAP virtual void setBeta(float value) = 0;

    /** @brief Returns adaptation rate for nms threshold.
    */
    CV_WRAP virtual float getEta() const = 0;
    /** @brief Sets the adaptation rate for nms threshold.
    */
    CV_WRAP virtual void setEta(float value) = 0;

    /** @brief Returns the min score of boxes to detect.
    */
    CV_WRAP virtual float getMinScore() const = 0;
    /** @brief Sets the min score of boxes to detect.
    */
    CV_WRAP virtual void setMinScore(float value) = 0;

    /** @brief Returns the max number of boxes to detect.
    */
    CV_WRAP virtual int getMaxBoxes() const = 0;
    /** @brief Sets max number of boxes to detect.
    */
    CV_WRAP virtual void setMaxBoxes(int value) = 0;

    /** @brief Returns the edge min magnitude.
    */
    CV_WRAP virtual float getEdgeMinMag() const = 0;
    /** @brief Sets the edge min magnitude.
    */
    CV_WRAP virtual void setEdgeMinMag(float value) = 0;

    /** @brief Returns the edge merge threshold.
    */
    CV_WRAP virtual float getEdgeMergeThr() const = 0;
    /** @brief Sets the edge merge threshold.
    */
    CV_WRAP virtual void setEdgeMergeThr(float value) = 0;

    /** @brief Returns the cluster min magnitude.
    */
    CV_WRAP virtual float getClusterMinMag() const = 0;
    /** @brief Sets the cluster min magnitude.
    */
    CV_WRAP virtual void setClusterMinMag(float value) = 0;

    /** @brief Returns the max aspect ratio of boxes.
    */
    CV_WRAP virtual float getMaxAspectRatio() const = 0;
    /** @brief Sets the max aspect ratio of boxes.
    */
    CV_WRAP virtual void setMaxAspectRatio(float value) = 0;

    /** @brief Returns the minimum area of boxes.
    */
    CV_WRAP virtual float getMinBoxArea() const = 0;
    /** @brief Sets the minimum area of boxes.
    */
    CV_WRAP virtual void setMinBoxArea(float value) = 0;

    /** @brief Returns the affinity sensitivity.
    */
    CV_WRAP virtual float getGamma() const = 0;
    /** @brief Sets the affinity sensitivity
    */
    CV_WRAP virtual void setGamma(float value) = 0;

    /** @brief Returns the scale sensitivity.
    */
    CV_WRAP virtual float getKappa() const = 0;
    /** @brief Sets the scale sensitivity.
    */
    CV_WRAP virtual void setKappa(float value) = 0;

};

/** @brief Creates a Edgeboxes

@param alpha step size of sliding window search.
@param beta nms threshold for object proposals.
@param eta adaptation rate for nms threshold.
@param minScore min score of boxes to detect.
@param maxBoxes max number of boxes to detect.
@param edgeMinMag edge min magnitude. Increase to trade off accuracy for speed.
@param edgeMergeThr edge merge threshold. Increase to trade off accuracy for speed.
@param clusterMinMag cluster min magnitude. Increase to trade off accuracy for speed.
@param maxAspectRatio max aspect ratio of boxes.
@param minBoxArea minimum area of boxes.
@param gamma affinity sensitivity.
@param kappa scale sensitivity.
*/
CV_EXPORTS_W Ptr<EdgeBoxes>
createEdgeBoxes(float alpha=0.65f,
                float beta=0.75f,
                float eta=1,
                float minScore=0.01f,
                int   maxBoxes=10000,
                float edgeMinMag=0.1f,
                float edgeMergeThr=0.5f,
                float clusterMinMag=0.5f,
                float maxAspectRatio=3,
                float minBoxArea=1000,
                float gamma=2,
                float kappa=1.5f);

//! @}

}
}

#endif /* __OPENCV_EDGEBOXES_HPP__ */
