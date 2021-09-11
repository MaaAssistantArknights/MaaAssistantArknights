/*
By downloading, copying, installing or using the software you agree to this
license. If you do not agree to this license, do not download, install,
copy or use the software.


                          License Agreement
               For Open Source Computer Vision Library
                       (3-clause BSD License)

Copyright (C) 2016, OpenCV Foundation, all rights reserved.
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

/**
 * @file   sparse_matching_gpc.hpp
 * @author Vladislav Samsonov <vvladxx@gmail.com>
 * @brief  Implementation of the Global Patch Collider.
 *
 * Implementation of the Global Patch Collider algorithm from the following paper:
 * http://research.microsoft.com/en-us/um/people/pkohli/papers/wfrik_cvpr2016.pdf
 *
 * @cite Wang_2016_CVPR
 */

#ifndef __OPENCV_OPTFLOW_SPARSE_MATCHING_GPC_HPP__
#define __OPENCV_OPTFLOW_SPARSE_MATCHING_GPC_HPP__

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"

namespace cv
{
namespace optflow
{

//! @addtogroup optflow
//! @{

struct CV_EXPORTS_W GPCPatchDescriptor
{
  static const unsigned nFeatures = 18; //!< number of features in a patch descriptor
  Vec< double, nFeatures > feature;

  double dot( const Vec< double, nFeatures > &coef ) const;

  void markAsSeparated() { feature[0] = std::numeric_limits< double >::quiet_NaN(); }

  bool isSeparated() const { return cvIsNaN( feature[0] ) != 0; }
};

struct CV_EXPORTS_W GPCPatchSample
{
  GPCPatchDescriptor ref;
  GPCPatchDescriptor pos;
  GPCPatchDescriptor neg;

  void getDirections( bool &refdir, bool &posdir, bool &negdir, const Vec< double, GPCPatchDescriptor::nFeatures > &coef, double rhs ) const;
};

typedef std::vector< GPCPatchSample > GPCSamplesVector;

/** @brief Descriptor types for the Global Patch Collider.
 */
enum GPCDescType
{
  GPC_DESCRIPTOR_DCT = 0, //!< Better quality but slow
  GPC_DESCRIPTOR_WHT      //!< Worse quality but much faster
};

/** @brief Class encapsulating training samples.
 */
class CV_EXPORTS_W GPCTrainingSamples
{
private:
  GPCSamplesVector samples;
  int descriptorType;

public:
  /** @brief This function can be used to extract samples from a pair of images and a ground truth flow.
   * Sizes of all the provided vectors must be equal.
   */
  static Ptr< GPCTrainingSamples > create( const std::vector< String > &imagesFrom, const std::vector< String > &imagesTo,
                                           const std::vector< String > &gt, int descriptorType );

  static Ptr< GPCTrainingSamples > create( InputArrayOfArrays imagesFrom, InputArrayOfArrays imagesTo, InputArrayOfArrays gt,
                                           int descriptorType );

  size_t size() const { return samples.size(); }

  int type() const { return descriptorType; }

  operator GPCSamplesVector &() { return samples; }
};

/** @brief Class encapsulating training parameters.
 */
struct GPCTrainingParams
{
  unsigned maxTreeDepth;  //!< Maximum tree depth to stop partitioning.
  int minNumberOfSamples; //!< Minimum number of samples in the node to stop partitioning.
  int descriptorType;     //!< Type of descriptors to use.
  bool printProgress;     //!< Print progress to stdout.

  GPCTrainingParams( unsigned _maxTreeDepth = 20, int _minNumberOfSamples = 3, GPCDescType _descriptorType = GPC_DESCRIPTOR_DCT,
                     bool _printProgress = true )
      : maxTreeDepth( _maxTreeDepth ), minNumberOfSamples( _minNumberOfSamples ), descriptorType( _descriptorType ),
        printProgress( _printProgress )
  {
    CV_Assert( check() );
  }

  bool check() const { return maxTreeDepth > 1 && minNumberOfSamples > 1; }
};

/** @brief Class encapsulating matching parameters.
 */
struct GPCMatchingParams
{
  bool useOpenCL; //!< Whether to use OpenCL to speed up the matching.

  GPCMatchingParams( bool _useOpenCL = false ) : useOpenCL( _useOpenCL ) {}

  GPCMatchingParams( const GPCMatchingParams &params ) : useOpenCL( params.useOpenCL ) {}
};

/** @brief Class for individual tree.
 */
class CV_EXPORTS_W GPCTree : public Algorithm
{
public:
  struct Node
  {
    Vec< double, GPCPatchDescriptor::nFeatures > coef; //!< Hyperplane coefficients
    double rhs;                                        //!< Bias term of the hyperplane
    unsigned left;
    unsigned right;

    bool operator==( const Node &n ) const { return coef == n.coef && rhs == n.rhs && left == n.left && right == n.right; }
  };

private:
  typedef GPCSamplesVector::iterator SIter;

  std::vector< Node > nodes;
  GPCTrainingParams params;

  bool trainNode( size_t nodeId, SIter begin, SIter end, unsigned depth );

public:
  void train( GPCTrainingSamples &samples, const GPCTrainingParams params = GPCTrainingParams() );

  void write( FileStorage &fs ) const CV_OVERRIDE;

  void read( const FileNode &fn ) CV_OVERRIDE;

  unsigned findLeafForPatch( const GPCPatchDescriptor &descr ) const;

  static Ptr< GPCTree > create() { return makePtr< GPCTree >(); }

  bool operator==( const GPCTree &t ) const { return nodes == t.nodes; }

  int getDescriptorType() const { return params.descriptorType; }
};

template < int T > class GPCForest : public Algorithm
{
private:
  struct Trail
  {
    unsigned leaf[T]; //!< Inside which leaf of the tree 0..T the patch fell?
    Point2i coord;    //!< Patch coordinates.

    bool operator==( const Trail &trail ) const { return memcmp( leaf, trail.leaf, sizeof( leaf ) ) == 0; }

    bool operator<( const Trail &trail ) const
    {
      for ( int i = 0; i < T - 1; ++i )
        if ( leaf[i] != trail.leaf[i] )
          return leaf[i] < trail.leaf[i];
      return leaf[T - 1] < trail.leaf[T - 1];
    }
  };

  class ParallelTrailsFilling : public ParallelLoopBody
  {
  private:
    const GPCForest *forest;
    const std::vector< GPCPatchDescriptor > *descr;
    std::vector< Trail > *trails;

    ParallelTrailsFilling &operator=( const ParallelTrailsFilling & );

  public:
    ParallelTrailsFilling( const GPCForest *_forest, const std::vector< GPCPatchDescriptor > *_descr, std::vector< Trail > *_trails )
        : forest( _forest ), descr( _descr ), trails( _trails ){};

    void operator()( const Range &range ) const CV_OVERRIDE
    {
      for ( int t = range.start; t < range.end; ++t )
        for ( size_t i = 0; i < descr->size(); ++i )
          trails->at( i ).leaf[t] = forest->tree[t].findLeafForPatch( descr->at( i ) );
    }
  };

  GPCTree tree[T];

public:
  /** @brief Train the forest using one sample set for every tree.
   * Please, consider using the next method instead of this one for better quality.
   */
  void train( GPCTrainingSamples &samples, const GPCTrainingParams params = GPCTrainingParams() )
  {
    for ( int i = 0; i < T; ++i )
      tree[i].train( samples, params );
  }

  /** @brief Train the forest using individual samples for each tree.
   * It is generally better to use this instead of the first method.
   */
  void train( const std::vector< String > &imagesFrom, const std::vector< String > &imagesTo, const std::vector< String > &gt,
              const GPCTrainingParams params = GPCTrainingParams() )
  {
    for ( int i = 0; i < T; ++i )
    {
      Ptr< GPCTrainingSamples > samples =
        GPCTrainingSamples::create( imagesFrom, imagesTo, gt, params.descriptorType ); // Create training set for the tree
      tree[i].train( *samples, params );
    }
  }

  void train( InputArrayOfArrays imagesFrom, InputArrayOfArrays imagesTo, InputArrayOfArrays gt,
              const GPCTrainingParams params = GPCTrainingParams() )
  {
    for ( int i = 0; i < T; ++i )
    {
      Ptr< GPCTrainingSamples > samples =
        GPCTrainingSamples::create( imagesFrom, imagesTo, gt, params.descriptorType ); // Create training set for the tree
      tree[i].train( *samples, params );
    }
  }

  void write( FileStorage &fs ) const CV_OVERRIDE
  {
    fs << "ntrees" << T << "trees"
       << "[";
    for ( int i = 0; i < T; ++i )
    {
      fs << "{";
      tree[i].write( fs );
      fs << "}";
    }
    fs << "]";
  }

  void read( const FileNode &fn ) CV_OVERRIDE
  {
    CV_Assert( T <= (int)fn["ntrees"] );
    FileNodeIterator it = fn["trees"].begin();
    for ( int i = 0; i < T; ++i, ++it )
      tree[i].read( *it );
  }

  /** @brief Find correspondences between two images.
   * @param[in] imgFrom First image in a sequence.
   * @param[in] imgTo Second image in a sequence.
   * @param[out] corr Output vector with pairs of corresponding points.
   * @param[in] params Additional matching parameters for fine-tuning.
   */
  void findCorrespondences( InputArray imgFrom, InputArray imgTo, std::vector< std::pair< Point2i, Point2i > > &corr,
                            const GPCMatchingParams params = GPCMatchingParams() ) const;

  static Ptr< GPCForest > create() { return makePtr< GPCForest >(); }
};

class CV_EXPORTS_W GPCDetails
{
public:
  static void dropOutliers( std::vector< std::pair< Point2i, Point2i > > &corr );

  static void getAllDescriptorsForImage( const Mat *imgCh, std::vector< GPCPatchDescriptor > &descr, const GPCMatchingParams &mp,
                                         int type );

  static void getCoordinatesFromIndex( size_t index, Size sz, int &x, int &y );
};

template < int T >
void GPCForest< T >::findCorrespondences( InputArray imgFrom, InputArray imgTo, std::vector< std::pair< Point2i, Point2i > > &corr,
                                          const GPCMatchingParams params ) const
{
  CV_Assert( imgFrom.channels() == 3 );
  CV_Assert( imgTo.channels() == 3 );

  Mat from, to;
  imgFrom.getMat().convertTo( from, CV_32FC3 );
  imgTo.getMat().convertTo( to, CV_32FC3 );
  cvtColor( from, from, COLOR_BGR2YCrCb );
  cvtColor( to, to, COLOR_BGR2YCrCb );

  Mat fromCh[3], toCh[3];
  split( from, fromCh );
  split( to, toCh );

  std::vector< GPCPatchDescriptor > descr;
  GPCDetails::getAllDescriptorsForImage( fromCh, descr, params, tree[0].getDescriptorType() );
  std::vector< Trail > trailsFrom( descr.size() ), trailsTo( descr.size() );

  for ( size_t i = 0; i < descr.size(); ++i )
    GPCDetails::getCoordinatesFromIndex( i, from.size(), trailsFrom[i].coord.x, trailsFrom[i].coord.y );
  parallel_for_( Range( 0, T ), ParallelTrailsFilling( this, &descr, &trailsFrom ) );

  descr.clear();
  GPCDetails::getAllDescriptorsForImage( toCh, descr, params, tree[0].getDescriptorType() );

  for ( size_t i = 0; i < descr.size(); ++i )
    GPCDetails::getCoordinatesFromIndex( i, to.size(), trailsTo[i].coord.x, trailsTo[i].coord.y );
  parallel_for_( Range( 0, T ), ParallelTrailsFilling( this, &descr, &trailsTo ) );

  std::sort( trailsFrom.begin(), trailsFrom.end() );
  std::sort( trailsTo.begin(), trailsTo.end() );

  for ( size_t i = 0; i < trailsFrom.size(); ++i )
  {
    bool uniq = true;
    while ( i + 1 < trailsFrom.size() && trailsFrom[i] == trailsFrom[i + 1] )
      ++i, uniq = false;
    if ( uniq )
    {
      typename std::vector< Trail >::const_iterator lb = std::lower_bound( trailsTo.begin(), trailsTo.end(), trailsFrom[i] );
      if ( lb != trailsTo.end() && *lb == trailsFrom[i] && ( ( lb + 1 ) == trailsTo.end() || !( *lb == *( lb + 1 ) ) ) )
        corr.push_back( std::make_pair( trailsFrom[i].coord, lb->coord ) );
    }
  }

  GPCDetails::dropOutliers( corr );
}

//! @}

} // namespace optflow

CV_EXPORTS void write( FileStorage &fs, const String &name, const optflow::GPCTree::Node &node );

CV_EXPORTS void read( const FileNode &fn, optflow::GPCTree::Node &node, optflow::GPCTree::Node );
} // namespace cv

#endif
