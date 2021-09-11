// This file is part of the OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#ifndef __mace_h_onboard__
#define __mace_h_onboard__

#include "opencv2/core.hpp"


namespace cv {
namespace face {

//! @addtogroup face
//! @{


/**
@brief Minimum Average Correlation Energy Filter
    useful for authentication with (cancellable) biometrical features.
    (does not need many positives to train (10-50), and no negatives at all, also robust to noise/salting)

    see also: @cite Savvides04

    this implementation is largely based on: https://code.google.com/archive/p/pam-face-authentication (GSOC 2009)

    use it like:
    @code

    Ptr<face::MACE> mace = face::MACE::create(64);

    vector<Mat> pos_images = ...
    mace->train(pos_images);

    Mat query = ...
    bool same = mace->same(query);

    @endcode

    you can also use two-factor authentication, with an additional passphrase:

    @code
    String owners_passphrase = "ilikehotdogs";
    Ptr<face::MACE> mace = face::MACE::create(64);
    mace->salt(owners_passphrase);
    vector<Mat> pos_images = ...
    mace->train(pos_images);

    // now, users have to give a valid passphrase, along with the image:
    Mat query = ...
    cout << "enter passphrase: ";
    string pass;
    getline(cin, pass);
    mace->salt(pass);
    bool same = mace->same(query);
    @endcode

    save/load your model:
    @code
    Ptr<face::MACE> mace = face::MACE::create(64);
    mace->train(pos_images);
    mace->save("my_mace.xml");

    // later:
    Ptr<MACE> reloaded = MACE::load("my_mace.xml");
    reloaded->same(some_image);
    @endcode

*/

class CV_EXPORTS_W MACE : public cv::Algorithm
{
public:
    /**
    @brief optionally encrypt images with random convolution
    @param passphrase a crc64 random seed will get generated from this
    */
    CV_WRAP virtual void salt(const cv::String &passphrase) = 0;

    /**
    @brief train it on positive features
       compute the mace filter: `h = D(-1) * X * (X(+) * D(-1) * X)(-1) * C`
       also calculate a minimal threshold for this class, the smallest self-similarity from the train images
    @param images  a vector<Mat> with the train images
    */
    CV_WRAP virtual void train(cv::InputArrayOfArrays images) = 0;

    /**
    @brief correlate query img and threshold to min class value
    @param query  a Mat with query image
    */
    CV_WRAP virtual bool same(cv::InputArray query) const = 0;


    /**
    @brief constructor
    @param filename  build a new MACE instance from a pre-serialized FileStorage
    @param objname (optional) top-level node in the FileStorage
    */
    CV_WRAP static cv::Ptr<MACE> load(const String &filename, const String &objname=String());

    /**
    @brief constructor
    @param IMGSIZE  images will get resized to this (should be an even number)
    */
    CV_WRAP static cv::Ptr<MACE> create(int IMGSIZE=64);
};

//! @}

}/* namespace face */
}/* namespace cv */

#endif // __mace_h_onboard__
