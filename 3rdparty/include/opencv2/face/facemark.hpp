// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

/*
This file was part of GSoC Project: Facemark API for OpenCV
Final report: https://gist.github.com/kurnianggoro/74de9121e122ad0bd825176751d47ecc
Student: Laksono Kurnianggoro
Mentor: Delia Passalacqua
*/

#ifndef __OPENCV_FACELANDMARK_HPP__
#define __OPENCV_FACELANDMARK_HPP__

/**
@defgroup face Face Analysis
- @ref tutorial_table_of_content_facemark
- The Facemark API
*/

#include "opencv2/core.hpp"
#include <vector>


namespace cv {
namespace face {


/** @brief Abstract base class for all facemark models

To utilize this API in your program, please take a look at the @ref tutorial_table_of_content_facemark
### Description

Facemark is a base class which provides universal access to any specific facemark algorithm.
Therefore, the users should declare a desired algorithm before they can use it in their application.

Here is an example on how to declare a facemark algorithm:
@code
// Using Facemark in your code:
Ptr<Facemark> facemark = createFacemarkLBF();
@endcode

The typical pipeline for facemark detection is as follows:
- Load the trained model using Facemark::loadModel.
- Perform the fitting on an image via Facemark::fit.
*/
class CV_EXPORTS_W Facemark : public virtual Algorithm
{
public:

    /** @brief A function to load the trained model before the fitting process.
    @param model A string represent the filename of a trained model.

    <B>Example of usage</B>
    @code
    facemark->loadModel("../data/lbf.model");
    @endcode
    */
    CV_WRAP virtual void loadModel( String model ) = 0;
    // virtual void saveModel(String fs)=0;

    /** @brief Detect facial landmarks from an image.
    @param image Input image.
    @param faces Output of the function which represent region of interest of the detected faces.
    Each face is stored in cv::Rect container.
    @param landmarks The detected landmark points for each faces.

    <B>Example of usage</B>
    @code
    Mat image = imread("image.jpg");
    std::vector<Rect> faces;
    std::vector<std::vector<Point2f> > landmarks;
    facemark->fit(image, faces, landmarks);
    @endcode
    */
    CV_WRAP virtual bool fit( InputArray image,
                              InputArray faces,
                              OutputArrayOfArrays landmarks) = 0;
}; /* Facemark*/


//! construct an AAM facemark detector
CV_EXPORTS_W Ptr<Facemark> createFacemarkAAM();

//! construct an LBF facemark detector
CV_EXPORTS_W Ptr<Facemark> createFacemarkLBF();

//! construct a Kazemi facemark detector
CV_EXPORTS_W Ptr<Facemark> createFacemarkKazemi();


} // face
} // cv

#endif //__OPENCV_FACELANDMARK_HPP__
