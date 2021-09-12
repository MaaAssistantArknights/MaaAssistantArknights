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

#ifndef OPENCV_DATASETS_TRACK_VOT_HPP
#define OPENCV_DATASETS_TRACK_VOT_HPP

#include <string>
#include <vector>

#include "opencv2/datasets/dataset.hpp"
#include "opencv2/datasets/util.hpp"

using namespace std;

namespace cv
{
namespace datasets
{

//! @addtogroup datasets_track
//! @{

struct TRACK_votObj : public Object
{
    int id;
    std::string imagePath;
    vector <Point2d> gtbb;
};

class CV_EXPORTS TRACK_vot : public Dataset
{
public:
    static Ptr<TRACK_vot> create();

    virtual void load(const std::string &path) CV_OVERRIDE = 0;

    virtual int getDatasetsNum() = 0;

    virtual int getDatasetLength(int id) = 0;

    virtual bool initDataset(int id) = 0;

    virtual bool getNextFrame(Mat &frame) = 0;

    virtual vector <Point2d> getGT() = 0;

protected:
    vector <vector <Ptr<TRACK_votObj> > > data;
    int activeDatasetID;
    int frameCounter;
};

//! @}

}
}

#endif
