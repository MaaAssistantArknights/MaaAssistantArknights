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
 // Copyright (C) 2014, OpenCV Foundation, all rights reserved.
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

#ifndef __OPENCV_CCALIB_HPP__
#define __OPENCV_CCALIB_HPP__

#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>

#include <vector>

/** @defgroup ccalib Custom Calibration Pattern for 3D reconstruction
*/

namespace cv{ namespace ccalib{

//! @addtogroup ccalib
//! @{

class CV_EXPORTS CustomPattern : public Algorithm
{
public:
	CustomPattern();
	virtual ~CustomPattern();

	bool create(InputArray pattern, const Size2f boardSize, OutputArray output = noArray());

	bool findPattern(InputArray image, OutputArray matched_features, OutputArray pattern_points, const double ratio = 0.7,
					 const double proj_error = 8.0, const bool refine_position = false, OutputArray out = noArray(),
					 OutputArray H = noArray(), OutputArray pattern_corners = noArray());

	bool isInitialized();

	void getPatternPoints(std::vector<KeyPoint>& original_points);
    /**<
		Returns a vector<Point> of the original points.
	*/
	double getPixelSize();
    /**<
		Get the pixel size of the pattern
	*/

	bool setFeatureDetector(Ptr<FeatureDetector> featureDetector);
	bool setDescriptorExtractor(Ptr<DescriptorExtractor> extractor);
	bool setDescriptorMatcher(Ptr<DescriptorMatcher> matcher);

	Ptr<FeatureDetector> getFeatureDetector();
	Ptr<DescriptorExtractor> getDescriptorExtractor();
	Ptr<DescriptorMatcher> getDescriptorMatcher();

	double calibrate(InputArrayOfArrays objectPoints, InputArrayOfArrays imagePoints,
				Size imageSize, InputOutputArray cameraMatrix, InputOutputArray distCoeffs,
				OutputArrayOfArrays rvecs, OutputArrayOfArrays tvecs, int flags = 0,
				TermCriteria criteria = TermCriteria(TermCriteria::COUNT + TermCriteria::EPS, 30, DBL_EPSILON));
    /**<
		Calls the calirateCamera function with the same inputs.
	*/

	bool findRt(InputArray objectPoints, InputArray imagePoints, InputArray cameraMatrix, InputArray distCoeffs,
                OutputArray rvec, OutputArray tvec, bool useExtrinsicGuess = false, int flags = SOLVEPNP_ITERATIVE);
	bool findRt(InputArray image, InputArray cameraMatrix, InputArray distCoeffs,
                OutputArray rvec, OutputArray tvec, bool useExtrinsicGuess = false, int flags = SOLVEPNP_ITERATIVE);
    /**<
		Uses solvePnP to find the rotation and translation of the pattern
		with respect to the camera frame.
	*/

	bool findRtRANSAC(InputArray objectPoints, InputArray imagePoints, InputArray cameraMatrix, InputArray distCoeffs,
				OutputArray rvec, OutputArray tvec, bool useExtrinsicGuess = false, int iterationsCount = 100,
				float reprojectionError = 8.0, int minInliersCount = 100, OutputArray inliers = noArray(), int flags = SOLVEPNP_ITERATIVE);
	bool findRtRANSAC(InputArray image, InputArray cameraMatrix, InputArray distCoeffs,
				OutputArray rvec, OutputArray tvec, bool useExtrinsicGuess = false, int iterationsCount = 100,
				float reprojectionError = 8.0, int minInliersCount = 100, OutputArray inliers = noArray(), int flags = SOLVEPNP_ITERATIVE);
        /**<
		Uses solvePnPRansac()
	*/

	void drawOrientation(InputOutputArray image, InputArray tvec, InputArray rvec, InputArray cameraMatrix,
						 InputArray distCoeffs, double axis_length = 3, int axis_width = 2);
    /**<
		pattern_corners -> projected over the image position of the edges of the pattern.
	*/

private:

	Mat img_roi;
	std::vector<Point2f> obj_corners;
	double pxSize;

	bool initialized;

	Ptr<FeatureDetector> detector;
	Ptr<DescriptorExtractor> descriptorExtractor;
	Ptr<DescriptorMatcher> descriptorMatcher;

	std::vector<KeyPoint> keypoints;
	std::vector<Point3f> points3d;
	Mat descriptor;

	bool init(Mat& image, const float pixel_size, OutputArray output = noArray());
	bool findPatternPass(const Mat& image, std::vector<Point2f>& matched_features, std::vector<Point3f>& pattern_points,
						 Mat& H, std::vector<Point2f>& scene_corners, const double pratio, const double proj_error,
						 const bool refine_position = false, const Mat& mask = Mat(), OutputArray output = noArray());
	void scaleFoundPoints(const double squareSize, const std::vector<KeyPoint>& corners, std::vector<Point3f>& pts3d);
	void check_matches(std::vector<Point2f>& matched, const std::vector<Point2f>& pattern, std::vector<DMatch>& good, std::vector<Point3f>& pattern_3d, const Mat& H);

	void keypoints2points(const std::vector<KeyPoint>& in, std::vector<Point2f>& out);
	void updateKeypointsPos(std::vector<KeyPoint>& in, const std::vector<Point2f>& new_pos);
	void refinePointsPos(const Mat& img, std::vector<Point2f>& p);
	void refineKeypointsPos(const Mat& img, std::vector<KeyPoint>& kp);
};

//! @}

}} // namespace ccalib, cv

#endif
