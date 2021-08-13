/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009-2012, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/
//################################################################################
//
//                    Created by Nuno Moutinho
//
//################################################################################

#ifndef _OPENCV_PLOT_H_
#define _OPENCV_PLOT_H_
#ifdef __cplusplus

#include <opencv2/core.hpp>

/**
@defgroup plot Plot function for Mat data
*/

namespace cv
{
    namespace plot
    {
    //! @addtogroup plot
    //! @{

        class CV_EXPORTS_W Plot2d : public Algorithm
        {
            public:

            CV_WRAP virtual void setMinX(double _plotMinX) = 0;
            CV_WRAP virtual void setMinY(double _plotMinY) = 0;
            CV_WRAP virtual void setMaxX(double _plotMaxX) = 0;
            CV_WRAP virtual void setMaxY(double _plotMaxY) = 0;
            CV_WRAP virtual void setPlotLineWidth(int _plotLineWidth) = 0;
            /**
             * @brief Switches data visualization mode
             *
             * @param _needPlotLine if true then neighbour plot points will be connected by lines.
             * In other case data will be plotted as a set of standalone points.
             */
            CV_WRAP virtual void setNeedPlotLine(bool _needPlotLine) = 0;
            CV_WRAP virtual void setPlotLineColor(Scalar _plotLineColor) = 0;
            CV_WRAP virtual void setPlotBackgroundColor(Scalar _plotBackgroundColor) = 0;
            CV_WRAP virtual void setPlotAxisColor(Scalar _plotAxisColor) = 0;
            CV_WRAP virtual void setPlotGridColor(Scalar _plotGridColor) = 0;
            CV_WRAP virtual void setPlotTextColor(Scalar _plotTextColor) = 0;
            CV_WRAP virtual void setPlotSize(int _plotSizeWidth, int _plotSizeHeight) = 0;
            CV_WRAP virtual void setShowGrid(bool needShowGrid) = 0;
            CV_WRAP virtual void setShowText(bool needShowText) = 0;
            CV_WRAP virtual void setGridLinesNumber(int gridLinesNumber) = 0;
            CV_WRAP virtual void setInvertOrientation(bool _invertOrientation) = 0;
            /**
             * @brief Sets the index of a point which coordinates will be printed on the top left corner of the plot (if ShowText flag is true).
             *
             * @param pointIdx index of the required point in data array.
             */
            CV_WRAP virtual void setPointIdxToPrint(int pointIdx) = 0;
            CV_WRAP virtual void render(OutputArray _plotResult) = 0;

            /**
             * @brief Creates Plot2d object
             *
             * @param data \f$1xN\f$ or \f$Nx1\f$ matrix containing \f$Y\f$ values of points to plot. \f$X\f$ values
             * will be equal to indexes of correspondind elements in data matrix.
             */
            CV_WRAP static Ptr<Plot2d> create(InputArray data);

            /**
             * @brief Creates Plot2d object
             *
             * @param dataX \f$1xN\f$ or \f$Nx1\f$ matrix \f$X\f$ values of points to plot.
             * @param dataY \f$1xN\f$ or \f$Nx1\f$ matrix containing \f$Y\f$ values of points to plot.
             */
            CV_WRAP static Ptr<Plot2d> create(InputArray dataX, InputArray dataY);
        };
    //! @}
    }
}

#endif
#endif
