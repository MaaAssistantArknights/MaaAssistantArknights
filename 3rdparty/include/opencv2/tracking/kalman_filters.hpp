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

#ifndef __OPENCV_TRACKING_KALMAN_HPP_
#define __OPENCV_TRACKING_KALMAN_HPP_

#include "opencv2/core.hpp"
#include <limits>

namespace cv
{
namespace tracking
{

/** @brief The interface for Unscented Kalman filter and Augmented Unscented Kalman filter.
*/
class CV_EXPORTS UnscentedKalmanFilter
{
public:

    virtual ~UnscentedKalmanFilter(){}

    /** The function performs prediction step of the algorithm
    * @param control - the current control vector,
    * @return the predicted estimate of the state.
    */
    virtual Mat predict( InputArray control = noArray() ) = 0;

    /** The function performs correction step of the algorithm
    * @param measurement - the current measurement vector,
    * @return the corrected estimate of the state.
    */
    virtual Mat correct( InputArray measurement ) = 0;

    /**
    * @return the process noise cross-covariance matrix.
    */
    virtual Mat getProcessNoiseCov() const = 0;

    /**
    * @return the measurement noise cross-covariance matrix.
    */
    virtual Mat getMeasurementNoiseCov() const = 0;

    /**
    * @return the error cross-covariance matrix.
    */
    virtual Mat getErrorCov() const = 0;

    /**
    * @return the current estimate of the state.
    */
    virtual Mat getState() const = 0;
};

/** @brief Model of dynamical system for Unscented Kalman filter.
* The interface for dynamical system model. It contains functions for computing the next state and the measurement.
* It must be inherited for using UKF.
*/
class CV_EXPORTS UkfSystemModel
{
public:

    virtual ~UkfSystemModel(){}

    /** The function for computing the next state from the previous state
    * @param x_k - previous state vector,
    * @param u_k - control vector,
    * @param v_k - noise vector,
    * @param x_kplus1 - next state vector.
    */
    virtual void stateConversionFunction( const Mat& x_k, const Mat& u_k, const Mat& v_k, Mat& x_kplus1 ) = 0;
    /** The function for computing the measurement from the state
    * @param x_k - state vector,
    * @param n_k - noise vector,
    * @param z_k - measurement vector.
    */
    virtual void measurementFunction( const Mat& x_k, const Mat& n_k, Mat& z_k ) = 0;
};


/** @brief Unscented Kalman filter parameters.
* The class for initialization parameters of Unscented Kalman filter
*/
class CV_EXPORTS UnscentedKalmanFilterParams
{
public:

     int DP;                                     //!< Dimensionality of the state vector.
     int MP;                                     //!< Dimensionality of the measurement vector.
     int CP;                                     //!< Dimensionality of the control vector.
     int dataType;                               //!< Type of elements of vectors and matrices, default is CV_64F.

     Mat stateInit;                              //!< Initial state, DP x 1, default is zero.
     Mat errorCovInit;                           //!< State estimate cross-covariance matrix, DP x DP, default is identity.

     Mat processNoiseCov;                        //!< Process noise cross-covariance matrix, DP x DP.
     Mat measurementNoiseCov;                    //!< Measurement noise cross-covariance matrix, MP x MP.

     // Parameters of algorithm
     double alpha;                               //!< Default is 1e-3.
     double k;                                   //!< Default is 0.
     double beta;                                //!< Default is 2.0.

     //Dynamical system model
     Ptr<UkfSystemModel> model;                  //!< Object of the class containing functions for computing the next state and the measurement.

    /** The constructors.
    */
    UnscentedKalmanFilterParams(){}

    /**
    * @param dp - dimensionality of the state vector,
    * @param mp - dimensionality of the measurement vector,
    * @param cp - dimensionality of the control vector,
    * @param processNoiseCovDiag - value of elements on main diagonal process noise cross-covariance matrix,
    * @param measurementNoiseCovDiag - value of elements on main diagonal measurement noise cross-covariance matrix,
    * @param dynamicalSystem - ptr to object of the class containing functions for computing the next state and the measurement,
    * @param type - type of the created matrices that should be CV_32F or CV_64F.
    */
    UnscentedKalmanFilterParams( int dp, int mp, int cp, double processNoiseCovDiag, double measurementNoiseCovDiag,
                                Ptr<UkfSystemModel> dynamicalSystem, int type = CV_64F );

    /** The function for initialization of Unscented Kalman filter
    * @param dp - dimensionality of the state vector,
    * @param mp - dimensionality of the measurement vector,
    * @param cp - dimensionality of the control vector,
    * @param processNoiseCovDiag - value of elements on main diagonal process noise cross-covariance matrix,
    * @param measurementNoiseCovDiag - value of elements on main diagonal measurement noise cross-covariance matrix,
    * @param dynamicalSystem - ptr to object of the class containing functions for computing the next state and the measurement,
    * @param type - type of the created matrices that should be CV_32F or CV_64F.
    */
    void init( int dp, int mp, int cp, double processNoiseCovDiag, double measurementNoiseCovDiag,
                                Ptr<UkfSystemModel> dynamicalSystem, int type = CV_64F );
};

/** @brief Augmented Unscented Kalman filter parameters.
* The class for initialization parameters of Augmented Unscented Kalman filter
*/
class CV_EXPORTS AugmentedUnscentedKalmanFilterParams: public UnscentedKalmanFilterParams
{
public:

    AugmentedUnscentedKalmanFilterParams(){}

    /**
    * @param dp - dimensionality of the state vector,
    * @param mp - dimensionality of the measurement vector,
    * @param cp - dimensionality of the control vector,
    * @param processNoiseCovDiag - value of elements on main diagonal process noise cross-covariance matrix,
    * @param measurementNoiseCovDiag - value of elements on main diagonal measurement noise cross-covariance matrix,
    * @param dynamicalSystem - ptr to object of the class containing functions for computing the next state and the measurement,
    * @param type - type of the created matrices that should be CV_32F or CV_64F.
    */
    AugmentedUnscentedKalmanFilterParams( int dp, int mp, int cp, double processNoiseCovDiag, double measurementNoiseCovDiag,
                                Ptr<UkfSystemModel> dynamicalSystem, int type = CV_64F );

    /** The function for initialization of Augmented Unscented Kalman filter
    * @param dp - dimensionality of the state vector,
    * @param mp - dimensionality of the measurement vector,
    * @param cp - dimensionality of the control vector,
    * @param processNoiseCovDiag - value of elements on main diagonal process noise cross-covariance matrix,
    * @param measurementNoiseCovDiag - value of elements on main diagonal measurement noise cross-covariance matrix,
    * @param dynamicalSystem - object of the class containing functions for computing the next state and the measurement,
    * @param type - type of the created matrices that should be CV_32F or CV_64F.
    */
    void init( int dp, int mp, int cp, double processNoiseCovDiag, double measurementNoiseCovDiag,
                                Ptr<UkfSystemModel> dynamicalSystem, int type = CV_64F );
};

/** @brief Unscented Kalman Filter factory method

* The class implements an Unscented Kalman filter <https://en.wikipedia.org/wiki/Kalman_filter#Unscented_Kalman_filter>.
* @param params - an object of the UnscentedKalmanFilterParams class containing UKF parameters.
* @return pointer to the object of the UnscentedKalmanFilterImpl class implementing UnscentedKalmanFilter.
*/
CV_EXPORTS Ptr<UnscentedKalmanFilter> createUnscentedKalmanFilter( const UnscentedKalmanFilterParams &params );
/** @brief Augmented Unscented Kalman Filter factory method

* The class implements an Augmented Unscented Kalman filter http://becs.aalto.fi/en/research/bayes/ekfukf/documentation.pdf, page 31-33.
* AUKF is more accurate than UKF but its computational complexity is larger.
* @param params - an object of the AugmentedUnscentedKalmanFilterParams class containing AUKF parameters.
* @return pointer to the object of the AugmentedUnscentedKalmanFilterImpl class implementing UnscentedKalmanFilter.
*/
CV_EXPORTS Ptr<UnscentedKalmanFilter> createAugmentedUnscentedKalmanFilter( const AugmentedUnscentedKalmanFilterParams &params );

} // tracking
} // cv

#endif
