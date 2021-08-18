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
 // Copyright (C) 2013, OpenCV Foundation, all rights reserved.
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

#ifndef __OPENCV_LINE_DESCRIPTOR_HPP__
#define __OPENCV_LINE_DESCRIPTOR_HPP__

#include "opencv2/line_descriptor/descriptor.hpp"

/** @defgroup line_descriptor Binary descriptors for lines extracted from an image

Introduction
------------

One of the most challenging activities in computer vision is the extraction of useful information
from a given image. Such information, usually comes in the form of points that preserve some kind of
property (for instance, they are scale-invariant) and are actually representative of input image.

The goal of this module is seeking a new kind of representative information inside an image and
providing the functionalities for its extraction and representation. In particular, differently from
previous methods for detection of relevant elements inside an image, lines are extracted in place of
points; a new class is defined ad hoc to summarize a line's properties, for reuse and plotting
purposes.

Computation of binary descriptors
---------------------------------

To obtatin a binary descriptor representing a certain line detected from a certain octave of an
image, we first compute a non-binary descriptor as described in @cite LBD . Such algorithm works on
lines extracted using EDLine detector, as explained in @cite EDL . Given a line, we consider a
rectangular region centered at it and called *line support region (LSR)*. Such region is divided
into a set of bands \f$\{B_1, B_2, ..., B_m\}\f$, whose length equals the one of line.

If we indicate with \f$\bf{d}_L\f$ the direction of line, the orthogonal and clockwise direction to line
\f$\bf{d}_{\perp}\f$ can be determined; these two directions, are used to construct a reference frame
centered in the middle point of line. The gradients of pixels \f$\bf{g'}\f$ inside LSR can be projected
to the newly determined frame, obtaining their local equivalent
\f$\bf{g'} = (\bf{g}^T \cdot \bf{d}_{\perp}, \bf{g}^T \cdot \bf{d}_L)^T \triangleq (\bf{g'}_{d_{\perp}}, \bf{g'}_{d_L})^T\f$.

Later on, a Gaussian function is applied to all LSR's pixels along \f$\bf{d}_\perp\f$ direction; first,
we assign a global weighting coefficient \f$f_g(i) = (1/\sqrt{2\pi}\sigma_g)e^{-d^2_i/2\sigma^2_g}\f$ to
*i*-th row in LSR, where \f$d_i\f$ is the distance of *i*-th row from the center row in LSR,
\f$\sigma_g = 0.5(m \cdot w - 1)\f$ and \f$w\f$ is the width of bands (the same for every band). Secondly,
considering a band \f$B_j\f$ and its neighbor bands \f$B_{j-1}, B_{j+1}\f$, we assign a local weighting
\f$F_l(k) = (1/\sqrt{2\pi}\sigma_l)e^{-d'^2_k/2\sigma_l^2}\f$, where \f$d'_k\f$ is the distance of *k*-th
row from the center row in \f$B_j\f$ and \f$\sigma_l = w\f$. Using the global and local weights, we obtain,
at the same time, the reduction of role played by gradients far from line and of boundary effect,
respectively.

Each band \f$B_j\f$ in LSR has an associated *band descriptor(BD)* which is computed considering
previous and next band (top and bottom bands are ignored when computing descriptor for first and
last band). Once each band has been assignen its BD, the LBD descriptor of line is simply given by

\f[LBD = (BD_1^T, BD_2^T, ... , BD^T_m)^T.\f]

To compute a band descriptor \f$B_j\f$, each *k*-th row in it is considered and the gradients in such
row are accumulated:

\f[\begin{matrix} \bf{V1}^k_j = \lambda \sum\limits_{\bf{g}'_{d_\perp}>0}\bf{g}'_{d_\perp}, &  \bf{V2}^k_j = \lambda \sum\limits_{\bf{g}'_{d_\perp}<0} -\bf{g}'_{d_\perp}, \\ \bf{V3}^k_j = \lambda \sum\limits_{\bf{g}'_{d_L}>0}\bf{g}'_{d_L}, & \bf{V4}^k_j = \lambda \sum\limits_{\bf{g}'_{d_L}<0} -\bf{g}'_{d_L}\end{matrix}.\f]

with \f$\lambda = f_g(k)f_l(k)\f$.

By stacking previous results, we obtain the *band description matrix (BDM)*

\f[BDM_j = \left(\begin{matrix} \bf{V1}_j^1 & \bf{V1}_j^2 & \ldots & \bf{V1}_j^n \\ \bf{V2}_j^1 & \bf{V2}_j^2 & \ldots & \bf{V2}_j^n \\ \bf{V3}_j^1 & \bf{V3}_j^2 & \ldots & \bf{V3}_j^n \\ \bf{V4}_j^1 & \bf{V4}_j^2 & \ldots & \bf{V4}_j^n \end{matrix} \right) \in \mathbb{R}^{4\times n},\f]

with \f$n\f$ the number of rows in band \f$B_j\f$:

\f[n = \begin{cases} 2w, & j = 1||m; \\ 3w, & \mbox{else}. \end{cases}\f]

Each \f$BD_j\f$ can be obtained using the standard deviation vector \f$S_j\f$ and mean vector \f$M_j\f$ of
\f$BDM_J\f$. Thus, finally:

\f[LBD = (M_1^T, S_1^T, M_2^T, S_2^T, \ldots, M_m^T, S_m^T)^T \in \mathbb{R}^{8m}\f]

Once the LBD has been obtained, it must be converted into a binary form. For such purpose, we
consider 32 possible pairs of BD inside it; each couple of BD is compared bit by bit and comparison
generates an 8 bit string. Concatenating 32 comparison strings, we get the 256-bit final binary
representation of a single LBD.
*/

#endif 
