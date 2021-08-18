// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#ifndef __OPENCV_FAST_LINE_DETECTOR_HPP__
#define __OPENCV_FAST_LINE_DETECTOR_HPP__

#include <opencv2/core.hpp>

namespace cv
{
namespace ximgproc
{

//! @addtogroup ximgproc_fast_line_detector
//! @{

/** @brief Class implementing the FLD (Fast Line Detector) algorithm described
in @cite Lee14 .
*/

//! @include samples/fld_lines.cpp

class CV_EXPORTS_W FastLineDetector : public Algorithm
{
public:
    /** @example fld_lines.cpp
      An example using the FastLineDetector
      */
    /** @brief Finds lines in the input image.
      This is the output of the default parameters of the algorithm on the above
      shown image.

      ![image](pics/corridor_fld.jpg)

      @param _image A grayscale (CV_8UC1) input image. If only a roi needs to be
      selected, use: `fld_ptr-\>detect(image(roi), lines, ...);
      lines += Scalar(roi.x, roi.y, roi.x, roi.y);`
      @param _lines A vector of Vec4f elements specifying the beginning
      and ending point of a line.  Where Vec4f is (x1, y1, x2, y2), point
      1 is the start, point 2 - end. Returned lines are directed so that the
      brighter side is on their left.
      */
    CV_WRAP virtual void detect(InputArray _image, OutputArray _lines) = 0;

    /** @brief Draws the line segments on a given image.
      @param _image The image, where the lines will be drawn. Should be bigger
      or equal to the image, where the lines were found.
      @param lines A vector of the lines that needed to be drawn.
      @param draw_arrow If true, arrow heads will be drawn.
    */
    CV_WRAP virtual void drawSegments(InputOutputArray _image, InputArray lines,
            bool draw_arrow = false) = 0;

    virtual ~FastLineDetector() { }
};

/** @brief Creates a smart pointer to a FastLineDetector object and initializes it

@param _length_threshold    10         - Segment shorter than this will be discarded
@param _distance_threshold  1.41421356 - A point placed from a hypothesis line
                                         segment farther than this will be
                                         regarded as an outlier
@param _canny_th1           50         - First threshold for
                                         hysteresis procedure in Canny()
@param _canny_th2           50         - Second threshold for
                                         hysteresis procedure in Canny()
@param _canny_aperture_size 3          - Aperturesize for the sobel
                                         operator in Canny()
@param _do_merge            false      - If true, incremental merging of segments
                                         will be perfomred
*/
CV_EXPORTS_W Ptr<FastLineDetector> createFastLineDetector(
        int _length_threshold = 10, float _distance_threshold = 1.414213562f,
        double _canny_th1 = 50.0, double _canny_th2 = 50.0, int _canny_aperture_size = 3,
        bool _do_merge = false);

//! @} ximgproc_fast_line_detector
}
}
#endif
