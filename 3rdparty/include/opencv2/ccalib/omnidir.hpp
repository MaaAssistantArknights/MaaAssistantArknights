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
// Copyright (C) 2015, Baisheng Lai (laibaisheng@gmail.com), Zhejiang University,
// all rights reserved.
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

#ifndef __OPENCV_OMNIDIR_HPP__
#define __OPENCV_OMNIDIR_HPP__

#include "opencv2/core.hpp"
#include "opencv2/core/affine.hpp"
#include <vector>

namespace cv
{
namespace omnidir
{
    //! @addtogroup ccalib
    //! @{

    enum {
        CALIB_USE_GUESS             = 1,
        CALIB_FIX_SKEW              = 2,
        CALIB_FIX_K1                = 4,
        CALIB_FIX_K2                = 8,
        CALIB_FIX_P1                = 16,
        CALIB_FIX_P2                = 32,
        CALIB_FIX_XI                = 64,
        CALIB_FIX_GAMMA             = 128,
        CALIB_FIX_CENTER            = 256
    };

    enum{
        RECTIFY_PERSPECTIVE         = 1,
        RECTIFY_CYLINDRICAL         = 2,
        RECTIFY_LONGLATI            = 3,
        RECTIFY_STEREOGRAPHIC       = 4
    };

    enum{
        XYZRGB  = 1,
        XYZ     = 2
    };
/**
 * This module was accepted as a GSoC 2015 project for OpenCV, authored by
 * Baisheng Lai, mentored by Bo Li.
 */

    /** @brief Projects points for omnidirectional camera using CMei's model

    @param objectPoints Object points in world coordinate, vector of vector of Vec3f or Mat of
    1xN/Nx1 3-channel of type CV_32F and N is the number of points. 64F is also acceptable.
    @param imagePoints Output array of image points, vector of vector of Vec2f or
    1xN/Nx1 2-channel of type CV_32F. 64F is also acceptable.
    @param rvec vector of rotation between world coordinate and camera coordinate, i.e., om
    @param tvec vector of translation between pattern coordinate and camera coordinate
    @param K Camera matrix \f$K = \vecthreethree{f_x}{s}{c_x}{0}{f_y}{c_y}{0}{0}{_1}\f$.
    @param D Input vector of distortion coefficients \f$(k_1, k_2, p_1, p_2)\f$.
    @param xi The parameter xi for CMei's model
    @param jacobian Optional output 2Nx16 of type CV_64F jacobian matrix, contains the derivatives of
    image pixel points wrt parameters including \f$om, T, f_x, f_y, s, c_x, c_y, xi, k_1, k_2, p_1, p_2\f$.
    This matrix will be used in calibration by optimization.

    The function projects object 3D points of world coordinate to image pixels, parameter by intrinsic
    and extrinsic parameters. Also, it optionally compute a by-product: the jacobian matrix containing
    contains the derivatives of image pixel points wrt intrinsic and extrinsic parameters.
     */
    CV_EXPORTS_W void projectPoints(InputArray objectPoints, OutputArray imagePoints, InputArray rvec, InputArray tvec,
                       InputArray K, double xi, InputArray D, OutputArray jacobian = noArray());

    /** @overload */
    CV_EXPORTS void projectPoints(InputArray objectPoints, OutputArray imagePoints, const Affine3d& affine,
                        InputArray K, double xi, InputArray D, OutputArray jacobian = noArray());

    /** @brief Undistort 2D image points for omnidirectional camera using CMei's model

    @param distorted Array of distorted image points, vector of Vec2f
    or 1xN/Nx1 2-channel Mat of type CV_32F, 64F depth is also acceptable
    @param K Camera matrix \f$K = \vecthreethree{f_x}{s}{c_x}{0}{f_y}{c_y}{0}{0}{_1}\f$.
    @param D Distortion coefficients \f$(k_1, k_2, p_1, p_2)\f$.
    @param xi The parameter xi for CMei's model
    @param R Rotation trainsform between the original and object space : 3x3 1-channel, or vector: 3x1/1x3
    1-channel or 1x1 3-channel
    @param undistorted array of normalized object points, vector of Vec2f/Vec2d or 1xN/Nx1 2-channel Mat with the same
    depth of distorted points.
     */
    CV_EXPORTS_W void undistortPoints(InputArray distorted, OutputArray undistorted, InputArray K, InputArray D, InputArray xi, InputArray R);

    /** @brief Computes undistortion and rectification maps for omnidirectional camera image transform by a rotation R.
    It output two maps that are used for cv::remap(). If D is empty then zero distortion is used,
    if R or P is empty then identity matrices are used.

    @param K Camera matrix \f$K = \vecthreethree{f_x}{s}{c_x}{0}{f_y}{c_y}{0}{0}{_1}\f$, with depth CV_32F or CV_64F
    @param D Input vector of distortion coefficients \f$(k_1, k_2, p_1, p_2)\f$, with depth CV_32F or CV_64F
    @param xi The parameter xi for CMei's model
    @param R Rotation transform between the original and object space : 3x3 1-channel, or vector: 3x1/1x3, with depth CV_32F or CV_64F
    @param P New camera matrix (3x3) or new projection matrix (3x4)
    @param size Undistorted image size.
    @param m1type Type of the first output map that can be CV_32FC1 or CV_16SC2 . See convertMaps()
    for details.
    @param map1 The first output map.
    @param map2 The second output map.
    @param flags Flags indicates the rectification type,  RECTIFY_PERSPECTIVE, RECTIFY_CYLINDRICAL, RECTIFY_LONGLATI and RECTIFY_STEREOGRAPHIC
    are supported.
     */
    CV_EXPORTS_W void initUndistortRectifyMap(InputArray K, InputArray D, InputArray xi, InputArray R, InputArray P, const cv::Size& size,
        int m1type, OutputArray map1, OutputArray map2, int flags);

    /** @brief Undistort omnidirectional images to perspective images

    @param distorted The input omnidirectional image.
    @param undistorted The output undistorted image.
    @param K Camera matrix \f$K = \vecthreethree{f_x}{s}{c_x}{0}{f_y}{c_y}{0}{0}{_1}\f$.
    @param D Input vector of distortion coefficients \f$(k_1, k_2, p_1, p_2)\f$.
    @param xi The parameter xi for CMei's model.
    @param flags Flags indicates the rectification type,  RECTIFY_PERSPECTIVE, RECTIFY_CYLINDRICAL, RECTIFY_LONGLATI and RECTIFY_STEREOGRAPHIC
    @param Knew Camera matrix of the distorted image. If it is not assigned, it is just K.
    @param new_size The new image size. By default, it is the size of distorted.
    @param R Rotation matrix between the input and output images. By default, it is identity matrix.
    */
    CV_EXPORTS_W void undistortImage(InputArray distorted, OutputArray undistorted, InputArray K, InputArray D, InputArray xi, int flags,
        InputArray Knew = cv::noArray(), const Size& new_size = Size(), InputArray R = Mat::eye(3, 3, CV_64F));

    /** @brief Perform omnidirectional camera calibration, the default depth of outputs is CV_64F.

    @param objectPoints Vector of vector of Vec3f object points in world (pattern) coordinate.
    It also can be vector of Mat with size 1xN/Nx1 and type CV_32FC3. Data with depth of 64_F is also acceptable.
    @param imagePoints Vector of vector of Vec2f corresponding image points of objectPoints. It must be the same
    size and the same type with objectPoints.
    @param size Image size of calibration images.
    @param K Output calibrated camera matrix.
    @param xi Output parameter xi for CMei's model
    @param D Output distortion parameters \f$(k_1, k_2, p_1, p_2)\f$
    @param rvecs Output rotations for each calibration images
    @param tvecs Output translation for each calibration images
    @param flags The flags that control calibrate
    @param criteria Termination criteria for optimization
    @param idx Indices of images that pass initialization, which are really used in calibration. So the size of rvecs is the
    same as idx.total().
    */
    CV_EXPORTS_W double calibrate(InputArrayOfArrays objectPoints, InputArrayOfArrays imagePoints, Size size,
        InputOutputArray K, InputOutputArray xi, InputOutputArray D, OutputArrayOfArrays rvecs, OutputArrayOfArrays tvecs,
        int flags, TermCriteria criteria, OutputArray idx=noArray());

    /** @brief Stereo calibration for omnidirectional camera model. It computes the intrinsic parameters for two
    cameras and the extrinsic parameters between two cameras. The default depth of outputs is CV_64F.

    @param objectPoints Object points in world (pattern) coordinate. Its type is vector<vector<Vec3f> >.
    It also can be vector of Mat with size 1xN/Nx1 and type CV_32FC3. Data with depth of 64_F is also acceptable.
    @param imagePoints1 The corresponding image points of the first camera, with type vector<vector<Vec2f> >.
    It must be the same size and the same type as objectPoints.
    @param imagePoints2 The corresponding image points of the second camera, with type vector<vector<Vec2f> >.
    It must be the same size and the same type as objectPoints.
    @param imageSize1 Image size of calibration images of the first camera.
    @param imageSize2 Image size of calibration images of the second camera.
    @param K1 Output camera matrix for the first camera.
    @param xi1 Output parameter xi of Mei's model for the first camera
    @param D1 Output distortion parameters \f$(k_1, k_2, p_1, p_2)\f$ for the first camera
    @param K2 Output camera matrix for the first camera.
    @param xi2 Output parameter xi of CMei's model for the second camera
    @param D2 Output distortion parameters \f$(k_1, k_2, p_1, p_2)\f$ for the second camera
    @param rvec Output rotation between the first and second camera
    @param tvec Output translation between the first and second camera
    @param rvecsL Output rotation for each image of the first camera
    @param tvecsL Output translation for each image of the first camera
    @param flags The flags that control stereoCalibrate
    @param criteria Termination criteria for optimization
    @param idx Indices of image pairs that pass initialization, which are really used in calibration. So the size of rvecs is the
    same as idx.total().
    @
    */
    CV_EXPORTS_W double stereoCalibrate(InputOutputArrayOfArrays objectPoints, InputOutputArrayOfArrays imagePoints1, InputOutputArrayOfArrays imagePoints2,
        const Size& imageSize1, const Size& imageSize2, InputOutputArray K1, InputOutputArray xi1, InputOutputArray D1, InputOutputArray K2, InputOutputArray xi2,
        InputOutputArray D2, OutputArray rvec, OutputArray tvec, OutputArrayOfArrays rvecsL, OutputArrayOfArrays tvecsL, int flags, TermCriteria criteria, OutputArray idx=noArray());

    /** @brief Stereo rectification for omnidirectional camera model. It computes the rectification rotations for two cameras

    @param R Rotation between the first and second camera
    @param T Translation between the first and second camera
    @param R1 Output 3x3 rotation matrix for the first camera
    @param R2 Output 3x3 rotation matrix for the second camera
    */
    CV_EXPORTS_W void stereoRectify(InputArray R, InputArray T, OutputArray R1, OutputArray R2);

    /** @brief Stereo 3D reconstruction from a pair of images

    @param image1 The first input image
    @param image2 The second input image
    @param K1 Input camera matrix of the first camera
    @param D1 Input distortion parameters \f$(k_1, k_2, p_1, p_2)\f$ for the first camera
    @param xi1 Input parameter xi for the first camera for CMei's model
    @param K2 Input camera matrix of the second camera
    @param D2 Input distortion parameters \f$(k_1, k_2, p_1, p_2)\f$ for the second camera
    @param xi2 Input parameter xi for the second camera for CMei's model
    @param R Rotation between the first and second camera
    @param T Translation between the first and second camera
    @param flag Flag of rectification type, RECTIFY_PERSPECTIVE or RECTIFY_LONGLATI
    @param numDisparities The parameter 'numDisparities' in StereoSGBM, see StereoSGBM for details.
    @param SADWindowSize The parameter 'SADWindowSize' in StereoSGBM, see StereoSGBM for details.
    @param disparity Disparity map generated by stereo matching
    @param image1Rec Rectified image of the first image
    @param image2Rec rectified image of the second image
    @param newSize Image size of rectified image, see omnidir::undistortImage
    @param Knew New camera matrix of rectified image, see omnidir::undistortImage
    @param pointCloud Point cloud of 3D reconstruction, with type CV_64FC3
    @param pointType Point cloud type, it can be XYZRGB or XYZ
    */
    CV_EXPORTS_W void stereoReconstruct(InputArray image1, InputArray image2, InputArray K1, InputArray D1, InputArray xi1,
        InputArray K2, InputArray D2, InputArray xi2, InputArray R, InputArray T, int flag, int numDisparities, int SADWindowSize,
        OutputArray disparity, OutputArray image1Rec, OutputArray image2Rec, const Size& newSize = Size(), InputArray Knew = cv::noArray(),
        OutputArray pointCloud = cv::noArray(), int pointType = XYZRGB);

namespace internal
{
    void initializeCalibration(InputArrayOfArrays objectPoints, InputArrayOfArrays imagePoints, Size size, OutputArrayOfArrays omAll,
        OutputArrayOfArrays tAll, OutputArray K, double& xi, OutputArray idx = noArray());

    void initializeStereoCalibration(InputArrayOfArrays objectPoints, InputArrayOfArrays imagePoints1, InputArrayOfArrays imagePoints2,
        const Size& size1, const Size& size2, OutputArray om, OutputArray T, OutputArrayOfArrays omL, OutputArrayOfArrays tL, OutputArray K1, OutputArray D1, OutputArray K2, OutputArray D2,
        double &xi1, double &xi2, int flags, OutputArray idx);

    void computeJacobian(InputArrayOfArrays objectPoints, InputArrayOfArrays imagePoints, InputArray parameters, Mat& JTJ_inv, Mat& JTE, int flags,
							double epsilon);

    void computeJacobianStereo(InputArrayOfArrays objectPoints, InputArrayOfArrays imagePoints1, InputArrayOfArrays imagePoints2,
        InputArray parameters, Mat& JTJ_inv, Mat& JTE, int flags, double epsilon);

    void encodeParameters(InputArray K, InputArrayOfArrays omAll, InputArrayOfArrays tAll, InputArray distoaration, double xi, OutputArray parameters);

    void encodeParametersStereo(InputArray K1, InputArray K2, InputArray om, InputArray T, InputArrayOfArrays omL, InputArrayOfArrays tL,
        InputArray D1, InputArray D2, double xi1, double xi2, OutputArray parameters);

    void decodeParameters(InputArray paramsters, OutputArray K, OutputArrayOfArrays omAll, OutputArrayOfArrays tAll, OutputArray distoration, double& xi);

    void decodeParametersStereo(InputArray parameters, OutputArray K1, OutputArray K2, OutputArray om, OutputArray T, OutputArrayOfArrays omL,
        OutputArrayOfArrays tL, OutputArray D1, OutputArray D2, double& xi1, double& xi2);

    void estimateUncertainties(InputArrayOfArrays objectPoints, InputArrayOfArrays imagePoints, InputArray parameters, Mat& errors, Vec2d& std_error, double& rms, int flags);

    void estimateUncertaintiesStereo(InputArrayOfArrays objectPoints, InputArrayOfArrays imagePoints1, InputArrayOfArrays imagePoints2, InputArray parameters, Mat& errors,
        Vec2d& std_error, double& rms, int flags);

    double computeMeanReproErr(InputArrayOfArrays imagePoints, InputArrayOfArrays proImagePoints);

    double computeMeanReproErr(InputArrayOfArrays objectPoints, InputArrayOfArrays imagePoints, InputArray K, InputArray D, double xi, InputArrayOfArrays omAll,
        InputArrayOfArrays tAll);

    double computeMeanReproErrStereo(InputArrayOfArrays objectPoints, InputArrayOfArrays imagePoints1, InputArrayOfArrays imagePoints2, InputArray K1, InputArray K2,
        InputArray D1, InputArray D2, double xi1, double xi2, InputArray om, InputArray T, InputArrayOfArrays omL, InputArrayOfArrays TL);

    void subMatrix(const Mat& src, Mat& dst, const std::vector<int>& cols, const std::vector<int>& rows);

    void flags2idx(int flags, std::vector<int>& idx, int n);

    void flags2idxStereo(int flags, std::vector<int>& idx, int n);

    void fillFixed(Mat&G, int flags, int n);

    void fillFixedStereo(Mat& G, int flags, int n);

    double findMedian(const Mat& row);

    Vec3d findMedian3(InputArray mat);

    void getInterset(InputArray idx1, InputArray idx2, OutputArray inter1, OutputArray inter2, OutputArray inter_ori);

    void compose_motion(InputArray _om1, InputArray _T1, InputArray _om2, InputArray _T2, Mat& om3, Mat& T3, Mat& dom3dom1,
        Mat& dom3dT1, Mat& dom3dom2, Mat& dom3dT2, Mat& dT3dom1, Mat& dT3dT1, Mat& dT3dom2, Mat& dT3dT2);

    //void JRodriguesMatlab(const Mat& src, Mat& dst);

    //void dAB(InputArray A, InputArray B, OutputArray dABdA, OutputArray dABdB);
} // internal

//! @}

} // omnidir

} //cv
#endif
