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

#ifndef __OPENCV_FACEMARK_AAM_HPP__
#define __OPENCV_FACEMARK_AAM_HPP__

#include "opencv2/face/facemark_train.hpp"
namespace cv {
namespace face {

//! @addtogroup face
//! @{

class CV_EXPORTS_W FacemarkAAM : public FacemarkTrain
{
public:
    struct CV_EXPORTS Params
    {
        /**
        * \brief Constructor
        */
        Params();

        /**
        * \brief Read parameters from file, currently unused
        */
        void read(const FileNode& /*fn*/);

        /**
        * \brief Read parameters from file, currently unused
        */
        void write(FileStorage& /*fs*/) const;

        std::string model_filename;
        int m;
        int n;
        int n_iter;
        bool verbose;
        bool save_model;
        int max_m, max_n, texture_max_m;
        std::vector<float>scales;
    };

    /**
    * \brief Optional parameter for fitting process.
    */
    struct CV_EXPORTS Config
    {
        Config( Mat rot = Mat::eye(2,2,CV_32F),
                Point2f trans = Point2f(0.0f, 0.0f),
                float scaling = 1.0f,
                int scale_id=0
        );

        Mat R;
        Point2f t;
        float scale;
        int model_scale_idx;

    };

    /**
    * \brief Data container for the facemark::getData function
    */
    struct CV_EXPORTS Data
    {
        std::vector<Point2f> s0;
    };

    /**
    * \brief The model of AAM Algorithm
    */
    struct CV_EXPORTS Model
    {
        std::vector<float>scales;
        //!<  defines the scales considered to build the model

        /*warping*/
        std::vector<Vec3i> triangles;
        //!<  each element contains 3 values, represent index of facemarks that construct one triangle (obtained using delaunay triangulation)

        struct Texture{
            int max_m; //!<  unused delete
            Rect resolution;
            //!<  resolution of the current scale
            Mat A;
            //!<  gray values from all face region in the dataset, projected in PCA space
            Mat A0;
            //!<  average of gray values from all face region in the dataset
            Mat AA;
            //!<  gray values from all erorded face region in the dataset, projected in PCA space
            Mat AA0;
            //!<  average of gray values from all erorded face region in the dataset

            std::vector<std::vector<Point> > textureIdx;
            //!<  index for warping of each delaunay triangle region constructed by 3 facemarks
            std::vector<Point2f> base_shape;
            //!<  basic shape, normalized to be fit in an image with current detection resolution
            std::vector<int> ind1;
            //!<  index of pixels for mapping process to obtains the grays values of face region
            std::vector<int> ind2;
            //!<  index of pixels for mapping process to obtains the grays values of eroded face region
        };
        std::vector<Texture> textures;
        //!<  a container to holds the texture data for each scale of fitting

        /*shape*/
        std::vector<Point2f> s0;
        //!<  the basic shape obtained from training dataset
        Mat S,Q;
        //!<  the encoded shapes from training data

    };

    //! overload with additional Config structures
    virtual bool fitConfig( InputArray image, InputArray roi, OutputArrayOfArrays _landmarks, const std::vector<Config> &runtime_params ) = 0;


    //!  initializer
    static Ptr<FacemarkAAM> create(const FacemarkAAM::Params &parameters = FacemarkAAM::Params() );
    virtual ~FacemarkAAM() {}

}; /* AAM */

//! @}

} /* namespace face */
} /* namespace cv */
#endif
