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

/**
 * @file
 *
 * @brief  Implementation of ICP (Iterative Closest Point) Algorithm
 * @author Tolga Birdal <tbirdal AT gmail.com>
 */

#ifndef __OPENCV_SURFACE_MATCHING_ICP_HPP__
#define __OPENCV_SURFACE_MATCHING_ICP_HPP__

#include <opencv2/core.hpp>

#include "pose_3d.hpp"
#include <vector>

namespace cv
{
namespace ppf_match_3d
{

//! @addtogroup surface_matching
//! @{

/**
* @brief This class implements a very efficient and robust variant of the iterative closest point (ICP) algorithm.
* The task is to register a 3D model (or point cloud) against a set of noisy target data. The variants are put together
* by myself after certain tests. The task is to be able to match partial, noisy point clouds in cluttered scenes, quickly.
* You will find that my emphasis is on the performance, while retaining the accuracy.
* This implementation is based on Tolga Birdal's MATLAB implementation in here:
* http://www.mathworks.com/matlabcentral/fileexchange/47152-icp-registration-using-efficient-variants-and-multi-resolution-scheme
* The main contributions come from:
* 1. Picky ICP:
* http://www5.informatik.uni-erlangen.de/Forschung/Publikationen/2003/Zinsser03-ARI.pdf
* 2. Efficient variants of the ICP Algorithm:
* http://docs.happycoders.org/orgadoc/graphics/imaging/fasticp_paper.pdf
* 3. Geometrically Stable Sampling for the ICP Algorithm: https://graphics.stanford.edu/papers/stabicp/stabicp.pdf
* 4. Multi-resolution registration:
* http://www.cvl.iis.u-tokyo.ac.jp/~oishi/Papers/Alignment/Jost_MultiResolutionICP_3DIM03.pdf
* 5. Linearization of Point-to-Plane metric by Kok Lim Low:
* https://www.comp.nus.edu.sg/~lowkl/publications/lowk_point-to-plane_icp_techrep.pdf
*/
class CV_EXPORTS_W ICP
{
public:

  CV_WRAP enum
  {
    ICP_SAMPLING_TYPE_UNIFORM = 0,
    ICP_SAMPLING_TYPE_GELFAND = 1
  };

  CV_WRAP ICP()
  {
    m_tolerance = 0.005f;
    m_rejectionScale = 2.5f;
    m_maxIterations = 250;
    m_numLevels = 6;
    m_sampleType = ICP_SAMPLING_TYPE_UNIFORM;
    m_numNeighborsCorr = 1;
  }

  virtual ~ICP() { }

  /**
     *  \brief ICP constructor with default arguments.
     *  @param [in] iterations
     *  @param [in] tolerence Controls the accuracy of registration at each iteration of ICP.
     *  @param [in] rejectionScale Robust outlier rejection is applied for robustness. This value
            actually corresponds to the standard deviation coefficient. Points with
            rejectionScale * &sigma are ignored during registration.
     *  @param [in] numLevels Number of pyramid levels to proceed. Deep pyramids increase speed but
            decrease accuracy. Too coarse pyramids might have computational overhead on top of the
            inaccurate registrtaion. This parameter should be chosen to optimize a balance. Typical
            values range from 4 to 10.
     *  @param [in] sampleType Currently this parameter is ignored and only uniform sampling is
            applied. Leave it as 0.
     *  @param [in] numMaxCorr Currently this parameter is ignored and only PickyICP is applied. Leave it as 1.
     */
  CV_WRAP ICP(const int iterations, const float tolerence = 0.05f, const float rejectionScale = 2.5f, const int numLevels = 6, const int sampleType = ICP::ICP_SAMPLING_TYPE_UNIFORM, const int numMaxCorr = 1)
  {
    m_tolerance = tolerence;
    m_numNeighborsCorr = numMaxCorr;
    m_rejectionScale = rejectionScale;
    m_maxIterations = iterations;
    m_numLevels = numLevels;
    m_sampleType = sampleType;
  }

  /**
     *  \brief Perform registration
     *
     *  @param [in] srcPC The input point cloud for the model. Expected to have the normals (Nx6). Currently,
     *  CV_32F is the only supported data type.
     *  @param [in] dstPC The input point cloud for the scene. It is assumed that the model is registered on the scene. Scene remains static. Expected to have the normals (Nx6). Currently, CV_32F is the only supported data type.
     *  @param [out] residual The output registration error.
     *  @param [out] pose Transformation between srcPC and dstPC.
     *  \return On successful termination, the function returns 0.
     *
     *  \details It is assumed that the model is registered on the scene. Scene remains static, while the model transforms. The output poses transform the models onto the scene. Because of the point to plane minimization, the scene is expected to have the normals available. Expected to have the normals (Nx6).
     */
  CV_WRAP int registerModelToScene(const Mat& srcPC, const Mat& dstPC, CV_OUT double& residual, CV_OUT Matx44d& pose);

  /**
     *  \brief Perform registration with multiple initial poses
     *
     *  @param [in] srcPC The input point cloud for the model. Expected to have the normals (Nx6). Currently,
     *  CV_32F is the only supported data type.
     *  @param [in] dstPC The input point cloud for the scene. Currently, CV_32F is the only supported data type.
     *  @param [in,out] poses Input poses to start with but also list output of poses.
     *  \return On successful termination, the function returns 0.
     *
     *  \details It is assumed that the model is registered on the scene. Scene remains static, while the model transforms. The output poses transform the models onto the scene. Because of the point to plane minimization, the scene is expected to have the normals available. Expected to have the normals (Nx6).
     */
  int registerModelToScene(const Mat& srcPC, const Mat& dstPC, std::vector<Pose3DPtr>& poses);

private:
  float m_tolerance;
  int m_maxIterations;
  float m_rejectionScale;
  int m_numNeighborsCorr;
  int m_numLevels;
  int m_sampleType;

};

//! @}

} // namespace ppf_match_3d

} // namespace cv

#endif
