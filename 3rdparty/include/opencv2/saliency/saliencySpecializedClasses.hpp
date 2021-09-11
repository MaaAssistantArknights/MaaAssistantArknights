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

#ifndef __OPENCV_SALIENCY_SPECIALIZED_CLASSES_HPP__
#define __OPENCV_SALIENCY_SPECIALIZED_CLASSES_HPP__

#include <cstdio>
#include <string>
#include <iostream>
#include <stdint.h>
#include "saliencyBaseClasses.hpp"
#include "opencv2/core.hpp"

namespace cv
{
namespace saliency
{

//! @addtogroup saliency
//! @{

/************************************ Specific Static Saliency Specialized Classes ************************************/

/** @brief the Spectral Residual approach from  @cite SR

Starting from the principle of natural image statistics, this method simulate the behavior of
pre-attentive visual search. The algorithm analyze the log spectrum of each image and obtain the
spectral residual. Then transform the spectral residual to spatial domain to obtain the saliency
map, which suggests the positions of proto-objects.
 */
class CV_EXPORTS_W StaticSaliencySpectralResidual : public StaticSaliency
{
public:

  StaticSaliencySpectralResidual();
  virtual ~StaticSaliencySpectralResidual();

  CV_WRAP static Ptr<StaticSaliencySpectralResidual> create()
  {
    return makePtr<StaticSaliencySpectralResidual>();
  }

  CV_WRAP bool computeSaliency( InputArray image, OutputArray saliencyMap )
  {
    if( image.empty() )
      return false;

    return computeSaliencyImpl( image, saliencyMap );
  }

  CV_WRAP void read( const FileNode& fn ) CV_OVERRIDE;
  void write( FileStorage& fs ) const CV_OVERRIDE;

  CV_WRAP int getImageWidth() const
  {
    return resImWidth;
  }
  CV_WRAP inline void setImageWidth(int val)
  {
    resImWidth = val;
  }
  CV_WRAP int getImageHeight() const
  {
    return resImHeight;
  }
  CV_WRAP void setImageHeight(int val)
  {
    resImHeight = val;
  }

protected:
  bool computeSaliencyImpl( InputArray image, OutputArray saliencyMap ) CV_OVERRIDE;
  CV_PROP_RW int resImWidth;
  CV_PROP_RW int resImHeight;

};


/** @brief the Fine Grained Saliency approach from @cite FGS

This method calculates saliency based on center-surround differences.
High resolution saliency maps are generated in real time by using integral images.
 */
class CV_EXPORTS_W StaticSaliencyFineGrained : public StaticSaliency
{
public:

  StaticSaliencyFineGrained();

  CV_WRAP static Ptr<StaticSaliencyFineGrained> create()
  {
    return makePtr<StaticSaliencyFineGrained>();
  }

  CV_WRAP bool computeSaliency( InputArray image, OutputArray saliencyMap )
  {
    if( image.empty() )
      return false;

    return computeSaliencyImpl( image, saliencyMap );
  }
  virtual ~StaticSaliencyFineGrained();

protected:
  bool computeSaliencyImpl( InputArray image, OutputArray saliencyMap ) CV_OVERRIDE;

private:
  void calcIntensityChannel(Mat src, Mat dst);
  void copyImage(Mat src, Mat dst);
  void getIntensityScaled(Mat integralImage, Mat gray, Mat saliencyOn, Mat saliencyOff, int neighborhood);
  float getMean(Mat srcArg, Point2i PixArg, int neighbourhood, int centerVal);
  void mixScales(Mat *saliencyOn, Mat intensityOn, Mat *saliencyOff, Mat intensityOff, const int numScales);
  void mixOnOff(Mat intensityOn, Mat intensityOff, Mat intensity);
  void getIntensity(Mat srcArg, Mat dstArg,  Mat dstOnArg,  Mat dstOffArg, bool generateOnOff);
};




/************************************ Specific Motion Saliency Specialized Classes ************************************/

/*!
 * A Fast Self-tuning Background Subtraction Algorithm.
 *
 * This background subtraction algorithm is inspired to the work of B. Wang and P. Dudek [2]
 * [2]  B. Wang and P. Dudek "A Fast Self-tuning Background Subtraction Algorithm", in proc of IEEE Workshop on Change Detection, 2014
 *
 */
/** @brief the Fast Self-tuning Background Subtraction Algorithm from @cite BinWangApr2014
 */
class CV_EXPORTS_W MotionSaliencyBinWangApr2014 : public MotionSaliency
{
public:
  MotionSaliencyBinWangApr2014();
  virtual ~MotionSaliencyBinWangApr2014();

  CV_WRAP static Ptr<MotionSaliencyBinWangApr2014> create()
  {
    return makePtr<MotionSaliencyBinWangApr2014>();
  }

  CV_WRAP bool computeSaliency( InputArray image, OutputArray saliencyMap )
  {
    if( image.empty() )
      return false;

    return computeSaliencyImpl( image, saliencyMap );
  }

  /** @brief This is a utility function that allows to set the correct size (taken from the input image) in the
    corresponding variables that will be used to size the data structures of the algorithm.
    @param W width of input image
    @param H height of input image
  */
  CV_WRAP void setImagesize( int W, int H );
  /** @brief This function allows the correct initialization of all data structures that will be used by the
    algorithm.
  */
  CV_WRAP bool init();

  CV_WRAP int getImageWidth() const
  {
    return imageWidth;
  }
  CV_WRAP inline void setImageWidth(int val)
  {
    imageWidth = val;
  }
  CV_WRAP int getImageHeight() const
  {
    return imageHeight;
  }
  CV_WRAP void setImageHeight(int val)
  {
    imageHeight = val;
  }

protected:
  /** @brief Performs all the operations and calls all internal functions necessary for the accomplishment of the
    Fast Self-tuning Background Subtraction Algorithm algorithm.
    @param image input image. According to the needs of this specialized algorithm, the param image is a
    single *Mat*.
    @param saliencyMap Saliency Map. Is a binarized map that, in accordance with the nature of the algorithm, highlights the moving objects or areas of change in the scene.
       The saliency map is given by a single *Mat* (one for each frame of an hypothetical video
        stream).
  */
  bool computeSaliencyImpl( InputArray image, OutputArray saliencyMap ) CV_OVERRIDE;

private:

  // classification (and adaptation) functions
  bool fullResolutionDetection( const Mat& image, Mat& highResBFMask );
  bool lowResolutionDetection( const Mat& image, Mat& lowResBFMask );

  // Background model maintenance functions
  bool templateOrdering();
  bool templateReplacement( const Mat& finalBFMask, const Mat& image );

  // Decision threshold adaptation and Activity control function
  bool activityControl(const Mat& current_noisePixelsMask);
  bool decisionThresholdAdaptation();

  // changing structure
  std::vector<Ptr<Mat> > backgroundModel;// The vector represents the background template T0---TK of reference paper.
  // Matrices are two-channel matrix. In the first layer there are the B (background value)
  // for each pixel. In the second layer, there are the C (efficacy) value for each pixel
  Mat potentialBackground;// Two channel Matrix. For each pixel, in the first level there are the Ba value (potential background value)
                          // and in the secon level there are the Ca value, the counter for each potential value.
  Mat epslonPixelsValue;// epslon threshold

  Mat activityPixelsValue;// Activity level of each pixel

  //vector<Mat> noisePixelMask; // We define a ‘noise-pixel’ as a pixel that has been classified as a foreground pixel during the full resolution
  Mat noisePixelMask;// We define a ‘noise-pixel’ as a pixel that has been classified as a foreground pixel during the full resolution
  //detection process,however, after the low resolution detection, it has become a
  // background pixel. The matrix is  two-channel matrix. In the first layer there is the mask ( the identified noise-pixels are set to 1 while other pixels are 0)
  // for each pixel. In the second layer, there is the value of activity level A for each pixel.

  //fixed parameter
  bool activityControlFlag;
  bool neighborhoodCheck;
  int N_DS;// Number of template to be downsampled and used in lowResolutionDetection function
  CV_PROP_RW int imageWidth;// Width of input image
  CV_PROP_RW int imageHeight;//Height of input image
  int K;// Number of background model template
  int N;// NxN is the size of the block for downsampling in the lowlowResolutionDetection
  float alpha;// Learning rate
  int L0, L1;// Upper-bound values for C0 and C1 (efficacy of the first two template (matrices) of backgroundModel
  int thetaL;// T0, T1 swap threshold
  int thetaA;// Potential background value threshold
  int gamma;// Parameter that controls the time that the newly updated long-term background value will remain in the
            // long-term template, regardless of any subsequent background changes. A relatively large (eg gamma=3) will
            //restrain the generation of ghosts.

  uchar Ainc;// Activity Incrementation;
  int Bmax;// Upper-bound value for pixel activity
  int Bth;// Max activity threshold
  int Binc, Bdec;// Threshold for pixel-level decision threshold (epslon) adaptation
  float deltaINC, deltaDEC;// Increment-decrement value for epslon adaptation
  int epslonMIN, epslonMAX;// Range values for epslon threshold

};

/************************************ Specific Objectness Specialized Classes ************************************/

/**
 * \brief Objectness algorithms based on [3]
 * [3] Cheng, Ming-Ming, et al. "BING: Binarized normed gradients for objectness estimation at 300fps." IEEE CVPR. 2014.
 */

/** @brief the Binarized normed gradients algorithm from @cite BING
 */
class CV_EXPORTS_W ObjectnessBING : public Objectness
{
public:

  ObjectnessBING();
  virtual ~ObjectnessBING();

  CV_WRAP static Ptr<ObjectnessBING> create()
  {
    return makePtr<ObjectnessBING>();
  }

  CV_WRAP bool computeSaliency( InputArray image, OutputArray saliencyMap )
  {
    if( image.empty() )
      return false;

    return computeSaliencyImpl( image, saliencyMap );
  }

  CV_WRAP void read();
  CV_WRAP void write() const;

  /** @brief Return the list of the rectangles' objectness value,

    in the same order as the *vector\<Vec4i\> objectnessBoundingBox* returned by the algorithm (in
    computeSaliencyImpl function). The bigger value these scores are, it is more likely to be an
    object window.
     */
  CV_WRAP std::vector<float> getobjectnessValues();

  /** @brief This is a utility function that allows to set the correct path from which the algorithm will load
    the trained model.
    @param trainingPath trained model path
     */
  CV_WRAP void setTrainingPath( const String& trainingPath );

  /** @brief This is a utility function that allows to set an arbitrary path in which the algorithm will save the
    optional results

    (ie writing on file the total number and the list of rectangles returned by objectess, one for
    each row).
    @param resultsDir results' folder path
     */
  CV_WRAP void setBBResDir( const String& resultsDir );

  CV_WRAP double getBase() const
  {
    return _base;
  }
  CV_WRAP inline void setBase(double val)
  {
    _base = val;
  }
  CV_WRAP int getNSS() const
  {
    return _NSS;
  }
  CV_WRAP void setNSS(int val)
  {
    _NSS = val;
  }
  CV_WRAP int getW() const
  {
    return _W;
  }
  CV_WRAP void setW(int val)
  {
    _W = val;
  }

protected:
  /** @brief Performs all the operations and calls all internal functions necessary for the
  accomplishment of the Binarized normed gradients algorithm.

    @param image input image. According to the needs of this specialized algorithm, the param image is a
    single *Mat*
    @param objectnessBoundingBox objectness Bounding Box vector. According to the result given by this
    specialized algorithm, the objectnessBoundingBox is a *vector\<Vec4i\>*. Each bounding box is
    represented by a *Vec4i* for (minX, minY, maxX, maxY).
     */
  bool computeSaliencyImpl( InputArray image, OutputArray objectnessBoundingBox ) CV_OVERRIDE;

private:

  class FilterTIG
  {
  public:
    void update( Mat &w );

    // For a W by H gradient magnitude map, find a W-7 by H-7 CV_32F matching score map
    Mat matchTemplate( const Mat &mag1u );

    float dot( int64_t tig1, int64_t tig2, int64_t tig4, int64_t tig8 );
    void reconstruct( Mat &w );// For illustration purpose

  private:
    static const int NUM_COMP = 2;// Number of components
    static const int D = 64;// Dimension of TIG
    int64_t _bTIGs[NUM_COMP];// Binary TIG features
    float _coeffs1[NUM_COMP];// Coefficients of binary TIG features

    // For efficiently deals with different bits in CV_8U gradient map
    float _coeffs2[NUM_COMP], _coeffs4[NUM_COMP], _coeffs8[NUM_COMP];
  };

  template<typename VT, typename ST>
  struct ValStructVec
  {
    ValStructVec();
    int size() const;
    void clear();
    void reserve( int resSz );
    void pushBack( const VT& val, const ST& structVal );
    const VT& operator ()( int i ) const;
    const ST& operator []( int i ) const;
    VT& operator ()( int i );
    ST& operator []( int i );

    void sort( bool descendOrder = true );
    const std::vector<ST> &getSortedStructVal();
    std::vector<std::pair<VT, int> > getvalIdxes();
    void append( const ValStructVec<VT, ST> &newVals, int startV = 0 );

    std::vector<ST> structVals;  // struct values
    int sz;// size of the value struct vector
    std::vector<std::pair<VT, int> > valIdxes;// Indexes after sort
    bool smaller()
    {
      return true;
    }
    std::vector<ST> sortedStructVals;
  };

  enum
  {
    MAXBGR,
    HSV,
    G
  };

  double _base, _logBase;  // base for window size quantization
  int _W;// As described in the paper: #Size, Size(_W, _H) of feature window.
  int _NSS;// Size for non-maximal suppress
  int _maxT, _minT, _numT;// The minimal and maximal dimensions of the template

  int _Clr;//
  static const char* _clrName[3];

// Names and paths to read model and to store results
  std::string _modelName, _bbResDir, _trainingPath, _resultsDir;

  std::vector<int> _svmSzIdxs;// Indexes of active size. It's equal to _svmFilters.size() and _svmReW1f.rows
  Mat _svmFilter;// Filters learned at stage I, each is a _H by _W CV_32F matrix
  FilterTIG _tigF;// TIG filter
  Mat _svmReW1f;// Re-weight parameters learned at stage II.

// List of the rectangles' objectness value, in the same order as
// the  vector<Vec4i> objectnessBoundingBox returned by the algorithm (in computeSaliencyImpl function)
  std::vector<float> objectnessValues;

private:
// functions

  inline static float LoG( float x, float y, float delta )
  {
    float d = - ( x * x + y * y ) / ( 2 * delta * delta );
    return -1.0f / ( (float) ( CV_PI ) * pow( delta, 4 ) ) * ( 1 + d ) * exp( d );
  }  // Laplacian of Gaussian

// Read matrix from binary file
  static bool matRead( const std::string& filename, Mat& M );

  void setColorSpace( int clr = MAXBGR );

// Load trained model.
  int loadTrainedModel();// Return -1, 0, or 1 if partial, none, or all loaded

// Get potential bounding boxes, each of which is represented by a Vec4i for (minX, minY, maxX, maxY).
// The trained model should be prepared before calling this function: loadTrainedModel() or trainStageI() + trainStageII().
// Use numDet to control the final number of proposed bounding boxes, and number of per size (scale and aspect ratio)
  void getObjBndBoxes( Mat &img3u, ValStructVec<float, Vec4i> &valBoxes, int numDetPerSize = 120 );
  void getObjBndBoxesForSingleImage( Mat img, ValStructVec<float, Vec4i> &boxes, int numDetPerSize );

  bool filtersLoaded()
  {
    int n = (int) _svmSzIdxs.size();
    return n > 0 && _svmReW1f.size() == Size( 2, n ) && _svmFilter.size() == Size( _W, _W );
  }
  void predictBBoxSI( Mat &mag3u, ValStructVec<float, Vec4i> &valBoxes, std::vector<int> &sz, int NUM_WIN_PSZ = 100, bool fast = true );
  void predictBBoxSII( ValStructVec<float, Vec4i> &valBoxes, const std::vector<int> &sz );

// Calculate the image gradient: center option as in VLFeat
  void gradientMag( Mat &imgBGR3u, Mat &mag1u );

  static void gradientRGB( Mat &bgr3u, Mat &mag1u );
  static void gradientGray( Mat &bgr3u, Mat &mag1u );
  static void gradientHSV( Mat &bgr3u, Mat &mag1u );
  static void gradientXY( Mat &x1i, Mat &y1i, Mat &mag1u );

  static inline int bgrMaxDist( const Vec3b &u, const Vec3b &v )
  {
    int b = abs( u[0] - v[0] ), g = abs( u[1] - v[1] ), r = abs( u[2] - v[2] );
    b = max( b, g );
    return max( b, r );
  }
  static inline int vecDist3b( const Vec3b &u, const Vec3b &v )
  {
    return abs( u[0] - v[0] ) + abs( u[1] - v[1] ) + abs( u[2] - v[2] );
  }

//Non-maximal suppress
  static void nonMaxSup( Mat &matchCost1f, ValStructVec<float, Point> &matchCost, int NSS = 1, int maxPoint = 50, bool fast = true );

};

//! @}

}
/* namespace saliency */
} /* namespace cv */

#endif
