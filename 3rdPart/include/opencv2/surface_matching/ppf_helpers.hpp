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
//

/** @file
@author Tolga Birdal <tbirdal AT gmail.com>
*/

#ifndef __OPENCV_SURFACE_MATCHING_HELPERS_HPP__
#define __OPENCV_SURFACE_MATCHING_HELPERS_HPP__

#include <opencv2/core.hpp>

namespace cv
{
namespace ppf_match_3d
{

//! @addtogroup surface_matching
//! @{

/**
 *  @brief Load a PLY file
 *  @param [in] fileName The PLY model to read
 *  @param [in] withNormals Flag wheather the input PLY contains normal information,
 *  and whether it should be loaded or not
 *  @return Returns the matrix on successful load
 */
CV_EXPORTS_W Mat loadPLYSimple(const char* fileName, int withNormals = 0);

/**
 *  @brief Write a point cloud to PLY file
 *  @param [in] PC Input point cloud
 *  @param [in] fileName The PLY model file to write
*/
CV_EXPORTS_W void writePLY(Mat PC, const char* fileName);

/**
*  @brief Used for debbuging pruposes, writes a point cloud to a PLY file with the tip
*  of the normal vectors as visible red points
*  @param [in] PC Input point cloud
*  @param [in] fileName The PLY model file to write
*/
CV_EXPORTS_W void writePLYVisibleNormals(Mat PC, const char* fileName);

Mat samplePCUniform(Mat PC, int sampleStep);
Mat samplePCUniformInd(Mat PC, int sampleStep, std::vector<int>& indices);

/**
 *  Sample a point cloud using uniform steps
 *  @param [in] pc Input point cloud
 *  @param [in] xrange X components (min and max) of the bounding box of the model
 *  @param [in] yrange Y components (min and max) of the bounding box of the model
 *  @param [in] zrange Z components (min and max) of the bounding box of the model
 *  @param [in] sample_step_relative The point cloud is sampled such that all points
 *  have a certain minimum distance. This minimum distance is determined relatively using
 *  the parameter sample_step_relative.
 *  @param [in] weightByCenter The contribution of the quantized data points can be weighted
 *  by the distance to the origin. This parameter enables/disables the use of weighting.
 *  @return Sampled point cloud
*/
CV_EXPORTS_W Mat samplePCByQuantization(Mat pc, Vec2f& xrange, Vec2f& yrange, Vec2f& zrange, float sample_step_relative, int weightByCenter=0);

void computeBboxStd(Mat pc, Vec2f& xRange, Vec2f& yRange, Vec2f& zRange);

void* indexPCFlann(Mat pc);
void destroyFlann(void* flannIndex);
void queryPCFlann(void* flannIndex, Mat& pc, Mat& indices, Mat& distances);
void queryPCFlann(void* flannIndex, Mat& pc, Mat& indices, Mat& distances, const int numNeighbors);

Mat normalizePCCoeff(Mat pc, float scale, float* Cx, float* Cy, float* Cz, float* MinVal, float* MaxVal);
Mat transPCCoeff(Mat pc, float scale, float Cx, float Cy, float Cz, float MinVal, float MaxVal);

/**
 *  Transforms the point cloud with a given a homogeneous 4x4 pose matrix (in double precision)
 *  @param [in] pc Input point cloud (CV_32F family). Point clouds with 3 or 6 elements per
 *  row are expected. In the case where the normals are provided, they are also rotated to be
 *  compatible with the entire transformation
 *  @param [in] Pose 4x4 pose matrix, but linearized in row-major form.
 *  @return Transformed point cloud
*/
CV_EXPORTS_W Mat transformPCPose(Mat pc, const Matx44d& Pose);

/**
 *  Generate a random 4x4 pose matrix
 *  @param [out] Pose The random pose
*/
CV_EXPORTS_W void getRandomPose(Matx44d& Pose);

/**
 *  Adds a uniform noise in the given scale to the input point cloud
 *  @param [in] pc Input point cloud (CV_32F family).
 *  @param [in] scale Input scale of the noise. The larger the scale, the more noisy the output
*/
CV_EXPORTS_W Mat addNoisePC(Mat pc, double scale);

/**
 *  @brief Compute the normals of an arbitrary point cloud
 *  computeNormalsPC3d uses a plane fitting approach to smoothly compute
 *  local normals. Normals are obtained through the eigenvector of the covariance
 *  matrix, corresponding to the smallest eigen value.
 *  If PCNormals is provided to be an Nx6 matrix, then no new allocation
 *  is made, instead the existing memory is overwritten.
 *  @param [in] PC Input point cloud to compute the normals for.
 *  @param [out] PCNormals Output point cloud
 *  @param [in] NumNeighbors Number of neighbors to take into account in a local region
 *  @param [in] FlipViewpoint Should normals be flipped to a viewing direction?
 *  @param [in] viewpoint
 *  @return Returns 0 on success
 */
CV_EXPORTS_W int computeNormalsPC3d(const Mat& PC, CV_OUT Mat& PCNormals, const int NumNeighbors, const bool FlipViewpoint, const Vec3f& viewpoint);

//! @}

} // namespace ppf_match_3d
} // namespace cv

#endif
