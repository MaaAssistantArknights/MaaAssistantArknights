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
 // Copyright (C) 2013, OpenCV Foundation, all rights reserved.
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

#ifndef __OPENCV_TRACKER_HPP__
#define __OPENCV_TRACKER_HPP__

#include "opencv2/core.hpp"
#include "opencv2/imgproc/types_c.h"
#include "feature.hpp"
#include "onlineMIL.hpp"
#include "onlineBoosting.hpp"

/*
 * Partially based on:
 * ====================================================================================================================
 *   - [AAM] S. Salti, A. Cavallaro, L. Di Stefano, Adaptive Appearance Modeling for Video Tracking: Survey and Evaluation
 *  - [AMVOT] X. Li, W. Hu, C. Shen, Z. Zhang, A. Dick, A. van den Hengel, A Survey of Appearance Models in Visual Object Tracking
 *
 * This Tracking API has been designed with PlantUML. If you modify this API please change UML files under modules/tracking/doc/uml
 *
 */

namespace cv
{

//! @addtogroup tracking
//! @{

/************************************ TrackerFeature Base Classes ************************************/

/** @brief Abstract base class for TrackerFeature that represents the feature.
 */
class CV_EXPORTS TrackerFeature
{
 public:
  virtual ~TrackerFeature();

  /** @brief Compute the features in the images collection
    @param images The images
    @param response The output response
     */
  void compute( const std::vector<Mat>& images, Mat& response );

  /** @brief Create TrackerFeature by tracker feature type
    @param trackerFeatureType The TrackerFeature name

    The modes available now:

    -   "HAAR" -- Haar Feature-based

    The modes that will be available soon:

    -   "HOG" -- Histogram of Oriented Gradients features
    -   "LBP" -- Local Binary Pattern features
    -   "FEATURE2D" -- All types of Feature2D
     */
  static Ptr<TrackerFeature> create( const String& trackerFeatureType );

  /** @brief Identify most effective features
    @param response Collection of response for the specific TrackerFeature
    @param npoints Max number of features

    @note This method modifies the response parameter
     */
  virtual void selection( Mat& response, int npoints ) = 0;

  /** @brief Get the name of the specific TrackerFeature
     */
  String getClassName() const;

 protected:

  virtual bool computeImpl( const std::vector<Mat>& images, Mat& response ) = 0;

  String className;
};

/** @brief Class that manages the extraction and selection of features

@cite AAM Feature Extraction and Feature Set Refinement (Feature Processing and Feature Selection).
See table I and section III C @cite AMVOT Appearance modelling -\> Visual representation (Table II,
section 3.1 - 3.2)

TrackerFeatureSet is an aggregation of TrackerFeature

@sa
   TrackerFeature

 */
class CV_EXPORTS TrackerFeatureSet
{
 public:

  TrackerFeatureSet();

  ~TrackerFeatureSet();

  /** @brief Extract features from the images collection
    @param images The input images
     */
  void extraction( const std::vector<Mat>& images );

  /** @brief Identify most effective features for all feature types (optional)
     */
  void selection();

  /** @brief Remove outliers for all feature types (optional)
     */
  void removeOutliers();

  /** @brief Add TrackerFeature in the collection. Return true if TrackerFeature is added, false otherwise
    @param trackerFeatureType The TrackerFeature name

    The modes available now:

    -   "HAAR" -- Haar Feature-based

    The modes that will be available soon:

    -   "HOG" -- Histogram of Oriented Gradients features
    -   "LBP" -- Local Binary Pattern features
    -   "FEATURE2D" -- All types of Feature2D

    Example TrackerFeatureSet::addTrackerFeature : :
    @code
        //sample usage:

        Ptr<TrackerFeature> trackerFeature = new TrackerFeatureHAAR( HAARparameters );
        featureSet->addTrackerFeature( trackerFeature );

        //or add CSC sampler with default parameters
        //featureSet->addTrackerFeature( "HAAR" );
    @endcode
    @note If you use the second method, you must initialize the TrackerFeature
     */
  bool addTrackerFeature( String trackerFeatureType );

  /** @overload
    @param feature The TrackerFeature class
    */
  bool addTrackerFeature( Ptr<TrackerFeature>& feature );

  /** @brief Get the TrackerFeature collection (TrackerFeature name, TrackerFeature pointer)
     */
  const std::vector<std::pair<String, Ptr<TrackerFeature> > >& getTrackerFeature() const;

  /** @brief Get the responses

    @note Be sure to call extraction before getResponses Example TrackerFeatureSet::getResponses : :
     */
  const std::vector<Mat>& getResponses() const;

 private:

  void clearResponses();
  bool blockAddTrackerFeature;

  std::vector<std::pair<String, Ptr<TrackerFeature> > > features;  //list of features
  std::vector<Mat> responses;        //list of response after compute

};

/************************************ TrackerSampler Base Classes ************************************/

/** @brief Abstract base class for TrackerSamplerAlgorithm that represents the algorithm for the specific
sampler.
 */
class CV_EXPORTS TrackerSamplerAlgorithm
{
 public:
  /**
   * \brief Destructor
   */
  virtual ~TrackerSamplerAlgorithm();

  /** @brief Create TrackerSamplerAlgorithm by tracker sampler type.
    @param trackerSamplerType The trackerSamplerType name

    The modes available now:

    -   "CSC" -- Current State Center
    -   "CS" -- Current State
     */
  static Ptr<TrackerSamplerAlgorithm> create( const String& trackerSamplerType );

  /** @brief Computes the regions starting from a position in an image.

    Return true if samples are computed, false otherwise

    @param image The current frame
    @param boundingBox The bounding box from which regions can be calculated

    @param sample The computed samples @cite AAM Fig. 1 variable Sk
     */
  bool sampling( const Mat& image, Rect boundingBox, std::vector<Mat>& sample );

  /** @brief Get the name of the specific TrackerSamplerAlgorithm
    */
  String getClassName() const;

 protected:
  String className;

  virtual bool samplingImpl( const Mat& image, Rect boundingBox, std::vector<Mat>& sample ) = 0;
};

/**
 * \brief Class that manages the sampler in order to select regions for the update the model of the tracker
 * [AAM] Sampling e Labeling. See table I and section III B
 */

/** @brief Class that manages the sampler in order to select regions for the update the model of the tracker

@cite AAM Sampling e Labeling. See table I and section III B

TrackerSampler is an aggregation of TrackerSamplerAlgorithm
@sa
   TrackerSamplerAlgorithm
 */
class CV_EXPORTS TrackerSampler
{
 public:

  /**
   * \brief Constructor
   */
  TrackerSampler();

  /**
   * \brief Destructor
   */
  ~TrackerSampler();

  /** @brief Computes the regions starting from a position in an image
    @param image The current frame
    @param boundingBox The bounding box from which regions can be calculated
     */
  void sampling( const Mat& image, Rect boundingBox );

  /** @brief Return the collection of the TrackerSamplerAlgorithm
    */
  const std::vector<std::pair<String, Ptr<TrackerSamplerAlgorithm> > >& getSamplers() const;

  /** @brief Return the samples from all TrackerSamplerAlgorithm, @cite AAM Fig. 1 variable Sk
    */
  const std::vector<Mat>& getSamples() const;

  /** @brief Add TrackerSamplerAlgorithm in the collection. Return true if sampler is added, false otherwise
    @param trackerSamplerAlgorithmType The TrackerSamplerAlgorithm name

    The modes available now:
    -   "CSC" -- Current State Center
    -   "CS" -- Current State
    -   "PF" -- Particle Filtering

    Example TrackerSamplerAlgorithm::addTrackerSamplerAlgorithm : :
    @code
         TrackerSamplerCSC::Params CSCparameters;
         Ptr<TrackerSamplerAlgorithm> CSCSampler = new TrackerSamplerCSC( CSCparameters );

         if( !sampler->addTrackerSamplerAlgorithm( CSCSampler ) )
           return false;

         //or add CSC sampler with default parameters
         //sampler->addTrackerSamplerAlgorithm( "CSC" );
    @endcode
    @note If you use the second method, you must initialize the TrackerSamplerAlgorithm
     */
  bool addTrackerSamplerAlgorithm( String trackerSamplerAlgorithmType );

  /** @overload
    @param sampler The TrackerSamplerAlgorithm
    */
  bool addTrackerSamplerAlgorithm( Ptr<TrackerSamplerAlgorithm>& sampler );

 private:
  std::vector<std::pair<String, Ptr<TrackerSamplerAlgorithm> > > samplers;
  std::vector<Mat> samples;
  bool blockAddTrackerSampler;

  void clearSamples();
};

/************************************ TrackerModel Base Classes ************************************/

/** @brief Abstract base class for TrackerTargetState that represents a possible state of the target.

See @cite AAM \f$\hat{x}^{i}_{k}\f$ all the states candidates.

Inherits this class with your Target state, In own implementation you can add scale variation,
width, height, orientation, etc.
 */
class CV_EXPORTS TrackerTargetState
{
 public:
  virtual ~TrackerTargetState()
  {
  }
  ;
  /**
   * \brief Get the position
   * \return The position
   */
  Point2f getTargetPosition() const;

  /**
   * \brief Set the position
   * \param position The position
   */
  void setTargetPosition( const Point2f& position );
  /**
   * \brief Get the width of the target
   * \return The width of the target
   */
  int getTargetWidth() const;

  /**
   * \brief Set the width of the target
   * \param width The width of the target
   */
  void setTargetWidth( int width );
  /**
   * \brief Get the height of the target
   * \return The height of the target
   */
  int getTargetHeight() const;

  /**
   * \brief Set the height of the target
   * \param height The height of the target
   */
  void setTargetHeight( int height );

 protected:
  Point2f targetPosition;
  int targetWidth;
  int targetHeight;

};

/** @brief Represents the model of the target at frame \f$k\f$ (all states and scores)

See @cite AAM The set of the pair \f$\langle \hat{x}^{i}_{k}, C^{i}_{k} \rangle\f$
@sa TrackerTargetState
 */
typedef std::vector<std::pair<Ptr<TrackerTargetState>, float> > ConfidenceMap;

/** @brief Represents the estimate states for all frames

@cite AAM \f$x_{k}\f$ is the trajectory of the target up to time \f$k\f$

@sa TrackerTargetState
 */
typedef std::vector<Ptr<TrackerTargetState> > Trajectory;

/** @brief Abstract base class for TrackerStateEstimator that estimates the most likely target state.

See @cite AAM State estimator

See @cite AMVOT Statistical modeling (Fig. 3), Table III (generative) - IV (discriminative) - V (hybrid)
 */
class CV_EXPORTS TrackerStateEstimator
{
 public:
  virtual ~TrackerStateEstimator();

  /** @brief Estimate the most likely target state, return the estimated state
    @param confidenceMaps The overall appearance model as a list of :cConfidenceMap
     */
  Ptr<TrackerTargetState> estimate( const std::vector<ConfidenceMap>& confidenceMaps );

  /** @brief Update the ConfidenceMap with the scores
    @param confidenceMaps The overall appearance model as a list of :cConfidenceMap
     */
  void update( std::vector<ConfidenceMap>& confidenceMaps );

  /** @brief Create TrackerStateEstimator by tracker state estimator type
    @param trackeStateEstimatorType The TrackerStateEstimator name

    The modes available now:

    -   "BOOSTING" -- Boosting-based discriminative appearance models. See @cite AMVOT section 4.4

    The modes available soon:

    -   "SVM" -- SVM-based discriminative appearance models. See @cite AMVOT section 4.5
     */
  static Ptr<TrackerStateEstimator> create( const String& trackeStateEstimatorType );

  /** @brief Get the name of the specific TrackerStateEstimator
     */
  String getClassName() const;

 protected:

  virtual Ptr<TrackerTargetState> estimateImpl( const std::vector<ConfidenceMap>& confidenceMaps ) = 0;
  virtual void updateImpl( std::vector<ConfidenceMap>& confidenceMaps ) = 0;
  String className;
};

/** @brief Abstract class that represents the model of the target. It must be instantiated by specialized
tracker

See @cite AAM Ak

Inherits this with your TrackerModel
 */
class CV_EXPORTS TrackerModel
{
 public:

  /**
   * \brief Constructor
   */
  TrackerModel();

  /**
   * \brief Destructor
   */
  virtual ~TrackerModel();

  /** @brief Set TrackerEstimator, return true if the tracker state estimator is added, false otherwise
    @param trackerStateEstimator The TrackerStateEstimator
    @note You can add only one TrackerStateEstimator
     */
  bool setTrackerStateEstimator( Ptr<TrackerStateEstimator> trackerStateEstimator );

  /** @brief Estimate the most likely target location

    @cite AAM ME, Model Estimation table I
    @param responses Features extracted from TrackerFeatureSet
     */
  void modelEstimation( const std::vector<Mat>& responses );

  /** @brief Update the model

    @cite AAM MU, Model Update table I
     */
  void modelUpdate();

  /** @brief Run the TrackerStateEstimator, return true if is possible to estimate a new state, false otherwise
    */
  bool runStateEstimator();

  /** @brief Set the current TrackerTargetState in the Trajectory
    @param lastTargetState The current TrackerTargetState
     */
  void setLastTargetState( const Ptr<TrackerTargetState>& lastTargetState );

  /** @brief Get the last TrackerTargetState from Trajectory
    */
  Ptr<TrackerTargetState> getLastTargetState() const;

  /** @brief Get the list of the ConfidenceMap
    */
  const std::vector<ConfidenceMap>& getConfidenceMaps() const;

  /** @brief Get the last ConfidenceMap for the current frame
     */
  const ConfidenceMap& getLastConfidenceMap() const;

  /** @brief Get the TrackerStateEstimator
    */
  Ptr<TrackerStateEstimator> getTrackerStateEstimator() const;

 private:

  void clearCurrentConfidenceMap();

 protected:
  std::vector<ConfidenceMap> confidenceMaps;
  Ptr<TrackerStateEstimator> stateEstimator;
  ConfidenceMap currentConfidenceMap;
  Trajectory trajectory;
  int maxCMLength;

  virtual void modelEstimationImpl( const std::vector<Mat>& responses ) = 0;
  virtual void modelUpdateImpl() = 0;

};

/************************************ Tracker Base Class ************************************/

/** @brief Base abstract class for the long-term tracker:
 */
class CV_EXPORTS_W Tracker : public virtual Algorithm
{
 public:

  virtual ~Tracker() CV_OVERRIDE;

  /** @brief Initialize the tracker with a known bounding box that surrounded the target
    @param image The initial frame
    @param boundingBox The initial bounding box

    @return True if initialization went succesfully, false otherwise
     */
  CV_WRAP bool init( InputArray image, const Rect2d& boundingBox );

  /** @brief Update the tracker, find the new most likely bounding box for the target
    @param image The current frame
    @param boundingBox The bounding box that represent the new target location, if true was returned, not
    modified otherwise

    @return True means that target was located and false means that tracker cannot locate target in
    current frame. Note, that latter *does not* imply that tracker has failed, maybe target is indeed
    missing from the frame (say, out of sight)
     */
  CV_WRAP bool update( InputArray image, CV_OUT Rect2d& boundingBox );

  virtual void read( const FileNode& fn ) CV_OVERRIDE = 0;
  virtual void write( FileStorage& fs ) const CV_OVERRIDE = 0;

 protected:

  virtual bool initImpl( const Mat& image, const Rect2d& boundingBox ) = 0;
  virtual bool updateImpl( const Mat& image, Rect2d& boundingBox ) = 0;

  bool isInit;

  Ptr<TrackerFeatureSet> featureSet;
  Ptr<TrackerSampler> sampler;
  Ptr<TrackerModel> model;
};


/************************************ Specific TrackerStateEstimator Classes ************************************/

/** @brief TrackerStateEstimator based on Boosting
    */
class CV_EXPORTS TrackerStateEstimatorMILBoosting : public TrackerStateEstimator
{
 public:

  /**
   * Implementation of the target state for TrackerStateEstimatorMILBoosting
   */
  class TrackerMILTargetState : public TrackerTargetState
  {

   public:
    /**
     * \brief Constructor
     * \param position Top left corner of the bounding box
     * \param width Width of the bounding box
     * \param height Height of the bounding box
     * \param foreground label for target or background
     * \param features features extracted
     */
    TrackerMILTargetState( const Point2f& position, int width, int height, bool foreground, const Mat& features );

    /**
     * \brief Destructor
     */
    ~TrackerMILTargetState()
    {
    }
    ;

    /** @brief Set label: true for target foreground, false for background
    @param foreground Label for background/foreground
     */
    void setTargetFg( bool foreground );
    /** @brief Set the features extracted from TrackerFeatureSet
    @param features The features extracted
     */
    void setFeatures( const Mat& features );
    /** @brief Get the label. Return true for target foreground, false for background
     */
    bool isTargetFg() const;
    /** @brief Get the features extracted
     */
    Mat getFeatures() const;

   private:
    bool isTarget;
    Mat targetFeatures;
  };

  /** @brief Constructor
    @param nFeatures Number of features for each sample
     */
  TrackerStateEstimatorMILBoosting( int nFeatures = 250 );
  ~TrackerStateEstimatorMILBoosting();

  /** @brief Set the current confidenceMap
    @param confidenceMap The current :cConfidenceMap
     */
  void setCurrentConfidenceMap( ConfidenceMap& confidenceMap );

 protected:
  Ptr<TrackerTargetState> estimateImpl( const std::vector<ConfidenceMap>& confidenceMaps ) CV_OVERRIDE;
  void updateImpl( std::vector<ConfidenceMap>& confidenceMaps ) CV_OVERRIDE;

 private:
  uint max_idx( const std::vector<float> &v );
  void prepareData( const ConfidenceMap& confidenceMap, Mat& positive, Mat& negative );

  ClfMilBoost boostMILModel;
  bool trained;
  int numFeatures;

  ConfidenceMap currentConfidenceMap;
};

/** @brief TrackerStateEstimatorAdaBoosting based on ADA-Boosting
 */
class CV_EXPORTS TrackerStateEstimatorAdaBoosting : public TrackerStateEstimator
{
 public:
  /** @brief Implementation of the target state for TrackerAdaBoostingTargetState
    */
  class TrackerAdaBoostingTargetState : public TrackerTargetState
  {

   public:
    /**
     * \brief Constructor
     * \param position Top left corner of the bounding box
     * \param width Width of the bounding box
     * \param height Height of the bounding box
     * \param foreground label for target or background
     * \param responses list of features
     */
    TrackerAdaBoostingTargetState( const Point2f& position, int width, int height, bool foreground, const Mat& responses );

    /**
     * \brief Destructor
     */
    ~TrackerAdaBoostingTargetState()
    {
    }
    ;

    /** @brief Set the features extracted from TrackerFeatureSet
    @param responses The features extracted
     */
    void setTargetResponses( const Mat& responses );
    /** @brief Set label: true for target foreground, false for background
    @param foreground Label for background/foreground
     */
    void setTargetFg( bool foreground );
    /** @brief Get the features extracted
     */
    Mat getTargetResponses() const;
    /** @brief Get the label. Return true for target foreground, false for background
    */
    bool isTargetFg() const;

   private:
    bool isTarget;
    Mat targetResponses;

  };

  /** @brief Constructor
    @param numClassifer Number of base classifiers
    @param initIterations Number of iterations in the initialization
    @param nFeatures Number of features/weak classifiers
    @param patchSize tracking rect
    @param ROI initial ROI
     */
  TrackerStateEstimatorAdaBoosting( int numClassifer, int initIterations, int nFeatures, Size patchSize, const Rect& ROI );

  /**
   * \brief Destructor
   */
  ~TrackerStateEstimatorAdaBoosting();

  /** @brief Get the sampling ROI
     */
  Rect getSampleROI() const;

  /** @brief Set the sampling ROI
    @param ROI the sampling ROI
     */
  void setSampleROI( const Rect& ROI );

  /** @brief Set the current confidenceMap
    @param confidenceMap The current :cConfidenceMap
     */
  void setCurrentConfidenceMap( ConfidenceMap& confidenceMap );

  /** @brief Get the list of the selected weak classifiers for the classification step
     */
  std::vector<int> computeSelectedWeakClassifier();

  /** @brief Get the list of the weak classifiers that should be replaced
     */
  std::vector<int> computeReplacedClassifier();

  /** @brief Get the list of the weak classifiers that replace those to be replaced
     */
  std::vector<int> computeSwappedClassifier();

 protected:
  Ptr<TrackerTargetState> estimateImpl( const std::vector<ConfidenceMap>& confidenceMaps ) CV_OVERRIDE;
  void updateImpl( std::vector<ConfidenceMap>& confidenceMaps ) CV_OVERRIDE;

  Ptr<StrongClassifierDirectSelection> boostClassifier;

 private:
  int numBaseClassifier;
  int iterationInit;
  int numFeatures;
  bool trained;
  Size initPatchSize;
  Rect sampleROI;
  std::vector<int> replacedClassifier;
  std::vector<int> swappedClassifier;

  ConfidenceMap currentConfidenceMap;
};

/**
 * \brief TrackerStateEstimator based on SVM
 */
class CV_EXPORTS TrackerStateEstimatorSVM : public TrackerStateEstimator
{
 public:
  TrackerStateEstimatorSVM();
  ~TrackerStateEstimatorSVM();

 protected:
  Ptr<TrackerTargetState> estimateImpl( const std::vector<ConfidenceMap>& confidenceMaps ) CV_OVERRIDE;
  void updateImpl( std::vector<ConfidenceMap>& confidenceMaps ) CV_OVERRIDE;
};

/************************************ Specific TrackerSamplerAlgorithm Classes ************************************/

/** @brief TrackerSampler based on CSC (current state centered), used by MIL algorithm TrackerMIL
 */
class CV_EXPORTS TrackerSamplerCSC : public TrackerSamplerAlgorithm
{
 public:
  enum
  {
    MODE_INIT_POS = 1,  //!< mode for init positive samples
    MODE_INIT_NEG = 2,  //!< mode for init negative samples
    MODE_TRACK_POS = 3,  //!< mode for update positive samples
    MODE_TRACK_NEG = 4,  //!< mode for update negative samples
    MODE_DETECT = 5   //!< mode for detect samples
  };

  struct CV_EXPORTS Params
  {
    Params();
    float initInRad;        //!< radius for gathering positive instances during init
    float trackInPosRad;    //!< radius for gathering positive instances during tracking
    float searchWinSize;  //!< size of search window
    int initMaxNegNum;      //!< # negative samples to use during init
    int trackMaxPosNum;     //!< # positive samples to use during training
    int trackMaxNegNum;     //!< # negative samples to use during training
  };

  /** @brief Constructor
    @param parameters TrackerSamplerCSC parameters TrackerSamplerCSC::Params
     */
  TrackerSamplerCSC( const TrackerSamplerCSC::Params &parameters = TrackerSamplerCSC::Params() );

  /** @brief Set the sampling mode of TrackerSamplerCSC
    @param samplingMode The sampling mode

    The modes are:

    -   "MODE_INIT_POS = 1" -- for the positive sampling in initialization step
    -   "MODE_INIT_NEG = 2" -- for the negative sampling in initialization step
    -   "MODE_TRACK_POS = 3" -- for the positive sampling in update step
    -   "MODE_TRACK_NEG = 4" -- for the negative sampling in update step
    -   "MODE_DETECT = 5" -- for the sampling in detection step
     */
  void setMode( int samplingMode );

  ~TrackerSamplerCSC();

 protected:

  bool samplingImpl( const Mat& image, Rect boundingBox, std::vector<Mat>& sample ) CV_OVERRIDE;

 private:

  Params params;
  int mode;
  RNG rng;

  std::vector<Mat> sampleImage( const Mat& img, int x, int y, int w, int h, float inrad, float outrad = 0, int maxnum = 1000000 );
};

/** @brief TrackerSampler based on CS (current state), used by algorithm TrackerBoosting
 */
class CV_EXPORTS TrackerSamplerCS : public TrackerSamplerAlgorithm
{
 public:
  enum
  {
    MODE_POSITIVE = 1,  //!< mode for positive samples
    MODE_NEGATIVE = 2,  //!< mode for negative samples
    MODE_CLASSIFY = 3  //!< mode for classify samples
  };

  struct CV_EXPORTS Params
  {
    Params();
    float overlap;  //!<overlapping for the search windows
    float searchFactor;  //!<search region parameter
  };
  /** @brief Constructor
    @param parameters TrackerSamplerCS parameters TrackerSamplerCS::Params
     */
  TrackerSamplerCS( const TrackerSamplerCS::Params &parameters = TrackerSamplerCS::Params() );

  /** @brief Set the sampling mode of TrackerSamplerCS
    @param samplingMode The sampling mode

    The modes are:

    -   "MODE_POSITIVE = 1" -- for the positive sampling
    -   "MODE_NEGATIVE = 2" -- for the negative sampling
    -   "MODE_CLASSIFY = 3" -- for the sampling in classification step
     */
  void setMode( int samplingMode );

  ~TrackerSamplerCS();

  bool samplingImpl( const Mat& image, Rect boundingBox, std::vector<Mat>& sample ) CV_OVERRIDE;
  Rect getROI() const;
 private:
  Rect getTrackingROI( float searchFactor );
  Rect RectMultiply( const Rect & rect, float f );
  std::vector<Mat> patchesRegularScan( const Mat& image, Rect trackingROI, Size patchSize );
  void setCheckedROI( Rect imageROI );

  Params params;
  int mode;
  Rect trackedPatch;
  Rect validROI;
  Rect ROI;

};

/** @brief This sampler is based on particle filtering.

In principle, it can be thought of as performing some sort of optimization (and indeed, this
tracker uses opencv's optim module), where tracker seeks to find the rectangle in given frame,
which is the most *"similar"* to the initial rectangle (the one, given through the constructor).

The optimization performed is stochastic and somehow resembles genetic algorithms, where on each new
image received (submitted via TrackerSamplerPF::sampling()) we start with the region bounded by
boundingBox, then generate several "perturbed" boxes, take the ones most similar to the original.
This selection round is repeated several times. At the end, we hope that only the most promising box
remaining, and these are combined to produce the subrectangle of image, which is put as a sole
element in array sample.

It should be noted, that the definition of "similarity" between two rectangles is based on comparing
their histograms. As experiments show, tracker is *not* very succesfull if target is assumed to
strongly change its dimensions.
 */
class CV_EXPORTS TrackerSamplerPF : public TrackerSamplerAlgorithm
{
public:
  /** @brief This structure contains all the parameters that can be varied during the course of sampling
    algorithm. Below is the structure exposed, together with its members briefly explained with
    reference to the above discussion on algorithm's working.
 */
  struct CV_EXPORTS Params
  {
    Params();
    int iterationNum; //!< number of selection rounds
    int particlesNum; //!< number of "perturbed" boxes on each round
    double alpha; //!< with each new round we exponentially decrease the amount of "perturbing" we allow (like in simulated annealing)
                  //!< and this very alpha controls how fast annealing happens, ie. how fast perturbing decreases
    Mat_<double> std; //!< initial values for perturbing (1-by-4 array, as each rectangle is given by 4 values -- coordinates of opposite vertices,
                      //!< hence we have 4 values to perturb)
  };
  /** @brief Constructor
    @param chosenRect Initial rectangle, that is supposed to contain target we'd like to track.
    @param parameters
     */
  TrackerSamplerPF(const Mat& chosenRect,const TrackerSamplerPF::Params &parameters = TrackerSamplerPF::Params());
protected:
  bool samplingImpl( const Mat& image, Rect boundingBox, std::vector<Mat>& sample ) CV_OVERRIDE;
private:
  Params params;
  Ptr<MinProblemSolver> _solver;
  Ptr<MinProblemSolver::Function> _function;
};

/************************************ Specific TrackerFeature Classes ************************************/

/**
 * \brief TrackerFeature based on Feature2D
 */
class CV_EXPORTS TrackerFeatureFeature2d : public TrackerFeature
{
 public:

  /**
   * \brief Constructor
   * \param detectorType string of FeatureDetector
   * \param descriptorType string of DescriptorExtractor
   */
  TrackerFeatureFeature2d( String detectorType, String descriptorType );

  ~TrackerFeatureFeature2d() CV_OVERRIDE;

  void selection( Mat& response, int npoints ) CV_OVERRIDE;

 protected:

  bool computeImpl( const std::vector<Mat>& images, Mat& response ) CV_OVERRIDE;

 private:

  std::vector<KeyPoint> keypoints;
};

/**
 * \brief TrackerFeature based on HOG
 */
class CV_EXPORTS TrackerFeatureHOG : public TrackerFeature
{
 public:

  TrackerFeatureHOG();

  ~TrackerFeatureHOG() CV_OVERRIDE;

  void selection( Mat& response, int npoints ) CV_OVERRIDE;

 protected:

  bool computeImpl( const std::vector<Mat>& images, Mat& response ) CV_OVERRIDE;

};

/** @brief TrackerFeature based on HAAR features, used by TrackerMIL and many others algorithms
@note HAAR features implementation is copied from apps/traincascade and modified according to MIL
 */
class CV_EXPORTS TrackerFeatureHAAR : public TrackerFeature
{
 public:
  struct CV_EXPORTS Params
  {
    Params();
    int numFeatures;  //!< # of rects
    Size rectSize;    //!< rect size
    bool isIntegral;  //!< true if input images are integral, false otherwise
  };

  /** @brief Constructor
    @param parameters TrackerFeatureHAAR parameters TrackerFeatureHAAR::Params
     */
  TrackerFeatureHAAR( const TrackerFeatureHAAR::Params &parameters = TrackerFeatureHAAR::Params() );

  ~TrackerFeatureHAAR() CV_OVERRIDE;

  /** @brief Compute the features only for the selected indices in the images collection
    @param selFeatures indices of selected features
    @param images The images
    @param response Collection of response for the specific TrackerFeature
     */
  bool extractSelected( const std::vector<int> selFeatures, const std::vector<Mat>& images, Mat& response );

  /** @brief Identify most effective features
    @param response Collection of response for the specific TrackerFeature
    @param npoints Max number of features

    @note This method modifies the response parameter
     */
  void selection( Mat& response, int npoints ) CV_OVERRIDE;

  /** @brief Swap the feature in position source with the feature in position target
  @param source The source position
  @param target The target position
 */
  bool swapFeature( int source, int target );

  /** @brief   Swap the feature in position id with the feature input
  @param id The position
  @param feature The feature
 */
  bool swapFeature( int id, CvHaarEvaluator::FeatureHaar& feature );

  /** @brief Get the feature in position id
    @param id The position
     */
  CvHaarEvaluator::FeatureHaar& getFeatureAt( int id );

 protected:
  bool computeImpl( const std::vector<Mat>& images, Mat& response ) CV_OVERRIDE;

 private:

  Params params;
  Ptr<CvHaarEvaluator> featureEvaluator;
};

/**
 * \brief TrackerFeature based on LBP
 */
class CV_EXPORTS TrackerFeatureLBP : public TrackerFeature
{
 public:

  TrackerFeatureLBP();

  ~TrackerFeatureLBP();

  void selection( Mat& response, int npoints ) CV_OVERRIDE;

 protected:

  bool computeImpl( const std::vector<Mat>& images, Mat& response ) CV_OVERRIDE;

};

/************************************ Specific Tracker Classes ************************************/

/** @brief The MIL algorithm trains a classifier in an online manner to separate the object from the
background.

Multiple Instance Learning avoids the drift problem for a robust tracking. The implementation is
based on @cite MIL .

Original code can be found here <http://vision.ucsd.edu/~bbabenko/project_miltrack.shtml>
 */
class CV_EXPORTS_W TrackerMIL : public Tracker
{
 public:
  struct CV_EXPORTS Params
  {
    Params();
    //parameters for sampler
    float samplerInitInRadius;  //!< radius for gathering positive instances during init
    int samplerInitMaxNegNum;  //!< # negative samples to use during init
    float samplerSearchWinSize;  //!< size of search window
    float samplerTrackInRadius;  //!< radius for gathering positive instances during tracking
    int samplerTrackMaxPosNum;  //!< # positive samples to use during tracking
    int samplerTrackMaxNegNum;  //!< # negative samples to use during tracking
    int featureSetNumFeatures;  //!< # features

    void read( const FileNode& fn );
    void write( FileStorage& fs ) const;
  };

  /** @brief Constructor
    @param parameters MIL parameters TrackerMIL::Params
     */
  static Ptr<TrackerMIL> create(const TrackerMIL::Params &parameters);

  CV_WRAP static Ptr<TrackerMIL> create();

  virtual ~TrackerMIL() CV_OVERRIDE {}
};

/** @brief the Boosting tracker

This is a real-time object tracking based on a novel on-line version of the AdaBoost algorithm.
The classifier uses the surrounding background as negative examples in update step to avoid the
drifting problem. The implementation is based on @cite OLB .
 */
class CV_EXPORTS_W TrackerBoosting : public Tracker
{
 public:
  struct CV_EXPORTS Params
  {
    Params();
    int numClassifiers;  //!<the number of classifiers to use in a OnlineBoosting algorithm
    float samplerOverlap;  //!<search region parameters to use in a OnlineBoosting algorithm
    float samplerSearchFactor;  //!< search region parameters to use in a OnlineBoosting algorithm
    int iterationInit;  //!<the initial iterations
    int featureSetNumFeatures;  //!< # features
    /**
     * \brief Read parameters from a file
     */
    void read( const FileNode& fn );

    /**
     * \brief Write parameters to a file
     */
    void write( FileStorage& fs ) const;
  };

  /** @brief Constructor
    @param parameters BOOSTING parameters TrackerBoosting::Params
     */
  static Ptr<TrackerBoosting> create(const TrackerBoosting::Params &parameters);

  CV_WRAP static Ptr<TrackerBoosting> create();

  virtual ~TrackerBoosting() CV_OVERRIDE {}
};

/** @brief the Median Flow tracker

Implementation of a paper @cite MedianFlow .

The tracker is suitable for very smooth and predictable movements when object is visible throughout
the whole sequence. It's quite and accurate for this type of problems (in particular, it was shown
by authors to outperform MIL). During the implementation period the code at
<http://www.aonsquared.co.uk/node/5>, the courtesy of the author Arthur Amarra, was used for the
reference purpose.
 */
class CV_EXPORTS_W TrackerMedianFlow : public Tracker
{
 public:
  struct CV_EXPORTS Params
  {
    Params(); //!<default constructor
              //!<note that the default values of parameters are recommended for most of use cases
    int pointsInGrid;      //!<square root of number of keypoints used; increase it to trade
                           //!<accurateness for speed
    cv::Size winSize;      //!<window size parameter for Lucas-Kanade optical flow
    int maxLevel;          //!<maximal pyramid level number for Lucas-Kanade optical flow
    TermCriteria termCriteria; //!<termination criteria for Lucas-Kanade optical flow
    cv::Size winSizeNCC;   //!<window size around a point for normalized cross-correlation check
    double maxMedianLengthOfDisplacementDifference; //!<criterion for loosing the tracked object

    void read( const FileNode& /*fn*/ );
    void write( FileStorage& /*fs*/ ) const;
  };

  /** @brief Constructor
    @param parameters Median Flow parameters TrackerMedianFlow::Params
    */
  static Ptr<TrackerMedianFlow> create(const TrackerMedianFlow::Params &parameters);

  CV_WRAP static Ptr<TrackerMedianFlow> create();

  virtual ~TrackerMedianFlow() CV_OVERRIDE {}
};

/** @brief the TLD (Tracking, learning and detection) tracker

TLD is a novel tracking framework that explicitly decomposes the long-term tracking task into
tracking, learning and detection.

The tracker follows the object from frame to frame. The detector localizes all appearances that
have been observed so far and corrects the tracker if necessary. The learning estimates detector's
errors and updates it to avoid these errors in the future. The implementation is based on @cite TLD .

The Median Flow algorithm (see cv::TrackerMedianFlow) was chosen as a tracking component in this
implementation, following authors. The tracker is supposed to be able to handle rapid motions, partial
occlusions, object absence etc.
 */
class CV_EXPORTS_W TrackerTLD : public Tracker
{
 public:
  struct CV_EXPORTS Params
  {
    Params();
    void read( const FileNode& /*fn*/ );
    void write( FileStorage& /*fs*/ ) const;
  };

  /** @brief Constructor
    @param parameters TLD parameters TrackerTLD::Params
     */
  static Ptr<TrackerTLD> create(const TrackerTLD::Params &parameters);

  CV_WRAP static Ptr<TrackerTLD> create();

  virtual ~TrackerTLD() CV_OVERRIDE {}
};

/** @brief the KCF (Kernelized Correlation Filter) tracker

 * KCF is a novel tracking framework that utilizes properties of circulant matrix to enhance the processing speed.
 * This tracking method is an implementation of @cite KCF_ECCV which is extended to KCF with color-names features (@cite KCF_CN).
 * The original paper of KCF is available at <http://www.robots.ox.ac.uk/~joao/publications/henriques_tpami2015.pdf>
 * as well as the matlab implementation. For more information about KCF with color-names features, please refer to
 * <http://www.cvl.isy.liu.se/research/objrec/visualtracking/colvistrack/index.html>.
 */
class CV_EXPORTS_W TrackerKCF : public Tracker
{
public:
  /**
  * \brief Feature type to be used in the tracking grayscale, colornames, compressed color-names
  * The modes available now:
  -   "GRAY" -- Use grayscale values as the feature
  -   "CN" -- Color-names feature
  */
  enum MODE {
    GRAY   = (1 << 0),
    CN     = (1 << 1),
    CUSTOM = (1 << 2)
  };

  struct CV_EXPORTS Params
  {
    /**
    * \brief Constructor
    */
    Params();

    /**
    * \brief Read parameters from a file
    */
    void read(const FileNode& /*fn*/);

    /**
    * \brief Write parameters to a file
    */
    void write(FileStorage& /*fs*/) const;

    float detect_thresh;         //!<  detection confidence threshold
    float sigma;                 //!<  gaussian kernel bandwidth
    float lambda;                //!<  regularization
    float interp_factor;         //!<  linear interpolation factor for adaptation
    float output_sigma_factor;   //!<  spatial bandwidth (proportional to target)
    float pca_learning_rate;     //!<  compression learning rate
    bool resize;                  //!<  activate the resize feature to improve the processing speed
    bool split_coeff;             //!<  split the training coefficients into two matrices
    bool wrap_kernel;             //!<  wrap around the kernel values
    bool compress_feature;        //!<  activate the pca method to compress the features
    int max_patch_size;           //!<  threshold for the ROI size
    int compressed_size;          //!<  feature size after compression
    int desc_pca;        //!<  compressed descriptors of TrackerKCF::MODE
    int desc_npca;       //!<  non-compressed descriptors of TrackerKCF::MODE
  };

  virtual void setFeatureExtractor(void(*)(const Mat, const Rect, Mat&), bool pca_func = false) = 0;

  /** @brief Constructor
  @param parameters KCF parameters TrackerKCF::Params
  */
  static Ptr<TrackerKCF> create(const TrackerKCF::Params &parameters);

  CV_WRAP static Ptr<TrackerKCF> create();

  virtual ~TrackerKCF() CV_OVERRIDE {}
};

/** @brief the GOTURN (Generic Object Tracking Using Regression Networks) tracker

 *  GOTURN (@cite GOTURN) is kind of trackers based on Convolutional Neural Networks (CNN). While taking all advantages of CNN trackers,
 *  GOTURN is much faster due to offline training without online fine-tuning nature.
 *  GOTURN tracker addresses the problem of single target tracking: given a bounding box label of an object in the first frame of the video,
 *  we track that object through the rest of the video. NOTE: Current method of GOTURN does not handle occlusions; however, it is fairly
 *  robust to viewpoint changes, lighting changes, and deformations.
 *  Inputs of GOTURN are two RGB patches representing Target and Search patches resized to 227x227.
 *  Outputs of GOTURN are predicted bounding box coordinates, relative to Search patch coordinate system, in format X1,Y1,X2,Y2.
 *  Original paper is here: <http://davheld.github.io/GOTURN/GOTURN.pdf>
 *  As long as original authors implementation: <https://github.com/davheld/GOTURN#train-the-tracker>
 *  Implementation of training algorithm is placed in separately here due to 3d-party dependencies:
 *  <https://github.com/Auron-X/GOTURN_Training_Toolkit>
 *  GOTURN architecture goturn.prototxt and trained model goturn.caffemodel are accessible on opencv_extra GitHub repository.
*/
class CV_EXPORTS_W TrackerGOTURN : public Tracker
{
public:
  struct CV_EXPORTS Params
  {
    Params();
    void read(const FileNode& /*fn*/);
    void write(FileStorage& /*fs*/) const;
    String modelTxt;
    String modelBin;
  };

  /** @brief Constructor
  @param parameters GOTURN parameters TrackerGOTURN::Params
  */
  static Ptr<TrackerGOTURN> create(const TrackerGOTURN::Params &parameters);

  CV_WRAP static Ptr<TrackerGOTURN> create();

  virtual ~TrackerGOTURN() CV_OVERRIDE {}
};

/** @brief the MOSSE (Minimum Output Sum of Squared %Error) tracker

The implementation is based on @cite MOSSE Visual Object Tracking using Adaptive Correlation Filters
@note this tracker works with grayscale images, if passed bgr ones, they will get converted internally.
*/

class CV_EXPORTS_W TrackerMOSSE : public Tracker
{
 public:
  /** @brief Constructor
  */
  CV_WRAP static Ptr<TrackerMOSSE> create();

  virtual ~TrackerMOSSE() CV_OVERRIDE {}
};


/************************************ MultiTracker Class ---By Laksono Kurnianggoro---) ************************************/
/** @brief This class is used to track multiple objects using the specified tracker algorithm.

* The %MultiTracker is naive implementation of multiple object tracking.
* It process the tracked objects independently without any optimization accross the tracked objects.
*/
class CV_EXPORTS_W MultiTracker : public Algorithm
{
public:

  /**
  * \brief Constructor.
  */
  CV_WRAP MultiTracker();

  /**
  * \brief Destructor
  */
  ~MultiTracker() CV_OVERRIDE;

  /**
  * \brief Add a new object to be tracked.
  *
  * @param newTracker tracking algorithm to be used
  * @param image input image
  * @param boundingBox a rectangle represents ROI of the tracked object
  */
  CV_WRAP bool add(Ptr<Tracker> newTracker, InputArray image, const Rect2d& boundingBox);

  /**
  * \brief Add a set of objects to be tracked.
  * @param newTrackers list of tracking algorithms to be used
  * @param image input image
  * @param boundingBox list of the tracked objects
  */
  bool add(std::vector<Ptr<Tracker> > newTrackers, InputArray image, std::vector<Rect2d> boundingBox);

  /**
  * \brief Update the current tracking status.
  * The result will be saved in the internal storage.
  * @param image input image
  */
  bool update(InputArray image);

  /**
  * \brief Update the current tracking status.
  * @param image input image
  * @param boundingBox the tracking result, represent a list of ROIs of the tracked objects.
  */
  CV_WRAP bool update(InputArray image, CV_OUT std::vector<Rect2d> & boundingBox);

  /**
  * \brief Returns a reference to a storage for the tracked objects, each object corresponds to one tracker algorithm
  */
  CV_WRAP const std::vector<Rect2d>& getObjects() const;

  /**
  * \brief Returns a pointer to a new instance of MultiTracker
  */
  CV_WRAP static Ptr<MultiTracker> create();

protected:
  //!<  storage for the tracker algorithms.
  std::vector< Ptr<Tracker> > trackerList;

  //!<  storage for the tracked objects, each object corresponds to one tracker algorithm.
  std::vector<Rect2d> objects;
};

/************************************ Multi-Tracker Classes ---By Tyan Vladimir---************************************/

/** @brief Base abstract class for the long-term Multi Object Trackers:

@sa Tracker, MultiTrackerTLD
*/
class CV_EXPORTS MultiTracker_Alt
{
public:
  /** @brief Constructor for Multitracker
  */
  MultiTracker_Alt()
  {
    targetNum = 0;
  }

  /** @brief Add a new target to a tracking-list and initialize the tracker with a known bounding box that surrounded the target
  @param image The initial frame
  @param boundingBox The initial bounding box of target
  @param tracker_algorithm Multi-tracker algorithm

  @return True if new target initialization went succesfully, false otherwise
  */
  bool addTarget(InputArray image, const Rect2d& boundingBox, Ptr<Tracker> tracker_algorithm);

  /** @brief Update all trackers from the tracking-list, find a new most likely bounding boxes for the targets
  @param image The current frame

  @return True means that all targets were located and false means that tracker couldn't locate one of the targets in
  current frame. Note, that latter *does not* imply that tracker has failed, maybe target is indeed
  missing from the frame (say, out of sight)
  */
  bool update(InputArray image);

  /** @brief Current number of targets in tracking-list
  */
  int targetNum;

  /** @brief Trackers list for Multi-Object-Tracker
  */
  std::vector <Ptr<Tracker> > trackers;

  /** @brief Bounding Boxes list for Multi-Object-Tracker
  */
  std::vector <Rect2d> boundingBoxes;
  /** @brief List of randomly generated colors for bounding boxes display
  */
  std::vector<Scalar> colors;
};

/** @brief Multi Object %Tracker for TLD.

TLD is a novel tracking framework that explicitly decomposes
the long-term tracking task into tracking, learning and detection.

The tracker follows the object from frame to frame. The detector localizes all appearances that
have been observed so far and corrects the tracker if necessary. The learning estimates detector's
errors and updates it to avoid these errors in the future. The implementation is based on @cite TLD .

The Median Flow algorithm (see cv::TrackerMedianFlow) was chosen as a tracking component in this
implementation, following authors. The tracker is supposed to be able to handle rapid motions, partial
occlusions, object absence etc.

@sa Tracker, MultiTracker, TrackerTLD
*/
class CV_EXPORTS MultiTrackerTLD : public MultiTracker_Alt
{
public:
  /** @brief Update all trackers from the tracking-list, find a new most likely bounding boxes for the targets by
  optimized update method using some techniques to speedup calculations specifically for MO TLD. The only limitation
  is that all target bounding boxes should have approximately same aspect ratios. Speed boost is around 20%

  @param image The current frame.

  @return True means that all targets were located and false means that tracker couldn't locate one of the targets in
  current frame. Note, that latter *does not* imply that tracker has failed, maybe target is indeed
  missing from the frame (say, out of sight)
  */
  bool update_opt(InputArray image);
};

/*********************************** CSRT ************************************/
/** @brief the CSRT tracker

The implementation is based on @cite Lukezic_IJCV2018 Discriminative Correlation Filter with Channel and Spatial Reliability
*/
class CV_EXPORTS_W TrackerCSRT : public Tracker
{
public:
  struct CV_EXPORTS Params
  {
    /**
    * \brief Constructor
    */
    Params();

    /**
    * \brief Read parameters from a file
    */
    void read(const FileNode& /*fn*/);

    /**
    * \brief Write parameters to a file
    */
    void write(cv::FileStorage& fs) const;

    bool use_hog;
    bool use_color_names;
    bool use_gray;
    bool use_rgb;
    bool use_channel_weights;
    bool use_segmentation;

    std::string window_function; //!<  Window function: "hann", "cheb", "kaiser"
    float kaiser_alpha;
    float cheb_attenuation;

    float template_size;
    float gsl_sigma;
    float hog_orientations;
    float hog_clip;
    float padding;
    float filter_lr;
    float weights_lr;
    int num_hog_channels_used;
    int admm_iterations;
    int histogram_bins;
    float histogram_lr;
    int background_ratio;
    int number_of_scales;
    float scale_sigma_factor;
    float scale_model_max_area;
    float scale_lr;
    float scale_step;

    float psr_threshold; //!< we lost the target, if the psr is lower than this.
  };

  /** @brief Constructor
  @param parameters CSRT parameters TrackerCSRT::Params
  */
  static Ptr<TrackerCSRT> create(const TrackerCSRT::Params &parameters);

  CV_WRAP static Ptr<TrackerCSRT> create();

  virtual void setInitialMask(const Mat mask) = 0;

  virtual ~TrackerCSRT() CV_OVERRIDE {}
};

//! @}
} /* namespace cv */

#endif
