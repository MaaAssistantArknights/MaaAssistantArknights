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
 // Copyright (C) 2014, Biagio Montesano, all rights reserved.
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

#ifndef __OPENCV_DESCRIPTOR_HPP__
#define __OPENCV_DESCRIPTOR_HPP__

#include <map>
#include <vector>
#include <list>

#if defined _MSC_VER && _MSC_VER <= 1700
#include <stdint.h>
#else
#include <inttypes.h>
#endif

#include <stdio.h>
#include <iostream>

#include "opencv2/core/utility.hpp"
#include <opencv2/imgproc.hpp>
#include "opencv2/core.hpp"

/* define data types */
typedef uint64_t UINT64;
typedef uint32_t UINT32;
typedef uint16_t UINT16;
typedef uint8_t UINT8;

/* define constants */
#define UINT64_1 ((UINT64)0x01)
#define UINT32_1 ((UINT32)0x01)

namespace cv
{
namespace line_descriptor
{

//! @addtogroup line_descriptor
//! @{

/** @brief A class to represent a line

As aformentioned, it is been necessary to design a class that fully stores the information needed to
characterize completely a line and plot it on image it was extracted from, when required.

*KeyLine* class has been created for such goal; it is mainly inspired to Feature2d's KeyPoint class,
since KeyLine shares some of *KeyPoint*'s fields, even if a part of them assumes a different
meaning, when speaking about lines. In particular:

-   the *class_id* field is used to gather lines extracted from different octaves which refer to
    same line inside original image (such lines and the one they represent in original image share
    the same *class_id* value)
-   the *angle* field represents line's slope with respect to (positive) X axis
-   the *pt* field represents line's midpoint
-   the *response* field is computed as the ratio between the line's length and maximum between
    image's width and height
-   the *size* field is the area of the smallest rectangle containing line

Apart from fields inspired to KeyPoint class, KeyLines stores information about extremes of line in
original image and in octave it was extracted from, about line's length and number of pixels it
covers.
 */
struct CV_EXPORTS_W_SIMPLE KeyLine
{
 public:
  /** orientation of the line */
  CV_PROP_RW float angle;

  /** object ID, that can be used to cluster keylines by the line they represent */
  CV_PROP_RW int class_id;

  /** octave (pyramid layer), from which the keyline has been extracted */
  CV_PROP_RW int octave;

  /** coordinates of the middlepoint */
  CV_PROP_RW Point2f pt;

  /** the response, by which the strongest keylines have been selected.
   It's represented by the ratio between line's length and maximum between
   image's width and height */
  CV_PROP_RW float response;

  /** minimum area containing line */
  CV_PROP_RW float size;

  /** lines's extremes in original image */
  CV_PROP_RW float startPointX;
  CV_PROP_RW float startPointY;
  CV_PROP_RW float endPointX;
  CV_PROP_RW float endPointY;

  /** line's extremes in image it was extracted from */
  CV_PROP_RW float sPointInOctaveX;
  CV_PROP_RW float sPointInOctaveY;
  CV_PROP_RW float ePointInOctaveX;
  CV_PROP_RW float ePointInOctaveY;

  /** the length of line */
  CV_PROP_RW float lineLength;

  /** number of pixels covered by the line */
  CV_PROP_RW int numOfPixels;

  /** Returns the start point of the line in the original image */
  CV_WRAP Point2f getStartPoint() const
  {
    return Point2f(startPointX, startPointY);
  }

  /** Returns the end point of the line in the original image */
  CV_WRAP Point2f getEndPoint() const
  {
    return Point2f(endPointX, endPointY);
  }

  /** Returns the start point of the line in the octave it was extracted from */
  CV_WRAP Point2f getStartPointInOctave() const
  {
    return Point2f(sPointInOctaveX, sPointInOctaveY);
  }

  /** Returns the end point of the line in the octave it was extracted from */
  CV_WRAP Point2f getEndPointInOctave() const
  {
    return Point2f(ePointInOctaveX, ePointInOctaveY);
  }

  /** constructor */
  CV_WRAP KeyLine()
  {
  }
};

/** @brief Class implements both functionalities for detection of lines and computation of their
binary descriptor.

Class' interface is mainly based on the ones of classical detectors and extractors, such as
Feature2d's @ref features2d_main and @ref features2d_match. Retrieved information about lines is
stored in line_descriptor::KeyLine objects.
 */
class CV_EXPORTS_W BinaryDescriptor : public Algorithm
{

 public:
  /** @brief List of BinaryDescriptor parameters:
  */
  struct CV_EXPORTS Params
  {
    /*CV_WRAP*/
    Params();

    /** the number of image octaves (default = 1) */

    int numOfOctave_;

    /** the width of band; (default: 7) */

    int widthOfBand_;

    /** image's reduction ratio in construction of Gaussian pyramids */
    int reductionRatio;

    int ksize_;

    /** read parameters from a FileNode object and store them (struct function) */
    void read( const FileNode& fn );

    /** store parameters to a FileStorage object (struct function) */
    void write( FileStorage& fs ) const;

  };

  /** @brief Constructor

  @param parameters configuration parameters BinaryDescriptor::Params

  If no argument is provided, constructor sets default values (see comments in the code snippet in
  previous section). Default values are strongly recommended.
  */
  BinaryDescriptor( const BinaryDescriptor::Params &parameters = BinaryDescriptor::Params() );

  /** @brief Create a BinaryDescriptor object with default parameters (or with the ones provided)
  and return a smart pointer to it
     */
  CV_WRAP static Ptr<BinaryDescriptor> createBinaryDescriptor();
  static Ptr<BinaryDescriptor> createBinaryDescriptor( Params parameters );

  /** destructor */
  ~BinaryDescriptor();

  /** @brief Get current number of octaves
  */
  CV_WRAP int getNumOfOctaves();
  /** @brief Set number of octaves
    @param octaves number of octaves
     */
  CV_WRAP void setNumOfOctaves( int octaves );
  /** @brief Get current width of bands
    */
  CV_WRAP int getWidthOfBand();
  /** @brief Set width of bands
    @param width width of bands
    */
  CV_WRAP void setWidthOfBand( int width );
  /** @brief Get current reduction ratio (used in Gaussian pyramids)
    */
  CV_WRAP int getReductionRatio();
  /** @brief Set reduction ratio (used in Gaussian pyramids)
    @param rRatio reduction ratio
     */
  CV_WRAP void setReductionRatio( int rRatio );

  /** @brief Read parameters from a FileNode object and store them

    @param fn source FileNode file
     */
  virtual void read( const cv::FileNode& fn ) CV_OVERRIDE;

  /** @brief Store parameters to a FileStorage object

    @param fs output FileStorage file
     */
  virtual void write( cv::FileStorage& fs ) const CV_OVERRIDE;

  /** @brief Requires line detection

    @param image input image
    @param keypoints vector that will store extracted lines for one or more images
    @param mask mask matrix to detect only KeyLines of interest
     */
  CV_WRAP void detect( const Mat& image, CV_OUT std::vector<KeyLine>& keypoints, const Mat& mask = Mat() );

  /** @overload

    @param images input images
    @param keylines set of vectors that will store extracted lines for one or more images
    @param masks vector of mask matrices to detect only KeyLines of interest from each input image
     */
  void detect( const std::vector<Mat>& images, std::vector<std::vector<KeyLine> >& keylines, const std::vector<Mat>& masks =
                   std::vector<Mat>() ) const;

  /** @brief Requires descriptors computation

    @param image input image
    @param keylines vector containing lines for which descriptors must be computed
    @param descriptors
    @param returnFloatDescr flag (when set to true, original non-binary descriptors are returned)
     */
  CV_WRAP void compute( const Mat& image, CV_OUT CV_IN_OUT std::vector<KeyLine>& keylines, CV_OUT Mat& descriptors, bool returnFloatDescr = false ) const;

  /** @overload

    @param images input images
    @param keylines set of vectors containing lines for which descriptors must be computed
    @param descriptors
    @param returnFloatDescr flag (when set to true, original non-binary descriptors are returned)
     */
  void compute( const std::vector<Mat>& images, std::vector<std::vector<KeyLine> >& keylines, std::vector<Mat>& descriptors, bool returnFloatDescr =
                    false ) const;

  /** @brief Return descriptor size
   */
  int descriptorSize() const;

  /** @brief Return data type
   */
  int descriptorType() const;

  /** returns norm mode */
  /*CV_WRAP*/
  int defaultNorm() const;

  /** @brief Define operator '()' to perform detection of KeyLines and computation of descriptors in a row.

    @param image input image
    @param mask mask matrix to select which lines in KeyLines must be accepted among the ones
    extracted (used when *keylines* is not empty)
    @param keylines vector that contains input lines (when filled, the detection part will be skipped
    and input lines will be passed as input to the algorithm computing descriptors)
    @param descriptors matrix that will store final descriptors
    @param useProvidedKeyLines flag (when set to true, detection phase will be skipped and only
    computation of descriptors will be executed, using lines provided in *keylines*)
    @param returnFloatDescr flag (when set to true, original non-binary descriptors are returned)
     */
  virtual void operator()( InputArray image, InputArray mask, CV_OUT std::vector<KeyLine>& keylines, OutputArray descriptors,
                           bool useProvidedKeyLines = false, bool returnFloatDescr = false ) const;

 protected:
  /** implementation of line detection */
  virtual void detectImpl( const Mat& imageSrc, std::vector<KeyLine>& keylines, const Mat& mask = Mat() ) const;

  /** implementation of descriptors' computation */
  virtual void computeImpl( const Mat& imageSrc, std::vector<KeyLine>& keylines, Mat& descriptors, bool returnFloatDescr,
                            bool useDetectionData ) const;

 private:
  /** struct to represent lines extracted from an octave */
  struct OctaveLine
  {
    unsigned int octaveCount;  //the octave which this line is detected
    unsigned int lineIDInOctave;  //the line ID in that octave image
    unsigned int lineIDInScaleLineVec;  //the line ID in Scale line vector
    float lineLength;  //the length of line in original image scale
  };

  // A 2D line (normal equation parameters).
  struct SingleLine
  {
    //note: rho and theta are based on coordinate origin, i.e. the top-left corner of image
    double rho;  //unit: pixel length
    double theta;  //unit: rad
    double linePointX;  // = rho * cos(theta);
    double linePointY;  // = rho * sin(theta);
    //for EndPoints, the coordinate origin is the top-left corner of image.
    double startPointX;
    double startPointY;
    double endPointX;
    double endPointY;
    //direction of a line, the angle between positive line direction (dark side is in the left) and positive X axis.
    double direction;
    //mean gradient magnitude
    double gradientMagnitude;
    //mean gray value of pixels in dark side of line
    double darkSideGrayValue;
    //mean gray value of pixels in light side of line
    double lightSideGrayValue;
    //the length of line
    double lineLength;
    //the width of line;
    double width;
    //number of pixels
    int numOfPixels;
    //the decriptor of line
    std::vector<double> descriptor;
  };

  // Specifies a vector of lines.
  typedef std::vector<SingleLine> Lines_list;

  struct OctaveSingleLine
  {
    /*endPoints, the coordinate origin is the top-left corner of the original image.
     *startPointX = sPointInOctaveX * (factor)^octaveCount; */
    float startPointX;
    float startPointY;
    float endPointX;
    float endPointY;
    //endPoints, the coordinate origin is the top-left corner of the octave image.
    float sPointInOctaveX;
    float sPointInOctaveY;
    float ePointInOctaveX;
    float ePointInOctaveY;
    //direction of a line, the angle between positive line direction (dark side is in the left) and positive X axis.
    float direction;
    //the summation of gradient magnitudes of pixels on lines
    float salience;
    //the length of line
    float lineLength;
    //number of pixels
    unsigned int numOfPixels;
    //the octave which this line is detected
    unsigned int octaveCount;
    //the decriptor of line
    std::vector<float> descriptor;

    OctaveSingleLine() : startPointX(0), startPointY(0), endPointX(0), endPointY(0),
        sPointInOctaveX(0), sPointInOctaveY(0), ePointInOctaveX(0), ePointInOctaveY(0),
        direction(0), salience(0), lineLength(0), numOfPixels(0), octaveCount(0),
        descriptor(std::vector<float>())
    {}
  };

  struct Pixel
  {
    unsigned int x;  //X coordinate
    unsigned int y;  //Y coordinate
  };
  struct EdgeChains
  {
    std::vector<unsigned int> xCors;  //all the x coordinates of edge points
    std::vector<unsigned int> yCors;  //all the y coordinates of edge points
    std::vector<unsigned int> sId;  //the start index of each edge in the coordinate arrays
    unsigned int numOfEdges;  //the number of edges whose length are larger than minLineLen; numOfEdges < sId.size;
  };

  struct LineChains
  {
    std::vector<unsigned int> xCors;  //all the x coordinates of line points
    std::vector<unsigned int> yCors;  //all the y coordinates of line points
    std::vector<unsigned int> sId;  //the start index of each line in the coordinate arrays
    unsigned int numOfLines;  //the number of lines whose length are larger than minLineLen; numOfLines < sId.size;
  };

  typedef std::list<Pixel> PixelChain;  //each edge is a pixel chain

  struct CV_EXPORTS_W_SIMPLE EDLineParam
  {
    CV_PROP_RW int ksize;
    CV_PROP_RW float sigma;
    CV_PROP_RW float gradientThreshold;
    CV_PROP_RW float anchorThreshold;
    CV_PROP_RW int scanIntervals;
    CV_PROP_RW int minLineLen;
    CV_PROP_RW double lineFitErrThreshold;
  };

  #define RELATIVE_ERROR_FACTOR   100.0
  #define MLN10   2.30258509299404568402
  #define log_gamma(x)    ((x)>15.0?log_gamma_windschitl(x):log_gamma_lanczos(x))

  /** This class is used to detect lines from input image.
   * First, edges are extracted from input image following the method presented in Cihan Topal and
   * Cuneyt Akinlar's paper:"Edge Drawing: A Heuristic Approach to Robust Real-Time Edge Detection", 2010.
   * Then, lines are extracted from the edge image following the method presented in Cuneyt Akinlar and
   * Cihan Topal's paper:"EDLines: A real-time line segment detector with a false detection control", 2011
   * PS: The linking step of edge detection has a little bit difference with the Edge drawing algorithm
   *     described in the paper. The edge chain doesn't stop when the pixel direction is changed.
   */
  class CV_EXPORTS_W EDLineDetector
  {
   public:
    CV_WRAP EDLineDetector();
    CV_WRAP_AS(EDLineDetectorWithParams) EDLineDetector( EDLineParam param );
    ~EDLineDetector();

    /** @brief Creates an EDLineDetector object, using smart pointers.
     */
    CV_WRAP static Ptr<EDLineDetector> createEDLineDetector();


    CV_WRAP_AS(createEDLineDetectorWithParams) static Ptr<EDLineDetector> createEDLineDetector(EDLineParam params);
    /*extract edges from image
     *image:    In, gray image;
     *edges:    Out, store the edges, each edge is a pixel chain
     *return -1: error happen
     */
    int EdgeDrawing( cv::Mat &image, EdgeChains &edgeChains );

    /*extract lines from image
     *image:    In, gray image;
     *lines:    Out, store the extracted lines,
     *return -1: error happen
     */
    int EDline( cv::Mat &image, LineChains &lines );

    /** extract line from image, and store them */
    CV_WRAP int EDline( cv::Mat &image );

    cv::Mat dxImg_;  //store the dxImg;

    cv::Mat dyImg_;  //store the dyImg;

    cv::Mat gImgWO_;  //store the gradient image without threshold;

    LineChains lines_;  //store the detected line chains;

    //store the line Equation coefficients, vec3=[w1,w2,w3] for line w1*x + w2*y + w3=0;
    std::vector<std::vector<double> > lineEquations_;

    //store the line endpoints, [x1,y1,x2,y3]
    std::vector<std::vector<float> > lineEndpoints_;

    //store the line direction
    std::vector<float> lineDirection_;

    //store the line salience, which is the summation of gradients of pixels on line
    std::vector<float> lineSalience_;

    // image sizes
    unsigned int imageWidth;
    unsigned int imageHeight;

    /*The threshold of line fit error;
     *If lineFitErr is large than this threshold, then
     *the pixel chain is not accepted as a single line segment.*/
    double lineFitErrThreshold_;

    /*the threshold of pixel gradient magnitude.
     *Only those pixel whose gradient magnitude are larger than this threshold will be
     *taken as possible edge points. Default value is 36*/
    short gradienThreshold_;

    /*If the pixel's gradient value is bigger than both of its neighbors by a
     *certain threshold (ANCHOR_THRESHOLD), the pixel is marked to be an anchor.
     *Default value is 8*/
    unsigned char anchorThreshold_;

    /*anchor testing can be performed at different scan intervals, i.e.,
     *every row/column, every second row/column etc.
     *Default value is 2*/
    unsigned int scanIntervals_;

    int minLineLen_;  //minimal acceptable line length

   private:
    void InitEDLine_();

    /*For an input edge chain, find the best fit line, the default chain length is minLineLen_
     *xCors:  In, pointer to the X coordinates of pixel chain;
     *yCors:  In, pointer to the Y coordinates of pixel chain;
     *offsetS:In, start index of this chain in vector;
     *lineEquation: Out, [a,b] which are the coefficient of lines y=ax+b(horizontal) or x=ay+b(vertical);
     *return:  line fit error; -1:error happens;
     */
    double LeastSquaresLineFit_( unsigned int *xCors, unsigned int *yCors, unsigned int offsetS, std::vector<double> &lineEquation );

    /*For an input pixel chain, find the best fit line. Only do the update based on new points.
     *For A*x=v,  Least square estimation of x = Inv(A^T * A) * (A^T * v);
     *If some new observations are added, i.e, [A; A'] * x = [v; v'],
     *then x' = Inv(A^T * A + (A')^T * A') * (A^T * v + (A')^T * v');
     *xCors:  In, pointer to the X coordinates of pixel chain;
     *yCors:  In, pointer to the Y coordinates of pixel chain;
     *offsetS:In, start index of this chain in vector;
     *newOffsetS: In, start index of extended part;
     *offsetE:In, end index of this chain in vector;
     *lineEquation: Out, [a,b] which are the coefficient of lines y=ax+b(horizontal) or x=ay+b(vertical);
     *return:  line fit error; -1:error happens;
     */
    double LeastSquaresLineFit_( unsigned int *xCors, unsigned int *yCors, unsigned int offsetS, unsigned int newOffsetS, unsigned int offsetE,
                                 std::vector<double> &lineEquation );

    /** Validate line based on the Helmholtz principle, which basically states that
     * for a structure to be perceptually meaningful, the expectation of this structure
     * by chance must be very low.
     */
    bool LineValidation_( unsigned int *xCors, unsigned int *yCors, unsigned int offsetS, unsigned int offsetE, std::vector<double> &lineEquation,
                          float &direction );

    bool bValidate_;  //flag to decide whether line will be validated

    int ksize_;  //the size of Gaussian kernel: ksize X ksize, default value is 5.

    float sigma_;  //the sigma of Gaussian kernal, default value is 1.0.

    /*For example, there two edges in the image:
     *edge1 = [(7,4), (8,5), (9,6),| (10,7)|, (11, 8), (12,9)] and
     *edge2 = [(14,9), (15,10), (16,11), (17,12),| (18, 13)|, (19,14)] ; then we store them as following:
     *pFirstPartEdgeX_ = [10, 11, 12, 18, 19];//store the first part of each edge[from middle to end]
     *pFirstPartEdgeY_ = [7,  8,  9,  13, 14];
     *pFirstPartEdgeS_ = [0,3,5];// the index of start point of first part of each edge
     *pSecondPartEdgeX_ = [10, 9, 8, 7, 18, 17, 16, 15, 14];//store the second part of each edge[from middle to front]
     *pSecondPartEdgeY_ = [7,  6, 5, 4, 13, 12, 11, 10, 9];//anchor points(10, 7) and (18, 13) are stored again
     *pSecondPartEdgeS_ = [0, 4, 9];// the index of start point of second part of each edge
     *This type of storage order is because of the order of edge detection process.
     *For each edge, start from one anchor point, first go right, then go left or first go down, then go up*/

    //store the X coordinates of the first part of the pixels for chains
    unsigned int *pFirstPartEdgeX_;

    //store the Y coordinates of the first part of the pixels for chains
    unsigned int *pFirstPartEdgeY_;

    //store the start index of every edge chain in the first part arrays
    unsigned int *pFirstPartEdgeS_;

    //store the X coordinates of the second part of the pixels for chains
    unsigned int *pSecondPartEdgeX_;

    //store the Y coordinates of the second part of the pixels for chains
    unsigned int *pSecondPartEdgeY_;

    //store the start index of every edge chain in the second part arrays
    unsigned int *pSecondPartEdgeS_;

    //store the X coordinates of anchors
    unsigned int *pAnchorX_;

    //store the Y coordinates of anchors
    unsigned int *pAnchorY_;

    //edges
    cv::Mat edgeImage_;

    cv::Mat gImg_;  //store the gradient image;

    cv::Mat dirImg_;  //store the direction image

    double logNT_;

    cv::Mat_<float> ATA;   //the previous matrix of A^T * A;

    cv::Mat_<float> ATV;    //the previous vector of A^T * V;

    cv::Mat_<float> fitMatT;   //the matrix used in line fit function;

    cv::Mat_<float> fitVec;    //the vector used in line fit function;

    cv::Mat_<float> tempMatLineFit;  //the matrix used in line fit function;

    cv::Mat_<float> tempVecLineFit;    //the vector used in line fit function;

};

  // Specifies a vector of lines.
typedef std::vector<OctaveSingleLine> LinesVec;

// each element in ScaleLines is a vector of lines
// which corresponds the same line detected in different octave images.
typedef std::vector<LinesVec> ScaleLines;

/* compute Gaussian pyramids */
void computeGaussianPyramid( const Mat& image, const int numOctaves );

/* compute Sobel's derivatives */
void computeSobel( const Mat& image, const int numOctaves );

/* conversion of an LBD descriptor to its binary representation */
unsigned char binaryConversion( float* f1, float* f2 );

/* compute LBD descriptors using EDLine extractor */
int computeLBD( ScaleLines &keyLines, bool useDetectionData = false );

/* gathers lines in groups using EDLine extractor.
 Each group contains the same line, detected in different octaves */
int OctaveKeyLines( cv::Mat& image, ScaleLines &keyLines );

/* the local gaussian coefficient applied to the orthogonal line direction within each band */
std::vector<double> gaussCoefL_;

/* the global gaussian coefficient applied to each row within line support region */
std::vector<double> gaussCoefG_;

/* descriptor parameters */
Params params;

/* vector of sizes of downsampled and blurred images */
std::vector<cv::Size> images_sizes;

/*For each octave of image, we define an EDLineDetector, because we can get gradient images (dxImg, dyImg, gImg)
 *from the EDLineDetector class without extra computation cost. Another reason is that, if we use
 *a single EDLineDetector to detect lines in different octave of images, then we need to allocate and release
 *memory for gradient images (dxImg, dyImg, gImg) repeatedly for their varying size*/
std::vector<Ptr<EDLineDetector> > edLineVec_;

/* Sobel's derivatives */
std::vector<cv::Mat> dxImg_vector, dyImg_vector;

/* Gaussian pyramid */
std::vector<cv::Mat> octaveImages;

};

/**
Lines extraction methodology
----------------------------

The lines extraction methodology described in the following is mainly based on @cite EDL . The
extraction starts with a Gaussian pyramid generated from an original image, downsampled N-1 times,
blurred N times, to obtain N layers (one for each octave), with layer 0 corresponding to input
image. Then, from each layer (octave) in the pyramid, lines are extracted using LSD algorithm.

Differently from EDLine lines extractor used in original article, LSD furnishes information only
about lines extremes; thus, additional information regarding slope and equation of line are computed
via analytic methods. The number of pixels is obtained using *LineIterator*. Extracted lines are
returned in the form of KeyLine objects, but since extraction is based on a method different from
the one used in *BinaryDescriptor* class, data associated to a line's extremes in original image and
in octave it was extracted from, coincide. KeyLine's field *class_id* is used as an index to
indicate the order of extraction of a line inside a single octave.
*/
struct CV_EXPORTS_W_SIMPLE LSDParam
{
  CV_PROP_RW double scale ;
  CV_PROP_RW double sigma_scale;
  CV_PROP_RW double quant;
  CV_PROP_RW double ang_th;
  CV_PROP_RW double log_eps;
  CV_PROP_RW double density_th ;
  CV_PROP_RW int n_bins ;


CV_WRAP LSDParam():scale(0.8),
                   sigma_scale(0.6),
                   quant(2.0),
                   ang_th(22.5),
                   log_eps(0),
                   density_th(0.7),
                   n_bins(1024){}

};

class CV_EXPORTS_W LSDDetector : public Algorithm
{
public:

/* constructor */
CV_WRAP LSDDetector() : params()
{
}
;

CV_WRAP_AS(LSDDetectorWithParams) LSDDetector(LSDParam _params) : params(_params)
{
}
;

/** @brief Creates ad LSDDetector object, using smart pointers.
 */
CV_WRAP static Ptr<LSDDetector> createLSDDetector();


CV_WRAP_AS(createLSDDetectorWithParams) static Ptr<LSDDetector> createLSDDetector(LSDParam params);


/** @brief Detect lines inside an image.

@param image input image
@param keypoints vector that will store extracted lines for one or more images
@param scale scale factor used in pyramids generation
@param numOctaves number of octaves inside pyramid
@param mask mask matrix to detect only KeyLines of interest
 */
CV_WRAP void detect( const Mat& image, CV_OUT std::vector<KeyLine>& keypoints, int scale, int numOctaves, const Mat& mask = Mat() );

/** @overload
@param images input images
@param keylines set of vectors that will store extracted lines for one or more images
@param scale scale factor used in pyramids generation
@param numOctaves number of octaves inside pyramid
@param masks vector of mask matrices to detect only KeyLines of interest from each input image
*/
CV_WRAP void detect( const std::vector<Mat>& images, std::vector<std::vector<KeyLine> >& keylines, int scale, int numOctaves,
const std::vector<Mat>& masks = std::vector<Mat>() ) const;

private:
/* compute Gaussian pyramid of input image */
void computeGaussianPyramid( const Mat& image, int numOctaves, int scale );

/* implementation of line detection */
void detectImpl( const Mat& imageSrc, std::vector<KeyLine>& keylines, int numOctaves, int scale, const Mat& mask ) const;

/* matrices for Gaussian pyramids */
std::vector<cv::Mat> gaussianPyrs;

/* parameters */
LSDParam params;
};

/** @brief furnishes all functionalities for querying a dataset provided by user or internal to
class (that user must, anyway, populate) on the model of @ref features2d_match


Once descriptors have been extracted from an image (both they represent lines and points), it
becomes interesting to be able to match a descriptor with another one extracted from a different
image and representing the same line or point, seen from a differente perspective or on a different
scale. In reaching such goal, the main headache is designing an efficient search algorithm to
associate a query descriptor to one extracted from a dataset. In the following, a matching modality
based on *Multi-Index Hashing (MiHashing)* will be described.

Multi-Index Hashing
-------------------

The theory described in this section is based on @cite MIH . Given a dataset populated with binary
codes, each code is indexed *m* times into *m* different hash tables, according to *m* substrings it
has been divided into. Thus, given a query code, all the entries close to it at least in one
substring are returned by search as *neighbor candidates*. Returned entries are then checked for
validity by verifying that their full codes are not distant (in Hamming space) more than *r* bits
from query code. In details, each binary code **h** composed of *b* bits is divided into *m*
disjoint substrings \f$\mathbf{h}^{(1)}, ..., \mathbf{h}^{(m)}\f$, each with length
\f$\lfloor b/m \rfloor\f$ or \f$\lceil b/m \rceil\f$ bits. Formally, when two codes **h** and **g** differ
by at the most *r* bits, in at the least one of their *m* substrings they differ by at the most
\f$\lfloor r/m \rfloor\f$ bits. In particular, when \f$||\mathbf{h}-\mathbf{g}||_H \le r\f$ (where \f$||.||_H\f$
is the Hamming norm), there must exist a substring *k* (with \f$1 \le k \le m\f$) such that

\f[||\mathbf{h}^{(k)} - \mathbf{g}^{(k)}||_H \le \left\lfloor \frac{r}{m} \right\rfloor .\f]

That means that if Hamming distance between each of the *m* substring is strictly greater than
\f$\lfloor r/m \rfloor\f$, then \f$||\mathbf{h}-\mathbf{g}||_H\f$ must be larger that *r* and that is a
contradiction. If the codes in dataset are divided into *m* substrings, then *m* tables will be
built. Given a query **q** with substrings \f$\{\mathbf{q}^{(i)}\}^m_{i=1}\f$, *i*-th hash table is
searched for entries distant at the most \f$\lfloor r/m \rfloor\f$ from \f$\mathbf{q}^{(i)}\f$ and a set of
candidates \f$\mathcal{N}_i(\mathbf{q})\f$ is obtained. The union of sets
\f$\mathcal{N}(\mathbf{q}) = \bigcup_i \mathcal{N}_i(\mathbf{q})\f$ is a superset of the *r*-neighbors
of **q**. Then, last step of algorithm is computing the Hamming distance between **q** and each
element in \f$\mathcal{N}(\mathbf{q})\f$, deleting the codes that are distant more that *r* from **q**.
*/
class CV_EXPORTS_W BinaryDescriptorMatcher : public Algorithm
{

public:
/** @brief For every input query descriptor, retrieve the best matching one from a dataset provided from user
or from the one internal to class

@param queryDescriptors query descriptors
@param trainDescriptors dataset of descriptors furnished by user
@param matches vector to host retrieved matches
@param mask mask to select which input descriptors must be matched to one in dataset
 */
CV_WRAP void match( const Mat& queryDescriptors, const Mat& trainDescriptors, CV_OUT std::vector<DMatch>& matches, const Mat& mask = Mat() ) const;

/** @overload
@param queryDescriptors query descriptors
@param matches vector to host retrieved matches
@param masks vector of masks to select which input descriptors must be matched to one in dataset
(the *i*-th mask in vector indicates whether each input query can be matched with descriptors in
dataset relative to *i*-th image)
*/
CV_WRAP_AS(matchQuery) void match( const Mat& queryDescriptors, CV_OUT std::vector<DMatch>& matches, const std::vector<Mat>& masks = std::vector<Mat>() );

/** @brief For every input query descriptor, retrieve the best *k* matching ones from a dataset provided from
user or from the one internal to class

@param queryDescriptors query descriptors
@param trainDescriptors dataset of descriptors furnished by user
@param matches vector to host retrieved matches
@param k number of the closest descriptors to be returned for every input query
@param mask mask to select which input descriptors must be matched to ones in dataset
@param compactResult flag to obtain a compact result (if true, a vector that doesn't contain any
matches for a given query is not inserted in final result)
 */
CV_WRAP void knnMatch( const Mat& queryDescriptors, const Mat& trainDescriptors, CV_OUT std::vector<std::vector<DMatch> >& matches, int k, const Mat& mask = Mat(),
bool compactResult = false ) const;

/** @overload
@param queryDescriptors query descriptors
@param matches vector to host retrieved matches
@param k number of the closest descriptors to be returned for every input query
@param masks vector of masks to select which input descriptors must be matched to ones in dataset
(the *i*-th mask in vector indicates whether each input query can be matched with descriptors in
dataset relative to *i*-th image)
@param compactResult flag to obtain a compact result (if true, a vector that doesn't contain any
matches for a given query is not inserted in final result)
*/
CV_WRAP_AS(knnMatchQuery) void knnMatch( const Mat& queryDescriptors, std::vector<std::vector<DMatch> >& matches, int k, const std::vector<Mat>& masks = std::vector<Mat>(),
bool compactResult = false );

/** @brief For every input query descriptor, retrieve, from a dataset provided from user or from the one
internal to class, all the descriptors that are not further than *maxDist* from input query

@param queryDescriptors query descriptors
@param trainDescriptors dataset of descriptors furnished by user
@param matches vector to host retrieved matches
@param maxDistance search radius
@param mask mask to select which input descriptors must be matched to ones in dataset
@param compactResult flag to obtain a compact result (if true, a vector that doesn't contain any
matches for a given query is not inserted in final result)
 */
void radiusMatch( const Mat& queryDescriptors, const Mat& trainDescriptors, std::vector<std::vector<DMatch> >& matches, float maxDistance,
const Mat& mask = Mat(), bool compactResult = false ) const;

/** @overload
@param queryDescriptors query descriptors
@param matches vector to host retrieved matches
@param maxDistance search radius
@param masks vector of masks to select which input descriptors must be matched to ones in dataset
(the *i*-th mask in vector indicates whether each input query can be matched with descriptors in
dataset relative to *i*-th image)
@param compactResult flag to obtain a compact result (if true, a vector that doesn't contain any
matches for a given query is not inserted in final result)
*/
void radiusMatch( const Mat& queryDescriptors, std::vector<std::vector<DMatch> >& matches, float maxDistance, const std::vector<Mat>& masks =
std::vector<Mat>(),
bool compactResult = false );

/** @brief Store locally new descriptors to be inserted in dataset, without updating dataset.

@param descriptors matrices containing descriptors to be inserted into dataset

@note Each matrix *i* in **descriptors** should contain descriptors relative to lines extracted from
*i*-th image.
 */
void add( const std::vector<Mat>& descriptors );

/** @brief Update dataset by inserting into it all descriptors that were stored locally by *add* function.

@note Every time this function is invoked, current dataset is deleted and locally stored descriptors
are inserted into dataset. The locally stored copy of just inserted descriptors is then removed.
 */
void train();

/** @brief Create a BinaryDescriptorMatcher object and return a smart pointer to it.
 */
static Ptr<BinaryDescriptorMatcher> createBinaryDescriptorMatcher();

/** @brief Clear dataset and internal data
 */
void clear() CV_OVERRIDE;

/** @brief Constructor.

The BinaryDescriptorMatcher constructed is able to store and manage 256-bits long entries.
 */
CV_WRAP BinaryDescriptorMatcher();

/** destructor */
~BinaryDescriptorMatcher()
{
}

private:
class BucketGroup
{

public:
/** constructor */
BucketGroup(bool needAllocateGroup = true);

/** destructor */
~BucketGroup();

/** insert data into the bucket */
void insert( int subindex, UINT32 data );

/** perform a query to the bucket */
UINT32* query( int subindex, int *size );

/** utility functions */
void insert_value( std::vector<uint32_t>& vec, int index, UINT32 data );
void push_value( std::vector<uint32_t>& vec, UINT32 Data );

/** data fields */
UINT32 empty;
std::vector<uint32_t> group;


};

class SparseHashtable
{

private:

/** Maximum bits per key before folding the table */
static const int MAX_B;

/** Bins (each bin is an Array object for duplicates of the same key) */
std::vector<BucketGroup> table;

public:

/** constructor */
SparseHashtable();

/** destructor */
~SparseHashtable();

/** initializer */
int init( int _b );

/** insert data */
void insert( UINT64 index, UINT32 data );

/** query data */
UINT32* query( UINT64 index, int* size );

/** Bits per index */
int b;

/**  Number of bins */
UINT64 size;

};

/** class defining a sequence of bits */
class bitarray
{

public:
/** pointer to bits sequence and sequence's length */
UINT32 *arr;
UINT32 length;

/** constructor setting default values */
bitarray()
{
arr = NULL;
length = 0;
}

/** constructor setting sequence's length */
bitarray( UINT64 _bits )
{
arr = NULL;
init( _bits );
}

/** initializer of private fields */
void init( UINT64 _bits )
{
if( arr )
delete[] arr;
length = (UINT32) ceil( _bits / 32.00 );
arr = new UINT32[length];
erase();
}

/** destructor */
~bitarray()
{
if( arr )
delete[] arr;
}

inline void flip( UINT64 index )
{
arr[index >> 5] ^= ( (UINT32) 0x01 ) << ( index % 32 );
}

inline void set( UINT64 index )
{
arr[index >> 5] |= ( (UINT32) 0x01 ) << ( index % 32 );
}

inline UINT8 get( UINT64 index )
{
return ( arr[index >> 5] & ( ( (UINT32) 0x01 ) << ( index % 32 ) ) ) != 0;
}

/** reserve menory for an UINT32 */
inline void erase()
{
memset( arr, 0, sizeof(UINT32) * length );
}

};

class Mihasher
{

public:
/** Bits per code */
int B;

/** B/8 */
int B_over_8;

/** Bits per chunk (must be less than 64) */
int b;

/** Number of chunks */
int m;

/** Number of chunks with b bits (have 1 bit more than others) */
int mplus;

/** Maximum hamming search radius (we use B/2 by default) */
int D;

/** Maximum hamming search radius per substring */
int d;

/** Maximum results to return */
int K;

/** Number of codes */
UINT64 N;

/** Table of original full-length codes */
cv::Mat codes;

/** Counter for eliminating duplicate results (it is not thread safe) */
Ptr<bitarray> counter;

/** Array of m hashtables */
std::vector<SparseHashtable> H;

/** Volume of a b-bit Hamming ball with radius s (for s = 0 to d) */
std::vector<UINT32> xornum;

/** Used within generation of binary codes at a certain Hamming distance */
int power[100];

/** constructor */
Mihasher();

/** desctructor */
~Mihasher();

/** constructor 2 */
Mihasher( int B, int m );

/** K setter */
void setK( int K );

/** populate tables */
void populate( cv::Mat & codes, UINT32 N, int dim1codes );

/** execute a batch query */
void batchquery( UINT32 * results, UINT32 *numres/*, qstat *stats*/, const cv::Mat & q, UINT32 numq, int dim1queries );

private:

/** execute a single query */
void query( UINT32 * results, UINT32* numres/*, qstat *stats*/, UINT8 *q, UINT64 * chunks, UINT32 * res );
};

/** retrieve Hamming distances */
void checkKDistances( UINT32 * numres, int k, std::vector<int>& k_distances, int row, int string_length ) const;

/** matrix to store new descriptors */
Mat descriptorsMat;

/** map storing where each bunch of descriptors benins in DS */
std::map<int, int> indexesMap;

/** internal MiHaser representing dataset */
Ptr<Mihasher> dataset;

/** index from which next added descriptors' bunch must begin */
int nextAddedIndex;

/** number of images whose descriptors are stored in DS */
int numImages;

/** number of descriptors in dataset */
int descrInDS;

};

/* --------------------------------------------------------------------------------------------
 UTILITY FUNCTIONS
 -------------------------------------------------------------------------------------------- */

/** struct for drawing options */
struct CV_EXPORTS_W_SIMPLE DrawLinesMatchesFlags
{
CV_PROP_RW enum
{
DEFAULT = 0,  //!< Output image matrix will be created (Mat::create),
              //!< i.e. existing memory of output image may be reused.
              //!< Two source images, matches, and single keylines
              //!< will be drawn.
DRAW_OVER_OUTIMG = 1,//!< Output image matrix will not be
//!< created (using Mat::create). Matches will be drawn
//!< on existing content of output image.
NOT_DRAW_SINGLE_LINES = 2//!< Single keylines will not be drawn.
};
};

/** @brief Draws the found matches of keylines from two images.

@param img1 first image
@param keylines1 keylines extracted from first image
@param img2 second image
@param keylines2 keylines extracted from second image
@param matches1to2 vector of matches
@param outImg output matrix to draw on
@param matchColor drawing color for matches (chosen randomly in case of default value)
@param singleLineColor drawing color for keylines (chosen randomly in case of default value)
@param matchesMask mask to indicate which matches must be drawn
@param flags drawing flags, see DrawLinesMatchesFlags

@note If both *matchColor* and *singleLineColor* are set to their default values, function draws
matched lines and line connecting them with same color
 */
CV_EXPORTS_W void drawLineMatches( const Mat& img1, const std::vector<KeyLine>& keylines1, const Mat& img2, const std::vector<KeyLine>& keylines2,
                                   const std::vector<DMatch>& matches1to2, CV_OUT Mat& outImg, const Scalar& matchColor = Scalar::all( -1 ),
                                   const Scalar& singleLineColor = Scalar::all( -1 ), const std::vector<char>& matchesMask = std::vector<char>(),
                                   int flags = DrawLinesMatchesFlags::DEFAULT );

/** @brief Draws keylines.

@param image input image
@param keylines keylines to be drawn
@param outImage output image to draw on
@param color color of lines to be drawn (if set to defaul value, color is chosen randomly)
@param flags drawing flags
 */
CV_EXPORTS_W void drawKeylines( const Mat& image, const std::vector<KeyLine>& keylines, CV_OUT Mat& outImage, const Scalar& color = Scalar::all( -1 ),
                                int flags = DrawLinesMatchesFlags::DEFAULT );

//! @}

}
}

#endif
