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
 // Copyright (C) 2015, OpenCV Foundation, all rights reserved.
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

/*#ifdef __OPENCV_BUILD
 #error this is a compatibility header which should not be used inside the OpenCV library
 #endif*/

#include "opencv2/structured_light/structured_light.hpp"
#include "opencv2/structured_light/graycodepattern.hpp"
#include "opencv2/structured_light/sinusoidalpattern.hpp"

/** @defgroup structured_light Structured Light API

 Structured light is considered one of the most effective techniques to acquire 3D models.
 This technique is based on projecting a light pattern and capturing the illuminated scene
 from one or more points of view. Since the pattern is coded, correspondences between image
 points and points of the projected pattern can be quickly found and 3D information easily
 retrieved.

 One of the most commonly exploited coding strategies is based on trmatime-multiplexing. In this
 case, a set of patterns  are successively projected onto the measuring surface.
 The codeword for a given pixel is usually formed by  the sequence of illuminance values for that
 pixel across the projected patterns. Thus, the codification is called  temporal because the bits
 of the codewords are multiplexed in time @cite pattern .

 In this module a time-multiplexing coding strategy based on Gray encoding is implemented following the
 (stereo) approach described in 3DUNDERWORLD algorithm @cite UNDERWORLD .
 For more details, see @ref tutorial_structured_light.

 */