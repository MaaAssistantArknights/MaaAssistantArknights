// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#ifndef OPENCV_IMG_HASH_H
#define OPENCV_IMG_HASH_H

#include "opencv2/img_hash/average_hash.hpp"
#include "opencv2/img_hash/block_mean_hash.hpp"
#include "opencv2/img_hash/color_moment_hash.hpp"
#include "opencv2/img_hash/marr_hildreth_hash.hpp"
#include "opencv2/img_hash/phash.hpp"
#include "opencv2/img_hash/radial_variance_hash.hpp"

/**
@defgroup img_hash The module brings implementations of different image hashing algorithms.

Provide algorithms to extract the hash of images and fast way to figure out most similar images in
huge data set.

Namespace for all functions is cv::img_hash.

### Supported Algorithms

- Average hash (also called Different hash)
- PHash (also called Perceptual hash)
- Marr Hildreth Hash
- Radial Variance Hash
- Block Mean Hash (modes 0 and 1)
- Color Moment Hash (this is the one and only hash algorithm resist to rotation attack(-90~90 degree))

You can study more about image hashing from following paper and websites:

- "Implementation and benchmarking of perceptual image hash functions" @cite zauner2010implementation
- "Looks Like It" @cite lookslikeit

### Code Example

@include samples/hash_samples.cpp

### Performance under different attacks

![Performance chart](img_hash/doc/attack_performance.JPG)

### Speed comparison with PHash library (100 images from ukbench)

![Hash Computation chart](img_hash/doc/hash_computation_chart.JPG)
![Hash comparison chart](img_hash/doc/hash_comparison_chart.JPG)

As you can see, hash computation speed of img_hash module outperform [PHash library](http://www.phash.org/) a lot.

PS : I do not list out the comparison of Average hash, PHash and Color Moment hash, because I cannot
find them in PHash.

### Motivation

Collects useful image hash algorithms into opencv, so we do not need to rewrite them by ourselves
again and again or rely on another 3rd party library(ex : PHash library). BOVW or correlation
matching are good and robust, but they are very slow compare with image hash, if you need to deal
with large scale CBIR(content based image retrieval) problem, image hash is a more reasonable
solution.

### More info

You can learn more about img_hash modules from following links, these links show you how to find
similar image from ukbench dataset, provide thorough benchmark of different attacks(contrast, blur,
noise(gaussion,pepper and salt), jpeg compression, watermark, resize).

* [Introduction to image hash module of opencv](http://qtandopencv.blogspot.my/2016/06/introduction-to-image-hash-module-of.html)
* [Speed up image hashing of opencv(img_hash) and introduce color moment hash](http://qtandopencv.blogspot.my/2016/06/speed-up-image-hashing-of-opencvimghash.html)

### Contributors

Tham Ngap Wei, thamngapwei@gmail.com

*/

#endif // OPENCV_IMG_HASH_H
