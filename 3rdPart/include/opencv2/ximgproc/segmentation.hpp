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
*/

#ifndef __OPENCV_XIMGPROC_SEGMENTATION_HPP__
#define __OPENCV_XIMGPROC_SEGMENTATION_HPP__

#include <opencv2/core.hpp>

namespace cv {
    namespace ximgproc {
        namespace segmentation {
            //! @addtogroup ximgproc_segmentation
            //! @{

                    /** @brief Graph Based Segmentation Algorithm.
                        The class implements the algorithm described in @cite PFF2004 .
                     */
                    class CV_EXPORTS_W GraphSegmentation : public Algorithm {
                        public:
                            /** @brief Segment an image and store output in dst
                                @param src The input image. Any number of channel (1 (Eg: Gray), 3 (Eg: RGB), 4 (Eg: RGB-D)) can be provided
                                @param dst The output segmentation. It's a CV_32SC1 Mat with the same number of cols and rows as input image, with an unique, sequential, id for each pixel.
                            */
                            CV_WRAP virtual void processImage(InputArray src, OutputArray dst) = 0;

                            CV_WRAP virtual void setSigma(double sigma) = 0;
                            CV_WRAP virtual double getSigma() = 0;

                            CV_WRAP virtual void setK(float k) = 0;
                            CV_WRAP virtual float getK() = 0;

                            CV_WRAP virtual void setMinSize(int min_size) = 0;
                            CV_WRAP virtual int getMinSize() = 0;
                    };

                    /** @brief Creates a graph based segmentor
                        @param sigma The sigma parameter, used to smooth image
                        @param k The k parameter of the algorythm
                        @param min_size The minimum size of segments
                     */
                    CV_EXPORTS_W Ptr<GraphSegmentation> createGraphSegmentation(double sigma=0.5, float k=300, int min_size=100);

                    /** @brief Strategie for the selective search segmentation algorithm
                        The class implements a generic stragery for the algorithm described in @cite uijlings2013selective.
                     */
                    class CV_EXPORTS_W SelectiveSearchSegmentationStrategy : public Algorithm {
                        public:
                            /** @brief Set a initial image, with a segmentation.
                                @param img The input image. Any number of channel can be provided
                                @param regions A segmentation of the image. The parameter must be the same size of img.
                                @param sizes The sizes of different regions
                                @param image_id If not set to -1, try to cache pre-computations. If the same set og (img, regions, size) is used, the image_id need to be the same.
                            */
                            CV_WRAP virtual void setImage(InputArray img, InputArray regions, InputArray sizes, int image_id = -1) = 0;

                            /** @brief Return the score between two regions (between 0 and 1)
                                @param r1 The first region
                                @param r2 The second region
                            */
                            CV_WRAP virtual float get(int r1, int r2) = 0;

                            /** @brief Inform the strategy that two regions will be merged
                                @param r1 The first region
                                @param r2 The second region
                            */
                            CV_WRAP virtual void merge(int r1, int r2) = 0;
                    };

                    /** @brief Color-based strategy for the selective search segmentation algorithm
                        The class is implemented from the algorithm described in @cite uijlings2013selective.
                     */
                    class CV_EXPORTS_W SelectiveSearchSegmentationStrategyColor : public SelectiveSearchSegmentationStrategy {
                    };

                    /** @brief Create a new color-based strategy */
                    CV_EXPORTS_W Ptr<SelectiveSearchSegmentationStrategyColor> createSelectiveSearchSegmentationStrategyColor();

                    /** @brief Size-based strategy for the selective search segmentation algorithm
                        The class is implemented from the algorithm described in @cite uijlings2013selective.
                     */
                    class CV_EXPORTS_W SelectiveSearchSegmentationStrategySize : public SelectiveSearchSegmentationStrategy {
                    };

                    /** @brief Create a new size-based strategy */
                    CV_EXPORTS_W Ptr<SelectiveSearchSegmentationStrategySize> createSelectiveSearchSegmentationStrategySize();

                    /** @brief Texture-based strategy for the selective search segmentation algorithm
                        The class is implemented from the algorithm described in @cite uijlings2013selective.
                     */
                    class CV_EXPORTS_W SelectiveSearchSegmentationStrategyTexture : public SelectiveSearchSegmentationStrategy {
                    };

                    /** @brief Create a new size-based strategy */
                    CV_EXPORTS_W Ptr<SelectiveSearchSegmentationStrategyTexture> createSelectiveSearchSegmentationStrategyTexture();

                    /** @brief Fill-based strategy for the selective search segmentation algorithm
                        The class is implemented from the algorithm described in @cite uijlings2013selective.
                     */
                    class CV_EXPORTS_W SelectiveSearchSegmentationStrategyFill : public SelectiveSearchSegmentationStrategy {
                    };

                    /** @brief Create a new fill-based strategy */
                    CV_EXPORTS_W Ptr<SelectiveSearchSegmentationStrategyFill> createSelectiveSearchSegmentationStrategyFill();

                    /** @brief Regroup multiple strategies for the selective search segmentation algorithm
                     */
                    class CV_EXPORTS_W SelectiveSearchSegmentationStrategyMultiple : public SelectiveSearchSegmentationStrategy {
                        public:

                            /** @brief Add a new sub-strategy
                                @param g The strategy
                                @param weight The weight of the strategy
                            */
                            CV_WRAP virtual void addStrategy(Ptr<SelectiveSearchSegmentationStrategy> g, float weight) = 0;
                            /** @brief Remove all sub-strategies
                            */
                            CV_WRAP virtual void clearStrategies() = 0;
                    };

                    /** @brief Create a new multiple strategy */
                    CV_EXPORTS_W Ptr<SelectiveSearchSegmentationStrategyMultiple> createSelectiveSearchSegmentationStrategyMultiple();

                    /** @brief Create a new multiple strategy and set one subtrategy
                        @param s1 The first strategy
                    */
                    CV_EXPORTS_W Ptr<SelectiveSearchSegmentationStrategyMultiple> createSelectiveSearchSegmentationStrategyMultiple(Ptr<SelectiveSearchSegmentationStrategy> s1);

                    /** @brief Create a new multiple strategy and set two subtrategies, with equal weights
                        @param s1 The first strategy
                        @param s2 The second strategy
                    */
                    CV_EXPORTS_W Ptr<SelectiveSearchSegmentationStrategyMultiple> createSelectiveSearchSegmentationStrategyMultiple(Ptr<SelectiveSearchSegmentationStrategy> s1, Ptr<SelectiveSearchSegmentationStrategy> s2);


                    /** @brief Create a new multiple strategy and set three subtrategies, with equal weights
                        @param s1 The first strategy
                        @param s2 The second strategy
                        @param s3 The third strategy
                    */
                    CV_EXPORTS_W Ptr<SelectiveSearchSegmentationStrategyMultiple> createSelectiveSearchSegmentationStrategyMultiple(Ptr<SelectiveSearchSegmentationStrategy> s1, Ptr<SelectiveSearchSegmentationStrategy> s2, Ptr<SelectiveSearchSegmentationStrategy> s3);

                    /** @brief Create a new multiple strategy and set four subtrategies, with equal weights
                        @param s1 The first strategy
                        @param s2 The second strategy
                        @param s3 The third strategy
                        @param s4 The forth strategy
                    */
                    CV_EXPORTS_W Ptr<SelectiveSearchSegmentationStrategyMultiple> createSelectiveSearchSegmentationStrategyMultiple(Ptr<SelectiveSearchSegmentationStrategy> s1, Ptr<SelectiveSearchSegmentationStrategy> s2, Ptr<SelectiveSearchSegmentationStrategy> s3, Ptr<SelectiveSearchSegmentationStrategy> s4);

                    /** @brief Selective search segmentation algorithm
                        The class implements the algorithm described in @cite uijlings2013selective.
                     */
                    class CV_EXPORTS_W SelectiveSearchSegmentation : public Algorithm {
                        public:

                            /** @brief Set a image used by switch* functions to initialize the class
                                @param img The image
                            */
                            CV_WRAP virtual void setBaseImage(InputArray img) = 0;

                            /** @brief Initialize the class with the 'Single stragegy' parameters describled in @cite uijlings2013selective.
                                @param k The k parameter for the graph segmentation
                                @param sigma The sigma parameter for the graph segmentation
                            */
                            CV_WRAP virtual void switchToSingleStrategy(int k = 200, float sigma = 0.8f) = 0;

                            /** @brief Initialize the class with the 'Selective search fast' parameters describled in @cite uijlings2013selective.
                                @param base_k The k parameter for the first graph segmentation
                                @param inc_k The increment of the k parameter for all graph segmentations
                                @param sigma The sigma parameter for the graph segmentation
                            */
                            CV_WRAP virtual void switchToSelectiveSearchFast(int base_k = 150, int inc_k = 150, float sigma = 0.8f) = 0;

                            /** @brief Initialize the class with the 'Selective search fast' parameters describled in @cite uijlings2013selective.
                                @param base_k The k parameter for the first graph segmentation
                                @param inc_k The increment of the k parameter for all graph segmentations
                                @param sigma The sigma parameter for the graph segmentation
                            */
                            CV_WRAP virtual void switchToSelectiveSearchQuality(int base_k = 150, int inc_k = 150, float sigma = 0.8f) = 0;

                            /** @brief Add a new image in the list of images to process.
                                @param img The image
                            */
                            CV_WRAP virtual void addImage(InputArray img) = 0;

                            /** @brief Clear the list of images to process
                            */
                            CV_WRAP virtual void clearImages() = 0;

                            /** @brief Add a new graph segmentation in the list of graph segementations to process.
                                @param g The graph segmentation
                            */
                            CV_WRAP virtual void addGraphSegmentation(Ptr<GraphSegmentation> g) = 0;

                            /** @brief Clear the list of graph segmentations to process;
                            */
                            CV_WRAP virtual void clearGraphSegmentations() = 0;

                            /** @brief Add a new strategy in the list of strategy to process.
                                @param s The strategy
                            */
                            CV_WRAP virtual void addStrategy(Ptr<SelectiveSearchSegmentationStrategy> s) = 0;

                            /** @brief Clear the list of strategy to process;
                            */
                            CV_WRAP virtual void clearStrategies() = 0;

                            /** @brief Based on all images, graph segmentations and stragies, computes all possible rects and return them
                                @param rects The list of rects. The first ones are more relevents than the lasts ones.
                            */
                            CV_WRAP virtual void process(CV_OUT std::vector<Rect>& rects) = 0;
                    };

                    /** @brief Create a new SelectiveSearchSegmentation class.
                     */
                    CV_EXPORTS_W Ptr<SelectiveSearchSegmentation> createSelectiveSearchSegmentation();

            //! @}

        }
    }
}

#endif
