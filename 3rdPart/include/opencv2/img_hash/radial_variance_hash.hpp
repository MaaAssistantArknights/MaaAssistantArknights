// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#ifndef OPENCV_RADIAL_VARIANCE_HASH_HPP
#define OPENCV_RADIAL_VARIANCE_HASH_HPP

#include "img_hash_base.hpp"

namespace cv {
namespace img_hash {

//! @addtogroup img_hash
//! @{


/** @brief Image hash based on Radon transform.

See @cite tang2012perceptual for details.
*/
class CV_EXPORTS_W RadialVarianceHash : public ImgHashBase
{
public:
    CV_WRAP static Ptr<RadialVarianceHash> create(double sigma = 1, int numOfAngleLine = 180);

    CV_WRAP int getNumOfAngleLine() const;
    CV_WRAP double getSigma() const;

    CV_WRAP void setNumOfAngleLine(int value);
    CV_WRAP void setSigma(double value);

    // internals
    std::vector<double> getFeatures();
    cv::Mat getHash();
    Mat getPixPerLine(Mat const &input);
    Mat getProjection();
protected:
    RadialVarianceHash() {}
};

/** @brief Computes radial variance hash of the input image
    @param inputArr input image want to compute hash value,
    type should be CV_8UC4, CV_8UC3, CV_8UC1.
    @param outputArr Hash value of input
    @param sigma Gaussian kernel standard deviation
    @param numOfAngleLine The number of angles to consider
     */
CV_EXPORTS_W void radialVarianceHash(cv::InputArray inputArr,
                                     cv::OutputArray outputArr,
                                     double sigma = 1,
                                     int numOfAngleLine = 180);


//! @}

}} // cv::img_hash::

#endif // OPENCV_RADIAL_VARIANCE_HASH_HPP
