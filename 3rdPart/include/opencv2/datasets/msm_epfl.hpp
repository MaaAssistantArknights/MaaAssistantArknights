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
// Copyright (C) 2014, Itseez Inc, all rights reserved.
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
// In no event shall the Itseez Inc or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef OPENCV_DATASETS_MSM_EPFL_HPP
#define OPENCV_DATASETS_MSM_EPFL_HPP

#include <string>
#include <vector>

#include "opencv2/datasets/dataset.hpp"

#include <opencv2/core.hpp>

namespace cv
{
namespace datasets
{

//! @addtogroup datasets_msm
//! @{

struct cameraParam
{
    Matx33d mat1;
    double mat2[3];
    Matx33d mat3;
    double mat4[3];
    int imageWidth, imageHeight;
};

struct MSM_epflObj : public Object
{
    std::string imageName;
    Matx23d bounding;
    Matx34d p;
    cameraParam camera;
};

class CV_EXPORTS MSM_epfl : public Dataset
{
public:
    virtual void load(const std::string &path) CV_OVERRIDE = 0;

    static Ptr<MSM_epfl> create();
};

//! @}

}
}

#endif
