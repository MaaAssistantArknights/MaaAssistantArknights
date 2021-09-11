/*
By downloading, copying, installing or using the software you agree to this
license. If you do not agree to this license, do not download, install,
copy or use the software.
                          License Agreement
               For Open Source Computer Vision Library
                       (3-clause BSD License)
Copyright (C) 2013, OpenCV Foundation, all rights reserved.
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

This file was part of GSoC Project: Facemark API for OpenCV
Final report: https://gist.github.com/kurnianggoro/74de9121e122ad0bd825176751d47ecc
Student: Laksono Kurnianggoro
Mentor: Delia Passalacqua
*/

#ifndef __OPENCV_FACEMARK_LBF_HPP__
#define __OPENCV_FACEMARK_LBF_HPP__

#include "opencv2/face/facemark_train.hpp"

namespace cv {
namespace face {

//! @addtogroup face
//! @{

class CV_EXPORTS_W FacemarkLBF : public FacemarkTrain
{
public:
    struct CV_EXPORTS Params
    {
        /**
        * \brief Constructor
        */
        Params();

        double shape_offset;
        //!<  offset for the loaded face landmark points
        String cascade_face;
        //!<  filename of the face detector model
        bool verbose;
        //!< show the training print-out

        int n_landmarks;
        //!<  number of landmark points
        int initShape_n;
        //!<  multiplier for augment the training data

        int stages_n;
        //!<  number of refinement stages
        int tree_n;
        //!<  number of tree in the model for each landmark point refinement
        int tree_depth;
        //!<  the depth of decision tree, defines the size of feature
        double bagging_overlap;
        //!<  overlap ratio for training the LBF feature

        std::string model_filename;
        //!<  filename where the trained model will be saved
        bool save_model; //!< flag to save the trained model or not
        unsigned int seed; //!< seed for shuffling the training data

        std::vector<int> feats_m;
        std::vector<double> radius_m;
        std::vector<int> pupils[2];
        //!<  index of facemark points on pupils of left and right eye

        Rect detectROI;

        void read(const FileNode& /*fn*/);
        void write(FileStorage& /*fs*/) const;

    };

    class BBox {
    public:
        BBox();
        ~BBox();
        BBox(double x, double y, double w, double h);

        Mat project(const Mat &shape) const;
        Mat reproject(const Mat &shape) const;

        double x, y;
        double x_center, y_center;
        double x_scale, y_scale;
        double width, height;
    };

    static Ptr<FacemarkLBF> create(const FacemarkLBF::Params &parameters = FacemarkLBF::Params() );
    virtual ~FacemarkLBF(){};
}; /* LBF */

//! @}

} /* namespace face */
}/* namespace cv */

#endif
