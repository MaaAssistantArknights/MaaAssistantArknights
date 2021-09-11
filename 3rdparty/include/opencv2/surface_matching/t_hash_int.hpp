//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                          License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2014, OpenCV Foundation, all rights reserved.
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

/** @file
@author Tolga Birdal <tbirdal AT gmail.com>
*/

#ifndef __OPENCV_SURFACE_MATCHING_T_HASH_INT_HPP__
#define __OPENCV_SURFACE_MATCHING_T_HASH_INT_HPP__

#include <stdio.h>
#include <stdlib.h>

namespace cv
{
namespace ppf_match_3d
{

//! @addtogroup surface_matching
//! @{

typedef uint KeyType;

typedef struct hashnode_i
{
  KeyType key;
  void *data;
  struct hashnode_i *next;
} hashnode_i ;

typedef struct HSHTBL_i
{
  size_t size;
  struct hashnode_i **nodes;
  size_t (*hashfunc)(uint);
} hashtable_int;


/** @brief Round up to the next highest power of 2

from http://www-graphics.stanford.edu/~seander/bithacks.html
*/
inline static uint next_power_of_two(uint value)
{

  --value;
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  ++value;

  return value;
}

hashtable_int *hashtableCreate(size_t size, size_t (*hashfunc)(uint));
void hashtableDestroy(hashtable_int *hashtbl);
int hashtableInsert(hashtable_int *hashtbl, KeyType key, void *data);
int hashtableInsertHashed(hashtable_int *hashtbl, KeyType key, void *data);
int hashtableRemove(hashtable_int *hashtbl, KeyType key);
void *hashtableGet(hashtable_int *hashtbl, KeyType key);
hashnode_i* hashtableGetBucketHashed(hashtable_int *hashtbl, KeyType key);
int hashtableResize(hashtable_int *hashtbl, size_t size);
hashtable_int *hashtable_int_clone(hashtable_int *hashtbl);
hashtable_int *hashtableRead(FILE* f);
int hashtableWrite(const hashtable_int * hashtbl, const size_t dataSize, FILE* f);
void hashtablePrint(hashtable_int *hashtbl);

//! @}

} // namespace ppf_match_3d

} // namespace cv
#endif


