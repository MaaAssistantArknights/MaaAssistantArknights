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

#ifndef __OPENCV_OPTFLOW_MOTEMPL_HPP__
#define __OPENCV_OPTFLOW_MOTEMPL_HPP__

#include "opencv2/core.hpp"

namespace cv
{
namespace motempl
{

//! @addtogroup optflow
//! @{

/** @brief Updates the motion history image by a moving silhouette.

@param silhouette Silhouette mask that has non-zero pixels where the motion occurs.
@param mhi Motion history image that is updated by the function (single-channel, 32-bit
floating-point).
@param timestamp Current time in milliseconds or other units.
@param duration Maximal duration of the motion track in the same units as timestamp .

The function updates the motion history image as follows:

\f[\texttt{mhi} (x,y)= \forkthree{\texttt{timestamp}}{if \(\texttt{silhouette}(x,y) \ne 0\)}{0}{if \(\texttt{silhouette}(x,y) = 0\) and \(\texttt{mhi} < (\texttt{timestamp} - \texttt{duration})\)}{\texttt{mhi}(x,y)}{otherwise}\f]

That is, MHI pixels where the motion occurs are set to the current timestamp , while the pixels
where the motion happened last time a long time ago are cleared.

The function, together with calcMotionGradient and calcGlobalOrientation , implements a motion
templates technique described in @cite Davis97 and @cite Bradski00 .
 */
CV_EXPORTS_W void updateMotionHistory( InputArray silhouette, InputOutputArray mhi,
                                       double timestamp, double duration );

/** @brief Calculates a gradient orientation of a motion history image.

@param mhi Motion history single-channel floating-point image.
@param mask Output mask image that has the type CV_8UC1 and the same size as mhi . Its non-zero
elements mark pixels where the motion gradient data is correct.
@param orientation Output motion gradient orientation image that has the same type and the same
size as mhi . Each pixel of the image is a motion orientation, from 0 to 360 degrees.
@param delta1 Minimal (or maximal) allowed difference between mhi values within a pixel
neighborhood.
@param delta2 Maximal (or minimal) allowed difference between mhi values within a pixel
neighborhood. That is, the function finds the minimum ( \f$m(x,y)\f$ ) and maximum ( \f$M(x,y)\f$ ) mhi
values over \f$3 \times 3\f$ neighborhood of each pixel and marks the motion orientation at \f$(x, y)\f$
as valid only if
\f[\min ( \texttt{delta1}  ,  \texttt{delta2}  )  \le  M(x,y)-m(x,y)  \le   \max ( \texttt{delta1}  , \texttt{delta2} ).\f]
@param apertureSize Aperture size of the Sobel operator.

The function calculates a gradient orientation at each pixel \f$(x, y)\f$ as:

\f[\texttt{orientation} (x,y)= \arctan{\frac{d\texttt{mhi}/dy}{d\texttt{mhi}/dx}}\f]

In fact, fastAtan2 and phase are used so that the computed angle is measured in degrees and covers
the full range 0..360. Also, the mask is filled to indicate pixels where the computed angle is
valid.

@note
   -   (Python) An example on how to perform a motion template technique can be found at
        opencv_source_code/samples/python2/motempl.py
 */
CV_EXPORTS_W void calcMotionGradient( InputArray mhi, OutputArray mask, OutputArray orientation,
                                      double delta1, double delta2, int apertureSize = 3 );

/** @brief Calculates a global motion orientation in a selected region.

@param orientation Motion gradient orientation image calculated by the function calcMotionGradient
@param mask Mask image. It may be a conjunction of a valid gradient mask, also calculated by
calcMotionGradient , and the mask of a region whose direction needs to be calculated.
@param mhi Motion history image calculated by updateMotionHistory .
@param timestamp Timestamp passed to updateMotionHistory .
@param duration Maximum duration of a motion track in milliseconds, passed to updateMotionHistory

The function calculates an average motion direction in the selected region and returns the angle
between 0 degrees and 360 degrees. The average direction is computed from the weighted orientation
histogram, where a recent motion has a larger weight and the motion occurred in the past has a
smaller weight, as recorded in mhi .
 */
CV_EXPORTS_W double calcGlobalOrientation( InputArray orientation, InputArray mask, InputArray mhi,
                                           double timestamp, double duration );

/** @brief Splits a motion history image into a few parts corresponding to separate independent motions (for
example, left hand, right hand).

@param mhi Motion history image.
@param segmask Image where the found mask should be stored, single-channel, 32-bit floating-point.
@param boundingRects Vector containing ROIs of motion connected components.
@param timestamp Current time in milliseconds or other units.
@param segThresh Segmentation threshold that is recommended to be equal to the interval between
motion history "steps" or greater.

The function finds all of the motion segments and marks them in segmask with individual values
(1,2,...). It also computes a vector with ROIs of motion connected components. After that the motion
direction for every component can be calculated with calcGlobalOrientation using the extracted mask
of the particular component.
 */
CV_EXPORTS_W void segmentMotion( InputArray mhi, OutputArray segmask,
                                 CV_OUT std::vector<Rect>& boundingRects,
                                 double timestamp, double segThresh );
                                 

//! @}

}
}                                 
                                 
#endif
