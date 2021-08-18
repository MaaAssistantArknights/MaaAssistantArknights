// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.
#ifndef _OPENCV_DNN_OBJDETECT_CORE_DETECT_HPP_
#define _OPENCV_DNN_OBJDETECT_CORE_DETECT_HPP_

#include <vector>
#include <memory>

#include <opencv2/imgproc.hpp>

/** @defgroup dnn_objdetect DNN used for object detection
*/

namespace cv
{
namespace dnn_objdetect
{

    //! @addtogroup dnn_objdetect
    //! @{

    /** @brief Structure to hold the details pertaining to a single bounding box
     */
    typedef struct
    {
      int xmin, xmax;
      int ymin, ymax;
      size_t class_idx;
      std::string label_name;
      double class_prob;
    } object;


    /** @brief A class to post process model predictions
     */
    class CV_EXPORTS InferBbox
    {
      public:
        /** @brief Default constructer
        @param _delta_bbox Blob containing relative coordinates of bounding boxes
        @param _class_scores Blob containing the probability values of each class
        @param _conf_scores Blob containing the confidence scores
         */
        InferBbox(Mat _delta_bbox, Mat _class_scores, Mat _conf_scores);

        /** @brief Filters the bounding boxes.
         */
        void filter(double thresh =  0.8);

        /** @brief Vector which holds the final detections of the model
         */
        std::vector<object> detections;

      protected:
        /** @brief Transform relative coordinates from ConvDet to bounding box coordinates
        @param bboxes Vector to hold the predicted bounding boxes
         */
        void transform_bboxes(std::vector<std::vector<double> > *bboxes);

        /** @brief Computes final probability values of each bounding box
        @param final_probs Vector to hold the probability values
         */
        void final_probability_dist(std::vector<std::vector<double> > *final_probs);

        /** @brief Transform bounding boxes from [x, y, h, w] to [xmin, ymin, xmax, ymax]
        @param pre Vector conatining initial co-ordinates
        @param post Vector containing the transformed co-ordinates
         */
        void transform_bboxes_inv(std::vector<std::vector<double> > *pre,
                                  std::vector<std::vector<double> > *post);

        /** @brief Ensures that the bounding box values are within image boundaries
        @param min_max_boxes Vector containing bounding boxes of the form [xmin, ymin, xmax, ymax]
         */
        void assert_predictions(std::vector<std::vector<double> > *min_max_boxes);

        /** @brief Filter top `n` predictions
        @param probs Final probability values of bounding boxes
        @param boxes Predicted bounding box co-ordinates
        @param top_n_boxes Contains bounding box co-ordinates of top `n` boxes
        @param top_n_idxs Containes class indices of top `n` bounding boxes
        @param top_n_probs Contains probability values of top `n` bounding boxes
         */
        void filter_top_n(std::vector<std::vector<double> > *probs,
                          std::vector<std::vector<double> > *boxes,
                          std::vector<std::vector<double> > &top_n_boxes,
                          std::vector<size_t> &top_n_idxs,
                          std::vector<double> &top_n_probs);

        /** @brief Wrapper to apply Non-Maximal Supression
        @param top_n_boxes Contains bounding box co-ordinates of top `n` boxes
        @param top_n_idxs Containes class indices of top `n` bounding boxes
        @param top_n_probs Contains probability values of top `n` bounding boxes
         */
        void nms_wrapper(std::vector<std::vector<double> > &top_n_boxes,
                         std::vector<size_t> &top_n_idxs,
                         std::vector<double> &top_n_probs);

       /** @brief Applies Non-Maximal Supression
       @param boxes Bounding box co-ordinates belonging to one class
       @param probs Probability values of boxes belonging to one class
        */
        std::vector<bool> non_maximal_suppression(std::vector<std::vector<double> >
                                         *boxes, std::vector<double> *probs);

       /** @brief Computes intersection over union of bounding boxes
       @param boxes Vector of bounding box co-ordinates
       @param base_box Base box wrt which IOU is calculated
       @param iou Vector to store IOU values
        */
        void intersection_over_union(std::vector<std::vector<double> > *boxes,
                          std::vector<double> *base_box, std::vector<double> *iou);

        static inline bool comparator (std::pair<double, size_t> l1,
            std::pair<double, size_t> l2)
        {
          return l1.first > l2.first;
        }

      private:
        Mat delta_bbox;
        Mat class_scores;
        Mat conf_scores;

        unsigned int image_width;
        unsigned int image_height;

        unsigned int W, H;
        std::vector<std::vector<double> > anchors_values;
        std::vector<std::pair<double, double> > anchor_center;
        std::vector<std::pair<double, double> > anchor_shapes;

        std::vector<std::string> label_map;

        unsigned int num_classes;
        unsigned int anchors_per_grid;
        size_t anchors;
        double intersection_thresh;
        double nms_intersection_thresh;
        size_t n_top_detections;
        double epsilon;
    };

    //! @}
} // namespace dnn_objdetect
} // namespace cv
#endif
