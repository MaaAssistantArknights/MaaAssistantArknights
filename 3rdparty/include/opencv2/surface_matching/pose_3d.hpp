//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                          License Agreement
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

/** @file
@author Tolga Birdal <tbirdal AT gmail.com>
*/

#ifndef __OPENCV_SURFACE_MATCHING_POSE3D_HPP__
#define __OPENCV_SURFACE_MATCHING_POSE3D_HPP__

#include "opencv2/core/cvstd.hpp" // cv::Ptr
#include <vector>
#include <string>

namespace cv
{
namespace ppf_match_3d
{

//! @addtogroup surface_matching
//! @{

class Pose3D;
typedef Ptr<Pose3D> Pose3DPtr;

class PoseCluster3D;
typedef Ptr<PoseCluster3D> PoseCluster3DPtr;

/**
* @brief Class, allowing the storage of a pose. The data structure stores both
* the quaternions and the matrix forms. It supports IO functionality together with
* various helper methods to work with poses
*
*/
class CV_EXPORTS_W Pose3D
{
public:
  Pose3D()
  {
    alpha=0;
    modelIndex=0;
    numVotes=0;
    residual = 0;

    pose = Matx44d::all(0);
  }

  Pose3D(double Alpha, size_t ModelIndex=0, size_t NumVotes=0)
  {
    alpha = Alpha;
    modelIndex = ModelIndex;
    numVotes = NumVotes;
    residual=0;

    pose = Matx44d::all(0);
  }

  /**
   *  \brief Updates the pose with the new one
   *  \param [in] NewPose New pose to overwrite
   */
  void updatePose(Matx44d& NewPose);

  /**
   *  \brief Updates the pose with the new one
   */
  void updatePose(Matx33d& NewR, Vec3d& NewT);

  /**
   *  \brief Updates the pose with the new one, but this time using quaternions to represent rotation
   */
  void updatePoseQuat(Vec4d& Q, Vec3d& NewT);

  /**
   *  \brief Left multiplies the existing pose in order to update the transformation
   *  \param [in] IncrementalPose New pose to apply
   */
  void appendPose(Matx44d& IncrementalPose);
  void printPose();

  Pose3DPtr clone();

  int writePose(FILE* f);
  int readPose(FILE* f);
  int writePose(const std::string& FileName);
  int readPose(const std::string& FileName);

  virtual ~Pose3D() {}

  double alpha, residual;
  size_t modelIndex, numVotes;
  Matx44d pose;
  double angle;
  Vec3d t;
  Vec4d q;
};

/**
* @brief When multiple poses (see Pose3D) are grouped together (contribute to the same transformation)
* pose clusters occur. This class is a general container for such groups of poses. It is possible to store,
* load and perform IO on these poses.
*/
class CV_EXPORTS_W PoseCluster3D
{
public:
  PoseCluster3D()
  {
    numVotes=0;
    id=0;
  }

  PoseCluster3D(Pose3DPtr newPose)
  {
    poseList.clear();
    poseList.push_back(newPose);
    numVotes=newPose->numVotes;
    id=0;
  }

  PoseCluster3D(Pose3DPtr newPose, int newId)
  {
    poseList.push_back(newPose);
    this->numVotes = newPose->numVotes;
    this->id = newId;
  }

  virtual ~PoseCluster3D()
  {}

  /**
   *  \brief Adds a new pose to the cluster. The pose should be "close" to the mean poses
   *  in order to preserve the consistency
   *  \param [in] newPose Pose to add to the cluster
   */
  void addPose(Pose3DPtr newPose);

  int writePoseCluster(FILE* f);
  int readPoseCluster(FILE* f);
  int writePoseCluster(const std::string& FileName);
  int readPoseCluster(const std::string& FileName);

  std::vector<Pose3DPtr> poseList;
  size_t numVotes;
  int id;
};

//! @}

} // namespace ppf_match_3d
} // namespace cv

#endif

