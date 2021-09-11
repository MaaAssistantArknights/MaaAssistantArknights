/*
By downloading, copying, installing or using the software you agree to this
license. If you do not agree to this license, do not download, install,
copy or use the software.

                          License Agreement
               For Open Source Computer Vision Library
                       (3-clause BSD License)

Copyright (C) 2013, OpenCV Foundation, all rights reserved.
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
warranties of merchantability and fitness for a particular purpose are
disclaimed. In no event shall copyright holders or contributors be liable for
any direct, indirect, incidental, special, exemplary, or consequential damages
(including, but not limited to, procurement of substitute goods or services;
loss of use, data, or profits; or business interruption) however caused
and on any theory of liability, whether in contract, strict liability,
or tort (including negligence or otherwise) arising in any way out of
the use of this software, even if advised of the possibility of such damage.
*/

#ifndef __OPENCV_CHARUCO_HPP__
#define __OPENCV_CHARUCO_HPP__

#include <opencv2/core.hpp>
#include <vector>
#include <opencv2/aruco.hpp>


namespace cv {
namespace aruco {

//! @addtogroup aruco
//! @{


/**
 * @brief ChArUco board
 * Specific class for ChArUco boards. A ChArUco board is a planar board where the markers are placed
 * inside the white squares of a chessboard. The benefits of ChArUco boards is that they provide
 * both, ArUco markers versatility and chessboard corner precision, which is important for
 * calibration and pose estimation.
 * This class also allows the easy creation and drawing of ChArUco boards.
 */
class CV_EXPORTS_W CharucoBoard : public Board {

    public:
    // vector of chessboard 3D corners precalculated
    CV_PROP std::vector< Point3f > chessboardCorners;

    // for each charuco corner, nearest marker id and nearest marker corner id of each marker
    CV_PROP std::vector< std::vector< int > > nearestMarkerIdx;
    CV_PROP std::vector< std::vector< int > > nearestMarkerCorners;

    /**
     * @brief Draw a ChArUco board
     *
     * @param outSize size of the output image in pixels.
     * @param img output image with the board. The size of this image will be outSize
     * and the board will be on the center, keeping the board proportions.
     * @param marginSize minimum margins (in pixels) of the board in the output image
     * @param borderBits width of the marker borders.
     *
     * This function return the image of the ChArUco board, ready to be printed.
     */
    CV_WRAP void draw(Size outSize, OutputArray img, int marginSize = 0, int borderBits = 1);


    /**
     * @brief Create a CharucoBoard object
     *
     * @param squaresX number of chessboard squares in X direction
     * @param squaresY number of chessboard squares in Y direction
     * @param squareLength chessboard square side length (normally in meters)
     * @param markerLength marker side length (same unit than squareLength)
     * @param dictionary dictionary of markers indicating the type of markers.
     * The first markers in the dictionary are used to fill the white chessboard squares.
     * @return the output CharucoBoard object
     *
     * This functions creates a CharucoBoard object given the number of squares in each direction
     * and the size of the markers and chessboard squares.
     */
    CV_WRAP static Ptr<CharucoBoard> create(int squaresX, int squaresY, float squareLength,
                                            float markerLength, const Ptr<Dictionary> &dictionary);

    /**
      *
      */
    CV_WRAP Size getChessboardSize() const { return Size(_squaresX, _squaresY); }

    /**
      *
      */
    CV_WRAP float getSquareLength() const { return _squareLength; }

    /**
      *
      */
    CV_WRAP float getMarkerLength() const { return _markerLength; }

    private:
    void _getNearestMarkerCorners();

    // number of markers in X and Y directions
    int _squaresX, _squaresY;

    // size of chessboard squares side (normally in meters)
    float _squareLength;

    // marker side length (normally in meters)
    float _markerLength;
};




/**
 * @brief Interpolate position of ChArUco board corners
 * @param markerCorners vector of already detected markers corners. For each marker, its four
 * corners are provided, (e.g std::vector<std::vector<cv::Point2f> > ). For N detected markers, the
 * dimensions of this array should be Nx4. The order of the corners should be clockwise.
 * @param markerIds list of identifiers for each marker in corners
 * @param image input image necesary for corner refinement. Note that markers are not detected and
 * should be sent in corners and ids parameters.
 * @param board layout of ChArUco board.
 * @param charucoCorners interpolated chessboard corners
 * @param charucoIds interpolated chessboard corners identifiers
 * @param cameraMatrix optional 3x3 floating-point camera matrix
 * \f$A = \vecthreethree{f_x}{0}{c_x}{0}{f_y}{c_y}{0}{0}{1}\f$
 * @param distCoeffs optional vector of distortion coefficients
 * \f$(k_1, k_2, p_1, p_2[, k_3[, k_4, k_5, k_6],[s_1, s_2, s_3, s_4]])\f$ of 4, 5, 8 or 12 elements
 * @param minMarkers number of adjacent markers that must be detected to return a charuco corner
 *
 * This function receives the detected markers and returns the 2D position of the chessboard corners
 * from a ChArUco board using the detected Aruco markers. If camera parameters are provided,
 * the process is based in an approximated pose estimation, else it is based on local homography.
 * Only visible corners are returned. For each corner, its corresponding identifier is
 * also returned in charucoIds.
 * The function returns the number of interpolated corners.
 */
CV_EXPORTS_W int interpolateCornersCharuco(InputArrayOfArrays markerCorners, InputArray markerIds,
                                           InputArray image, const Ptr<CharucoBoard> &board,
                                           OutputArray charucoCorners, OutputArray charucoIds,
                                           InputArray cameraMatrix = noArray(),
                                           InputArray distCoeffs = noArray(), int minMarkers = 2);




/**
 * @brief Pose estimation for a ChArUco board given some of their corners
 * @param charucoCorners vector of detected charuco corners
 * @param charucoIds list of identifiers for each corner in charucoCorners
 * @param board layout of ChArUco board.
 * @param cameraMatrix input 3x3 floating-point camera matrix
 * \f$A = \vecthreethree{f_x}{0}{c_x}{0}{f_y}{c_y}{0}{0}{1}\f$
 * @param distCoeffs vector of distortion coefficients
 * \f$(k_1, k_2, p_1, p_2[, k_3[, k_4, k_5, k_6],[s_1, s_2, s_3, s_4]])\f$ of 4, 5, 8 or 12 elements
 * @param rvec Output vector (e.g. cv::Mat) corresponding to the rotation vector of the board
 * (see cv::Rodrigues).
 * @param tvec Output vector (e.g. cv::Mat) corresponding to the translation vector of the board.
 * @param useExtrinsicGuess defines whether initial guess for \b rvec and \b tvec will be used or not.
 *
 * This function estimates a Charuco board pose from some detected corners.
 * The function checks if the input corners are enough and valid to perform pose estimation.
 * If pose estimation is valid, returns true, else returns false.
 */
CV_EXPORTS_W bool estimatePoseCharucoBoard(InputArray charucoCorners, InputArray charucoIds,
                                           const Ptr<CharucoBoard> &board, InputArray cameraMatrix,
                                           InputArray distCoeffs, OutputArray rvec, OutputArray tvec,
                                           bool useExtrinsicGuess = false);




/**
 * @brief Draws a set of Charuco corners
 * @param image input/output image. It must have 1 or 3 channels. The number of channels is not
 * altered.
 * @param charucoCorners vector of detected charuco corners
 * @param charucoIds list of identifiers for each corner in charucoCorners
 * @param cornerColor color of the square surrounding each corner
 *
 * This function draws a set of detected Charuco corners. If identifiers vector is provided, it also
 * draws the id of each corner.
 */
CV_EXPORTS_W void drawDetectedCornersCharuco(InputOutputArray image, InputArray charucoCorners,
                                             InputArray charucoIds = noArray(),
                                             Scalar cornerColor = Scalar(255, 0, 0));



/**
 * @brief Calibrate a camera using Charuco corners
 *
 * @param charucoCorners vector of detected charuco corners per frame
 * @param charucoIds list of identifiers for each corner in charucoCorners per frame
 * @param board Marker Board layout
 * @param imageSize input image size
 * @param cameraMatrix Output 3x3 floating-point camera matrix
 * \f$A = \vecthreethree{f_x}{0}{c_x}{0}{f_y}{c_y}{0}{0}{1}\f$ . If CV\_CALIB\_USE\_INTRINSIC\_GUESS
 * and/or CV_CALIB_FIX_ASPECT_RATIO are specified, some or all of fx, fy, cx, cy must be
 * initialized before calling the function.
 * @param distCoeffs Output vector of distortion coefficients
 * \f$(k_1, k_2, p_1, p_2[, k_3[, k_4, k_5, k_6],[s_1, s_2, s_3, s_4]])\f$ of 4, 5, 8 or 12 elements
 * @param rvecs Output vector of rotation vectors (see Rodrigues ) estimated for each board view
 * (e.g. std::vector<cv::Mat>>). That is, each k-th rotation vector together with the corresponding
 * k-th translation vector (see the next output parameter description) brings the board pattern
 * from the model coordinate space (in which object points are specified) to the world coordinate
 * space, that is, a real position of the board pattern in the k-th pattern view (k=0.. *M* -1).
 * @param tvecs Output vector of translation vectors estimated for each pattern view.
 * @param stdDeviationsIntrinsics Output vector of standard deviations estimated for intrinsic parameters.
 * Order of deviations values:
 * \f$(f_x, f_y, c_x, c_y, k_1, k_2, p_1, p_2, k_3, k_4, k_5, k_6 , s_1, s_2, s_3,
 * s_4, \tau_x, \tau_y)\f$ If one of parameters is not estimated, it's deviation is equals to zero.
 * @param stdDeviationsExtrinsics Output vector of standard deviations estimated for extrinsic parameters.
 * Order of deviations values: \f$(R_1, T_1, \dotsc , R_M, T_M)\f$ where M is number of pattern views,
 * \f$R_i, T_i\f$ are concatenated 1x3 vectors.
 * @param perViewErrors Output vector of average re-projection errors estimated for each pattern view.
 * @param flags flags Different flags  for the calibration process (see #calibrateCamera for details).
 * @param criteria Termination criteria for the iterative optimization algorithm.
 *
 * This function calibrates a camera using a set of corners of a  Charuco Board. The function
 * receives a list of detected corners and its identifiers from several views of the Board.
 * The function returns the final re-projection error.
 */
CV_EXPORTS_AS(calibrateCameraCharucoExtended) double calibrateCameraCharuco(
    InputArrayOfArrays charucoCorners, InputArrayOfArrays charucoIds, const Ptr<CharucoBoard> &board,
    Size imageSize, InputOutputArray cameraMatrix, InputOutputArray distCoeffs,
    OutputArrayOfArrays rvecs, OutputArrayOfArrays tvecs,
    OutputArray stdDeviationsIntrinsics, OutputArray stdDeviationsExtrinsics,
    OutputArray perViewErrors, int flags = 0,
    TermCriteria criteria = TermCriteria(TermCriteria::COUNT + TermCriteria::EPS, 30, DBL_EPSILON));

/** @brief It's the same function as #calibrateCameraCharuco but without calibration error estimation.
*/
CV_EXPORTS_W double calibrateCameraCharuco(
  InputArrayOfArrays charucoCorners, InputArrayOfArrays charucoIds, const Ptr<CharucoBoard> &board,
  Size imageSize, InputOutputArray cameraMatrix, InputOutputArray distCoeffs,
  OutputArrayOfArrays rvecs = noArray(), OutputArrayOfArrays tvecs = noArray(), int flags = 0,
  TermCriteria criteria = TermCriteria(TermCriteria::COUNT + TermCriteria::EPS, 30, DBL_EPSILON));



/**
 * @brief Detect ChArUco Diamond markers
 *
 * @param image input image necessary for corner subpixel.
 * @param markerCorners list of detected marker corners from detectMarkers function.
 * @param markerIds list of marker ids in markerCorners.
 * @param squareMarkerLengthRate rate between square and marker length:
 * squareMarkerLengthRate = squareLength/markerLength. The real units are not necessary.
 * @param diamondCorners output list of detected diamond corners (4 corners per diamond). The order
 * is the same than in marker corners: top left, top right, bottom right and bottom left. Similar
 * format than the corners returned by detectMarkers (e.g std::vector<std::vector<cv::Point2f> > ).
 * @param diamondIds ids of the diamonds in diamondCorners. The id of each diamond is in fact of
 * type Vec4i, so each diamond has 4 ids, which are the ids of the aruco markers composing the
 * diamond.
 * @param cameraMatrix Optional camera calibration matrix.
 * @param distCoeffs Optional camera distortion coefficients.
 *
 * This function detects Diamond markers from the previous detected ArUco markers. The diamonds
 * are returned in the diamondCorners and diamondIds parameters. If camera calibration parameters
 * are provided, the diamond search is based on reprojection. If not, diamond search is based on
 * homography. Homography is faster than reprojection but can slightly reduce the detection rate.
 */
CV_EXPORTS_W void detectCharucoDiamond(InputArray image, InputArrayOfArrays markerCorners,
                                       InputArray markerIds, float squareMarkerLengthRate,
                                       OutputArrayOfArrays diamondCorners, OutputArray diamondIds,
                                       InputArray cameraMatrix = noArray(),
                                       InputArray distCoeffs = noArray());



/**
 * @brief Draw a set of detected ChArUco Diamond markers
 *
 * @param image input/output image. It must have 1 or 3 channels. The number of channels is not
 * altered.
 * @param diamondCorners positions of diamond corners in the same format returned by
 * detectCharucoDiamond(). (e.g std::vector<std::vector<cv::Point2f> > ). For N detected markers,
 * the dimensions of this array should be Nx4. The order of the corners should be clockwise.
 * @param diamondIds vector of identifiers for diamonds in diamondCorners, in the same format
 * returned by detectCharucoDiamond() (e.g. std::vector<Vec4i>).
 * Optional, if not provided, ids are not painted.
 * @param borderColor color of marker borders. Rest of colors (text color and first corner color)
 * are calculated based on this one.
 *
 * Given an array of detected diamonds, this functions draws them in the image. The marker borders
 * are painted and the markers identifiers if provided.
 * Useful for debugging purposes.
 */
CV_EXPORTS_W void drawDetectedDiamonds(InputOutputArray image, InputArrayOfArrays diamondCorners,
                                       InputArray diamondIds = noArray(),
                                       Scalar borderColor = Scalar(0, 0, 255));




/**
 * @brief Draw a ChArUco Diamond marker
 *
 * @param dictionary dictionary of markers indicating the type of markers.
 * @param ids list of 4 ids for each ArUco marker in the ChArUco marker.
 * @param squareLength size of the chessboard squares in pixels.
 * @param markerLength size of the markers in pixels.
 * @param img output image with the marker. The size of this image will be
 * 3*squareLength + 2*marginSize,.
 * @param marginSize minimum margins (in pixels) of the marker in the output image
 * @param borderBits width of the marker borders.
 *
 * This function return the image of a ChArUco marker, ready to be printed.
 */
// TODO cannot be exported yet; conversion from/to Vec4i is not wrapped in core
CV_EXPORTS void drawCharucoDiamond(const Ptr<Dictionary> &dictionary, Vec4i ids, int squareLength,
                                   int markerLength, OutputArray img, int marginSize = 0,
                                   int borderBits = 1);


/**
 * @brief test whether the ChArUco markers are collinear
 *
 * @param _board layout of ChArUco board.
 * @param _charucoIds list of identifiers for each corner in charucoCorners per frame.
 * @return bool value, 1 (true) if detected corners form a line, 0 (false) if they do not.
      solvePnP, calibration functions will fail if the corners are collinear (true).
 *
 * The number of ids in charucoIDs should be <= the number of chessboard corners in the board.  This functions checks whether the charuco corners are on a straight line (returns true, if so), or not (false).  Axis parallel, as well as diagonal and other straight lines detected.  Degenerate cases: for number of charucoIDs <= 2, the function returns true.
 */
CV_EXPORTS_W bool testCharucoCornersCollinear(const Ptr<CharucoBoard> &_board,
                                              InputArray _charucoIds);

//! @}
}
}

#endif
