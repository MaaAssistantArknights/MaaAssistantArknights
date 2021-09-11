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

#ifndef __OPENCV_DICTIONARY_HPP__
#define __OPENCV_DICTIONARY_HPP__

#include <opencv2/core.hpp>

namespace cv {
namespace aruco {

//! @addtogroup aruco
//! @{


/**
 * @brief Dictionary/Set of markers. It contains the inner codification
 *
 * bytesList contains the marker codewords where
 * - bytesList.rows is the dictionary size
 * - each marker is encoded using `nbytes = ceil(markerSize*markerSize/8.)`
 * - each row contains all 4 rotations of the marker, so its length is `4*nbytes`
 *
 * `bytesList.ptr(i)[k*nbytes + j]` is then the j-th byte of i-th marker, in its k-th rotation.
 */
class CV_EXPORTS_W Dictionary {

    public:
    CV_PROP_RW Mat bytesList;         // marker code information
    CV_PROP_RW int markerSize;        // number of bits per dimension
    CV_PROP_RW int maxCorrectionBits; // maximum number of bits that can be corrected


    /**
      */
    Dictionary(const Mat &_bytesList = Mat(), int _markerSize = 0, int _maxcorr = 0);


    /**
    Dictionary(const Dictionary &_dictionary);
    */


    /**
      */
    Dictionary(const Ptr<Dictionary> &_dictionary);


    /**
     * @see generateCustomDictionary
     */
    CV_WRAP_AS(create) static Ptr<Dictionary> create(int nMarkers, int markerSize, int randomSeed=0);


    /**
     * @see generateCustomDictionary
     */
    CV_WRAP_AS(create_from) static Ptr<Dictionary> create(int nMarkers, int markerSize,
            const Ptr<Dictionary> &baseDictionary, int randomSeed=0);

    /**
     * @see getPredefinedDictionary
     */
    CV_WRAP static Ptr<Dictionary> get(int dict);

    /**
     * @brief Given a matrix of bits. Returns whether if marker is identified or not.
     * It returns by reference the correct id (if any) and the correct rotation
     */
    bool identify(const Mat &onlyBits, int &idx, int &rotation, double maxCorrectionRate) const;

    /**
      * @brief Returns the distance of the input bits to the specific id. If allRotations is true,
      * the four posible bits rotation are considered
      */
    int getDistanceToId(InputArray bits, int id, bool allRotations = true) const;


    /**
     * @brief Draw a canonical marker image
     */
    CV_WRAP void drawMarker(int id, int sidePixels, OutputArray _img, int borderBits = 1) const;


    /**
      * @brief Transform matrix of bits to list of bytes in the 4 rotations
      */
    CV_WRAP static Mat getByteListFromBits(const Mat &bits);


    /**
      * @brief Transform list of bytes to matrix of bits
      */
    CV_WRAP static Mat getBitsFromByteList(const Mat &byteList, int markerSize);
};




/**
 * @brief Predefined markers dictionaries/sets
 * Each dictionary indicates the number of bits and the number of markers contained
 * - DICT_ARUCO_ORIGINAL: standard ArUco Library Markers. 1024 markers, 5x5 bits, 0 minimum
                          distance
 */
enum PREDEFINED_DICTIONARY_NAME {
    DICT_4X4_50 = 0,
    DICT_4X4_100,
    DICT_4X4_250,
    DICT_4X4_1000,
    DICT_5X5_50,
    DICT_5X5_100,
    DICT_5X5_250,
    DICT_5X5_1000,
    DICT_6X6_50,
    DICT_6X6_100,
    DICT_6X6_250,
    DICT_6X6_1000,
    DICT_7X7_50,
    DICT_7X7_100,
    DICT_7X7_250,
    DICT_7X7_1000,
    DICT_ARUCO_ORIGINAL,
    DICT_APRILTAG_16h5,     ///< 4x4 bits, minimum hamming distance between any two codes = 5, 30 codes
    DICT_APRILTAG_25h9,     ///< 5x5 bits, minimum hamming distance between any two codes = 9, 35 codes
    DICT_APRILTAG_36h10,    ///< 6x6 bits, minimum hamming distance between any two codes = 10, 2320 codes
    DICT_APRILTAG_36h11     ///< 6x6 bits, minimum hamming distance between any two codes = 11, 587 codes
};


/**
  * @brief Returns one of the predefined dictionaries defined in PREDEFINED_DICTIONARY_NAME
  */
CV_EXPORTS Ptr<Dictionary> getPredefinedDictionary(PREDEFINED_DICTIONARY_NAME name);


/**
  * @brief Returns one of the predefined dictionaries referenced by DICT_*.
  */
CV_EXPORTS_W Ptr<Dictionary> getPredefinedDictionary(int dict);


/**
  * @see generateCustomDictionary
  */
CV_EXPORTS_AS(custom_dictionary) Ptr<Dictionary> generateCustomDictionary(
        int nMarkers,
        int markerSize,
        int randomSeed=0);


/**
  * @brief Generates a new customizable marker dictionary
  *
  * @param nMarkers number of markers in the dictionary
  * @param markerSize number of bits per dimension of each markers
  * @param baseDictionary Include the markers in this dictionary at the beginning (optional)
  * @param randomSeed a user supplied seed for theRNG()
  *
  * This function creates a new dictionary composed by nMarkers markers and each markers composed
  * by markerSize x markerSize bits. If baseDictionary is provided, its markers are directly
  * included and the rest are generated based on them. If the size of baseDictionary is higher
  * than nMarkers, only the first nMarkers in baseDictionary are taken and no new marker is added.
  */
CV_EXPORTS_AS(custom_dictionary_from) Ptr<Dictionary> generateCustomDictionary(
        int nMarkers,
        int markerSize,
        const Ptr<Dictionary> &baseDictionary,
        int randomSeed=0);



//! @}
}
}

#endif
