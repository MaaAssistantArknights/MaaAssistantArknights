// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#ifndef _OPENCV_STEREO_DESCRIPTOR_HPP_
#define _OPENCV_STEREO_DESCRIPTOR_HPP_

namespace cv { namespace stereo {

// FIXIT deprecate and remove CV_ prefix
/// types of supported kernels
enum {
    CV_DENSE_CENSUS, CV_SPARSE_CENSUS,
    CV_CS_CENSUS, CV_MODIFIED_CS_CENSUS, CV_MODIFIED_CENSUS_TRANSFORM,
    CV_MEAN_VARIATION, CV_STAR_KERNEL
};

/**
Two variations of census applied on input images
Implementation of a census transform which is taking into account just the some pixels from the census kernel thus allowing for larger block sizes
**/
CV_EXPORTS void censusTransform(const Mat &image1, const Mat &image2, int kernelSize, Mat &dist1, Mat &dist2, const int type);
/// single image census transform
CV_EXPORTS void censusTransform(const Mat &image1, int kernelSize, Mat &dist1, const int type);
/**
STANDARD_MCT - Modified census which is memorizing for each pixel 2 bits and includes a tolerance to the pixel comparison
MCT_MEAN_VARIATION - Implementation of a modified census transform which is also taking into account the variation to the mean of the window not just the center pixel
**/
CV_EXPORTS void modifiedCensusTransform(const Mat &img1, const Mat &img2, int kernelSize, Mat &dist1, Mat &dist2, const int type, int t = 0, const Mat &integralImage1 = Mat(), const Mat &integralImage2 = Mat());
///single version of modified census transform descriptor
CV_EXPORTS void modifiedCensusTransform(const Mat &img1, int kernelSize, Mat &dist, const int type, int t = 0, const Mat &integralImage = Mat());
/**The classical center symetric census
A modified version of cs census which is comparing a pixel with its correspondent after the center
**/
CV_EXPORTS void symetricCensusTransform(const Mat &img1, const Mat &img2, int kernelSize, Mat &dist1, Mat &dist2, const int type);
///single version of census transform
CV_EXPORTS void symetricCensusTransform(const Mat &img1, int kernelSize, Mat &dist1, const int type);
///in a 9x9 kernel only certain positions are choosen
CV_EXPORTS void starCensusTransform(const Mat &img1, const Mat &img2, int kernelSize, Mat &dist1, Mat &dist2);
///single image version of star kernel
CV_EXPORTS void starCensusTransform(const Mat &img1, int kernelSize, Mat &dist);

}}  // namespace
#endif
