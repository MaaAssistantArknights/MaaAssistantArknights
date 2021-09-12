// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.
#ifndef __OPENCV_FACE_ALIGNMENT_HPP__
#define __OPENCV_FACE_ALIGNMENT_HPP__

#include "opencv2/face/facemark_train.hpp"

namespace cv{
namespace face{
class CV_EXPORTS_W FacemarkKazemi : public Facemark
{
public:
    struct CV_EXPORTS Params
    {
        /**
        * \brief Constructor
        */
        Params();
        /// cascade_depth This stores the deapth of cascade used for training.
        unsigned long cascade_depth;
        /// tree_depth This stores the max height of the regression tree built.
        unsigned long tree_depth;
        /// num_trees_per_cascade_level This stores number of trees fit per cascade level.
        unsigned long num_trees_per_cascade_level;
        /// learning_rate stores the learning rate in gradient boosting, also referred as shrinkage.
        float learning_rate;
        /// oversampling_amount stores number of initialisations used to create training samples.
        unsigned long oversampling_amount;
        /// num_test_coordinates stores number of test coordinates.
        unsigned long num_test_coordinates;
        /// lambda stores a value to calculate probability of closeness of two coordinates.
        float lambda;
        /// num_test_splits stores number of random test splits generated.
        unsigned long num_test_splits;
        /// configfile stores the name of the file containing the values of training parameters
        String configfile;
    };
    static Ptr<FacemarkKazemi> create(const FacemarkKazemi::Params &parameters = FacemarkKazemi::Params());
    virtual ~FacemarkKazemi();

    /** @brief This function is used to train the model using gradient boosting to get a cascade of regressors
    *which can then be used to predict shape.
    *@param images A vector of type cv::Mat which stores the images which are used in training samples.
    *@param landmarks A vector of vectors of type cv::Point2f which stores the landmarks detected in a particular image.
    *@param scale A size of type cv::Size to which all images and landmarks have to be scaled to.
    *@param configfile A variable of type std::string which stores the name of the file storing parameters for training the model.
    *@param modelFilename A variable of type std::string which stores the name of the trained model file that has to be saved.
    *@returns A boolean value. The function returns true if the model is trained properly or false if it is not trained.
    */
    virtual bool training(std::vector<Mat>& images, std::vector< std::vector<Point2f> >& landmarks,std::string configfile,Size scale,std::string modelFilename = "face_landmarks.dat")=0;

    /// set the custom face detector
    virtual bool setFaceDetector(bool(*f)(InputArray , OutputArray, void*), void* userData)=0;
    /// get faces using the custom detector
    virtual bool getFaces(InputArray image, OutputArray faces)=0;
};

}} // namespace
#endif
