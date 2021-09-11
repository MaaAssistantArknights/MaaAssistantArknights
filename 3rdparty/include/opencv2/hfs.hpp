// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#include "opencv2/core.hpp"

namespace cv { namespace hfs {

/** @defgroup hfs Hierarchical Feature Selection for Efficient Image Segmentation

The opencv hfs module contains an efficient algorithm to segment an image.
This module is implemented based on the paper Hierarchical Feature Selection for Efficient
Image Segmentation, ECCV 2016. The original project was developed by
Yun Liu(https://github.com/yun-liu/hfs).


Introduction to Hierarchical Feature Selection
----------------------------------------------


This algorithm is executed in 3 stages:

In the first stage, the algorithm uses SLIC (simple linear iterative clustering) algorithm
to obtain the superpixel of the input image.

In the second stage, the algorithm view each superpixel as a node in the graph.
It will calculate a feature vector for each edge of the graph. It then calculates a weight
for each edge based on the feature vector and trained SVM parameters. After obtaining
weight for each edge, it will exploit  EGB (Efficient Graph-based Image Segmentation)
algorithm to merge some nodes in the graph thus obtaining a coarser segmentation
After these operations, a post process will be executed to merge regions that are smaller
then a specific number of pixels into their nearby region.

In the third stage, the algorithm exploits the similar mechanism to further merge
the small regions obtained in the second stage into even coarser segmentation.

After these three stages, we can obtain the final segmentation of the image.
For further details about the algorithm, please refer to the original paper:
Hierarchical Feature Selection for Efficient Image Segmentation, ECCV 2016

*/

//! @addtogroup hfs
//! @{
class CV_EXPORTS_W HfsSegment : public Algorithm {
public:

/** @brief: set and get the parameter segEgbThresholdI.
* This parameter is used in the second stage mentioned above.
* It is a constant used to threshold weights of the edge when merging
* adjacent nodes when applying EGB algorithm. The segmentation result
* tends to have more regions remained if this value is large and vice versa.
*/
CV_WRAP virtual void setSegEgbThresholdI(float c) = 0;
CV_WRAP virtual float getSegEgbThresholdI() = 0;


/** @brief: set and get the parameter minRegionSizeI.
* This parameter is used in the second stage
* mentioned above. After the EGB segmentation, regions that have fewer
* pixels then this parameter will be merged into it's adjacent region.
*/
CV_WRAP virtual void setMinRegionSizeI(int n) = 0;
CV_WRAP virtual int getMinRegionSizeI() = 0;


/** @brief: set and get the parameter segEgbThresholdII.
* This parameter is used in the third stage
* mentioned above. It serves the same purpose as segEgbThresholdI.
* The segmentation result tends to have more regions remained if
* this value is large and vice versa.
*/
CV_WRAP virtual void setSegEgbThresholdII(float c) = 0;
CV_WRAP virtual float getSegEgbThresholdII() = 0;


/** @brief: set and get the parameter minRegionSizeII.
* This parameter is used in the third stage
* mentioned above. It serves the same purpose as minRegionSizeI
*/
CV_WRAP virtual void setMinRegionSizeII(int n) = 0;
CV_WRAP virtual int getMinRegionSizeII() = 0;


/** @brief: set and get the parameter spatialWeight.
* This parameter is used in the first stage
* mentioned above(the SLIC stage). It describes how important is the role
* of position when calculating the distance between each pixel and it's
* center. The exact formula to calculate the distance is
* \f$colorDistance + spatialWeight \times spatialDistance\f$.
* The segmentation result tends to have more local consistency
* if this value is larger.
*/
CV_WRAP virtual void setSpatialWeight(float w) = 0;
CV_WRAP virtual float getSpatialWeight() = 0;


/** @brief: set and get the parameter slicSpixelSize.
* This parameter is used in the first stage mentioned
* above(the SLIC stage). It describes the size of each
* superpixel when initializing SLIC. Every superpixel
* approximately has \f$slicSpixelSize \times slicSpixelSize\f$
* pixels in the beginning.
*/
CV_WRAP virtual void setSlicSpixelSize(int n) = 0;
CV_WRAP virtual int getSlicSpixelSize() = 0;


/** @brief: set and get the parameter numSlicIter.
* This parameter is used in the first stage. It
* describes how many iteration to perform when executing SLIC.
*/
CV_WRAP virtual void setNumSlicIter(int n) = 0;
CV_WRAP virtual int getNumSlicIter() = 0;

/** @brief do segmentation gpu
* @param src: the input image
* @param ifDraw: if draw the image in the returned Mat. if this parameter is false,
* then the content of the returned Mat is a matrix of index, describing the region
* each pixel belongs to. And it's data type is CV_16U. If this parameter is true,
* then the returned Mat is a segmented picture, and color of each region is the
* average color of all pixels in that region. And it's data type is the same as
* the input image
*/
CV_WRAP virtual Mat performSegmentGpu(InputArray src, bool ifDraw = true) = 0;

/** @brief do segmentation with cpu
* This method is only implemented for reference.
* It is highly NOT recommanded to use it.
*/
CV_WRAP virtual Mat performSegmentCpu(InputArray src, bool ifDraw = true) = 0;

/** @brief: create a hfs object
* @param height: the height of the input image
* @param width: the width of the input image
* @param segEgbThresholdI: parameter segEgbThresholdI
* @param minRegionSizeI: parameter minRegionSizeI
* @param segEgbThresholdII: parameter segEgbThresholdII
* @param minRegionSizeII: parameter minRegionSizeII
* @param spatialWeight: parameter spatialWeight
* @param slicSpixelSize: parameter slicSpixelSize
* @param numSlicIter: parameter numSlicIter
*/
CV_WRAP static Ptr<HfsSegment> create(int height, int width,
    float segEgbThresholdI = 0.08f, int minRegionSizeI = 100,
    float segEgbThresholdII = 0.28f, int minRegionSizeII = 200,
    float spatialWeight = 0.6f, int slicSpixelSize = 8, int numSlicIter = 5);

};

//! @}

}} // namespace cv { namespace hfs {
