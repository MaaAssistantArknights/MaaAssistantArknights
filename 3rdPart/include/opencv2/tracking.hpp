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

#ifndef __OPENCV_TRACKING_HPP__
#define __OPENCV_TRACKING_HPP__

#include "opencv2/core/cvdef.h"

/** @defgroup tracking Tracking API

Long-term optical tracking API
------------------------------

Long-term optical tracking is an important issue for many computer vision applications in
real world scenario. The development in this area is very fragmented and this API is an unique
interface useful for plug several algorithms and compare them. This work is partially based on
@cite AAM and @cite AMVOT .

These algorithms start from a bounding box of the target and with their internal representation they
avoid the drift during the tracking. These long-term trackers are able to evaluate online the
quality of the location of the target in the new frame, without ground truth.

There are three main components: the TrackerSampler, the TrackerFeatureSet and the TrackerModel. The
first component is the object that computes the patches over the frame based on the last target
location. The TrackerFeatureSet is the class that manages the Features, is possible plug many kind
of these (HAAR, HOG, LBP, Feature2D, etc). The last component is the internal representation of the
target, it is the appearance model. It stores all state candidates and compute the trajectory (the
most likely target states). The class TrackerTargetState represents a possible state of the target.
The TrackerSampler and the TrackerFeatureSet are the visual representation of the target, instead
the TrackerModel is the statistical model.

A recent benchmark between these algorithms can be found in @cite OOT

Creating Your Own %Tracker
--------------------

If you want to create a new tracker, here's what you have to do. First, decide on the name of the class
for the tracker (to meet the existing style, we suggest something with prefix "tracker", e.g.
trackerMIL, trackerBoosting) -- we shall refer to this choice as to "classname" in subsequent.

-   Declare your tracker in modules/tracking/include/opencv2/tracking/tracker.hpp. Your tracker should inherit from
    Tracker (please, see the example below). You should declare the specialized Param structure,
    where you probably will want to put the data, needed to initialize your tracker. You should
    get something similar to :
@code
        class CV_EXPORTS_W TrackerMIL : public Tracker
        {
         public:
          struct CV_EXPORTS Params
          {
            Params();
            //parameters for sampler
            float samplerInitInRadius;  // radius for gathering positive instances during init
            int samplerInitMaxNegNum;  // # negative samples to use during init
            float samplerSearchWinSize;  // size of search window
            float samplerTrackInRadius;  // radius for gathering positive instances during tracking
            int samplerTrackMaxPosNum;  // # positive samples to use during tracking
            int samplerTrackMaxNegNum;  // # negative samples to use during tracking
            int featureSetNumFeatures;  // #features

            void read( const FileNode& fn );
            void write( FileStorage& fs ) const;
          };
@endcode
    of course, you can also add any additional methods of your choice. It should be pointed out,
    however, that it is not expected to have a constructor declared, as creation should be done via
    the corresponding create() method.
-   Finally, you should implement the function with signature :
@code
        Ptr<classname> classname::create(const classname::Params &parameters){
            ...
        }
@endcode
    That function can (and probably will) return a pointer to some derived class of "classname",
    which will probably have a real constructor.

Every tracker has three component TrackerSampler, TrackerFeatureSet and TrackerModel. The first two
are instantiated from Tracker base class, instead the last component is abstract, so you must
implement your TrackerModel.

### TrackerSampler

TrackerSampler is already instantiated, but you should define the sampling algorithm and add the
classes (or single class) to TrackerSampler. You can choose one of the ready implementation as
TrackerSamplerCSC or you can implement your sampling method, in this case the class must inherit
TrackerSamplerAlgorithm. Fill the samplingImpl method that writes the result in "sample" output
argument.

Example of creating specialized TrackerSamplerAlgorithm TrackerSamplerCSC : :
@code
    class CV_EXPORTS_W TrackerSamplerCSC : public TrackerSamplerAlgorithm
    {
     public:
      TrackerSamplerCSC( const TrackerSamplerCSC::Params &parameters = TrackerSamplerCSC::Params() );
      ~TrackerSamplerCSC();
      ...

     protected:
      bool samplingImpl( const Mat& image, Rect boundingBox, std::vector<Mat>& sample );
      ...

    };
@endcode

Example of adding TrackerSamplerAlgorithm to TrackerSampler : :
@code
    //sampler is the TrackerSampler
    Ptr<TrackerSamplerAlgorithm> CSCSampler = new TrackerSamplerCSC( CSCparameters );
    if( !sampler->addTrackerSamplerAlgorithm( CSCSampler ) )
     return false;

    //or add CSC sampler with default parameters
    //sampler->addTrackerSamplerAlgorithm( "CSC" );
@endcode
@sa
   TrackerSamplerCSC, TrackerSamplerAlgorithm

### TrackerFeatureSet

TrackerFeatureSet is already instantiated (as first) , but you should define what kinds of features
you'll use in your tracker. You can use multiple feature types, so you can add a ready
implementation as TrackerFeatureHAAR in your TrackerFeatureSet or develop your own implementation.
In this case, in the computeImpl method put the code that extract the features and in the selection
method optionally put the code for the refinement and selection of the features.

Example of creating specialized TrackerFeature TrackerFeatureHAAR : :
@code
    class CV_EXPORTS_W TrackerFeatureHAAR : public TrackerFeature
    {
     public:
      TrackerFeatureHAAR( const TrackerFeatureHAAR::Params &parameters = TrackerFeatureHAAR::Params() );
      ~TrackerFeatureHAAR();
      void selection( Mat& response, int npoints );
      ...

     protected:
      bool computeImpl( const std::vector<Mat>& images, Mat& response );
      ...

    };
@endcode
Example of adding TrackerFeature to TrackerFeatureSet : :
@code
    //featureSet is the TrackerFeatureSet
    Ptr<TrackerFeature> trackerFeature = new TrackerFeatureHAAR( HAARparameters );
    featureSet->addTrackerFeature( trackerFeature );
@endcode
@sa
   TrackerFeatureHAAR, TrackerFeatureSet

### TrackerModel

TrackerModel is abstract, so in your implementation you must develop your TrackerModel that inherit
from TrackerModel. Fill the method for the estimation of the state "modelEstimationImpl", that
estimates the most likely target location, see @cite AAM table I (ME) for further information. Fill
"modelUpdateImpl" in order to update the model, see @cite AAM table I (MU). In this class you can use
the :cConfidenceMap and :cTrajectory to storing the model. The first represents the model on the all
possible candidate states and the second represents the list of all estimated states.

Example of creating specialized TrackerModel TrackerMILModel : :
@code
    class TrackerMILModel : public TrackerModel
    {
     public:
      TrackerMILModel( const Rect& boundingBox );
      ~TrackerMILModel();
      ...

     protected:
      void modelEstimationImpl( const std::vector<Mat>& responses );
      void modelUpdateImpl();
      ...

    };
@endcode
And add it in your Tracker : :
@code
    bool TrackerMIL::initImpl( const Mat& image, const Rect2d& boundingBox )
    {
      ...
      //model is the general TrackerModel field of the general Tracker
      model = new TrackerMILModel( boundingBox );
      ...
    }
@endcode
In the last step you should define the TrackerStateEstimator based on your implementation or you can
use one of ready class as TrackerStateEstimatorMILBoosting. It represent the statistical part of the
model that estimates the most likely target state.

Example of creating specialized TrackerStateEstimator TrackerStateEstimatorMILBoosting : :
@code
    class CV_EXPORTS_W TrackerStateEstimatorMILBoosting : public TrackerStateEstimator
    {
     class TrackerMILTargetState : public TrackerTargetState
     {
     ...
     };

     public:
      TrackerStateEstimatorMILBoosting( int nFeatures = 250 );
      ~TrackerStateEstimatorMILBoosting();
      ...

     protected:
      Ptr<TrackerTargetState> estimateImpl( const std::vector<ConfidenceMap>& confidenceMaps );
      void updateImpl( std::vector<ConfidenceMap>& confidenceMaps );
      ...

    };
@endcode
And add it in your TrackerModel : :
@code
    //model is the TrackerModel of your Tracker
    Ptr<TrackerStateEstimatorMILBoosting> stateEstimator = new TrackerStateEstimatorMILBoosting( params.featureSetNumFeatures );
    model->setTrackerStateEstimator( stateEstimator );
@endcode
@sa
   TrackerModel, TrackerStateEstimatorMILBoosting, TrackerTargetState

During this step, you should define your TrackerTargetState based on your implementation.
TrackerTargetState base class has only the bounding box (upper-left position, width and height), you
can enrich it adding scale factor, target rotation, etc.

Example of creating specialized TrackerTargetState TrackerMILTargetState : :
@code
    class TrackerMILTargetState : public TrackerTargetState
    {
     public:
      TrackerMILTargetState( const Point2f& position, int targetWidth, int targetHeight, bool foreground, const Mat& features );
      ~TrackerMILTargetState();
      ...

     private:
      bool isTarget;
      Mat targetFeatures;
      ...

    };
@endcode

*/

#include <opencv2/tracking/tracker.hpp>
#include <opencv2/tracking/tldDataset.hpp>

#endif //__OPENCV_TRACKING_HPP__
