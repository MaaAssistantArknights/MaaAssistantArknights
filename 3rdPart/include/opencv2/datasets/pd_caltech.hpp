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

#ifndef OPENCV_DATASETS_PD_CALTECH_HPP
#define OPENCV_DATASETS_PD_CALTECH_HPP

#include <string>
#include <vector>

#include "opencv2/datasets/dataset.hpp"

#include <opencv2/core.hpp>

namespace cv
{
namespace datasets
{

//! @addtogroup datasets_pd
//! @{

struct PD_caltechObj : public Object
{
    //double groundTrue[][];
    //Mat image;
    std::string name;
    std::vector< std::string > imageNames;
};

//
// first version of Caltech Pedestrian dataset loading
// code to unpack all frames from seq files commented as their number is huge
// so currently load only meta information without data
//
// also ground truth isn't processed, as need to convert it from mat files first
//

class CV_EXPORTS PD_caltech : public Dataset
{
public:
    virtual void load(const std::string &path) CV_OVERRIDE = 0;

    static Ptr<PD_caltech> create();
};

//! @}

}
}

#endif
