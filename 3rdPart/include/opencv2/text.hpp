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

#ifndef __OPENCV_TEXT_HPP__
#define __OPENCV_TEXT_HPP__

#include "opencv2/text/erfilter.hpp"
#include "opencv2/text/ocr.hpp"
#include "opencv2/text/textDetector.hpp"

/** @defgroup text Scene Text Detection and Recognition

The opencv_text module provides different algorithms for text detection and recognition in natural
scene images.

  @{
    @defgroup text_detect Scene Text Detection

Class-specific Extremal Regions for Scene Text Detection
--------------------------------------------------------

The scene text detection algorithm described below has been initially proposed by Lukás Neumann &
Jiri Matas @cite Neumann11. The main idea behind Class-specific Extremal Regions is similar to the MSER
in that suitable Extremal Regions (ERs) are selected from the whole component tree of the image.
However, this technique differs from MSER in that selection of suitable ERs is done by a sequential
classifier trained for character detection, i.e. dropping the stability requirement of MSERs and
selecting class-specific (not necessarily stable) regions.

The component tree of an image is constructed by thresholding by an increasing value step-by-step
from 0 to 255 and then linking the obtained connected components from successive levels in a
hierarchy by their inclusion relation:

![image](pics/component_tree.png)

The component tree may contain a huge number of regions even for a very simple image as shown in
the previous image. This number can easily reach the order of 1 x 10\^6 regions for an average 1
Megapixel image. In order to efficiently select suitable regions among all the ERs the algorithm
make use of a sequential classifier with two differentiated stages.

In the first stage incrementally computable descriptors (area, perimeter, bounding box, and Euler's
number) are computed (in O(1)) for each region r and used as features for a classifier which
estimates the class-conditional probability p(r|character). Only the ERs which correspond to local
maximum of the probability p(r|character) are selected (if their probability is above a global limit
p_min and the difference between local maximum and local minimum is greater than a delta_min
value).

In the second stage, the ERs that passed the first stage are classified into character and
non-character classes using more informative but also more computationally expensive features. (Hole
area ratio, convex hull ratio, and the number of outer boundary inflexion points).

This ER filtering process is done in different single-channel projections of the input image in
order to increase the character localization recall.

After the ER filtering is done on each input channel, character candidates must be grouped in
high-level text blocks (i.e. words, text lines, paragraphs, ...). The opencv_text module implements
two different grouping algorithms: the Exhaustive Search algorithm proposed in @cite Neumann12 for
grouping horizontally aligned text, and the method proposed by Lluis Gomez and Dimosthenis Karatzas
in @cite Gomez13 @cite Gomez14 for grouping arbitrary oriented text (see erGrouping).

To see the text detector at work, have a look at the textdetection demo:
<https://github.com/opencv/opencv_contrib/blob/master/modules/text/samples/textdetection.cpp>

    @defgroup text_recognize Scene Text Recognition
  @}
*/

#endif
