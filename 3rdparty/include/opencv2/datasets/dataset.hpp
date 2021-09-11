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
// Copyright (C) 2014, Itseez Inc, all rights reserved.
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
// In no event shall the Itseez Inc or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef OPENCV_DATASETS_DATASET_HPP
#define OPENCV_DATASETS_DATASET_HPP

#include <string>
#include <vector>

#include <opencv2/core.hpp>

/** @defgroup datasets Framework for working with different datasets

The datasets module includes classes for working with different datasets: load data, evaluate
different algorithms on them, contains benchmarks, etc.

It is planned to have:

-   basic: loading code for all datasets to help start work with them.
-   next stage: quick benchmarks for all datasets to show how to solve them using OpenCV and
implement evaluation code.
-   finally: implement on OpenCV state-of-the-art algorithms, which solve these tasks.

@{
@defgroup datasets_ar Action Recognition

### HMDB: A Large Human Motion Database

Implements loading dataset:

"HMDB: A Large Human Motion Database": <http://serre-lab.clps.brown.edu/resource/hmdb-a-large-human-motion-database/>

Usage:
-# From link above download dataset files: `hmdb51_org.rar` & `test_train_splits.rar`.
-# Unpack them. Unpack all archives from directory: `hmdb51_org/` and remove them.
-# To load data run:
~~~
./opencv/build/bin/example_datasets_ar_hmdb -p=/home/user/path_to_unpacked_folders/
~~~

#### Benchmark

For this dataset was implemented benchmark with accuracy: 0.107407 (using precomputed HOG/HOF
"STIP" features from site, averaging for 3 splits)

To run this benchmark execute:
~~~
./opencv/build/bin/example_datasets_ar_hmdb_benchmark -p=/home/user/path_to_unpacked_folders/
~~~

@note
Precomputed features should be unpacked in the same folder: `/home/user/path_to_unpacked_folders/hmdb51_org_stips/`.
Also unpack all archives from directory: `hmdb51_org_stips/` and remove them.

### Sports-1M %Dataset

Implements loading dataset:

"Sports-1M Dataset": <http://cs.stanford.edu/people/karpathy/deepvideo/>

Usage:
-# From link above download dataset files (`git clone https://code.google.com/p/sports-1m-dataset/`).
-# To load data run:
~~~
./opencv/build/bin/example_datasets_ar_sports -p=/home/user/path_to_downloaded_folders/
~~~

@defgroup datasets_fr Face Recognition

### Adience

Implements loading dataset:

"Adience": <http://www.openu.ac.il/home/hassner/Adience/data.html>

Usage:
-# From link above download any dataset file: `faces.tar.gz\aligned.tar.gz` and files with splits:
`fold_0_data.txt-fold_4_data.txt`, `fold_frontal_0_data.txt-fold_frontal_4_data.txt`. (For
face recognition task another splits should be created)
-# Unpack dataset file to some folder and place split files into the same folder.
-# To load data run:
~~~
./opencv/build/bin/example_datasets_fr_adience -p=/home/user/path_to_created_folder/
~~~

### Labeled Faces in the Wild

Implements loading dataset:

"Labeled Faces in the Wild": <http://vis-www.cs.umass.edu/lfw/>

Usage:
-# From link above download any dataset file:
`lfw.tgz\lfwa.tar.gz\lfw-deepfunneled.tgz\lfw-funneled.tgz` and files with pairs: 10 test
splits: `pairs.txt` and developer train split: `pairsDevTrain.txt`.
-# Unpack dataset file and place `pairs.txt` and `pairsDevTrain.txt` in created folder.
-# To load data run:
~~~
./opencv/build/bin/example_datasets_fr_lfw -p=/home/user/path_to_unpacked_folder/lfw2/
~~~

#### Benchmark

For this dataset was implemented benchmark with accuracy: 0.623833 +- 0.005223 (train split:
`pairsDevTrain.txt`, dataset: lfwa)

To run this benchmark execute:
~~~
./opencv/build/bin/example_datasets_fr_lfw_benchmark -p=/home/user/path_to_unpacked_folder/lfw2/
~~~

@defgroup datasets_gr Gesture Recognition

### ChaLearn Looking at People

Implements loading dataset:

"ChaLearn Looking at People": <http://gesture.chalearn.org/>

Usage
-# Follow instruction from site above, download files for dataset "Track 3: Gesture Recognition":
`Train1.zip`-`Train5.zip`, `Validation1.zip`-`Validation3.zip` (Register on site: www.codalab.org and
accept the terms and conditions of competition:
<https://www.codalab.org/competitions/991#learn_the_details> There are three mirrors for
downloading dataset files. When I downloaded data only mirror: "Universitat Oberta de Catalunya"
works).
-# Unpack train archives `Train1.zip`-`Train5.zip` to folder `Train/`, validation archives
`Validation1.zip`-`Validation3.zip` to folder `Validation/`
-# Unpack all archives in `Train/` & `Validation/` in the folders with the same names, for example:
`Sample0001.zip` to `Sample0001/`
-# To load data run:
~~~
./opencv/build/bin/example_datasets_gr_chalearn -p=/home/user/path_to_unpacked_folders/
~~~

### Sheffield Kinect Gesture Dataset

Implements loading dataset:

"Sheffield Kinect Gesture Dataset": <http://lshao.staff.shef.ac.uk/data/SheffieldKinectGesture.htm>

Usage:
-# From link above download dataset files: `subject1_dep.7z`-`subject6_dep.7z`, `subject1_rgb.7z`-`subject6_rgb.7z`.
-# Unpack them.
-# To load data run:
~~~
./opencv/build/bin/example_datasets_gr_skig -p=/home/user/path_to_unpacked_folders/
~~~

@defgroup datasets_hpe Human Pose Estimation

### HumanEva Dataset

Implements loading dataset:

"HumanEva Dataset": <http://humaneva.is.tue.mpg.de>

Usage:
-# From link above download dataset files for `HumanEva-I` (tar) & `HumanEva-II`.
-# Unpack them to `HumanEva_1` & `HumanEva_2` accordingly.
-# To load data run:
~~~
./opencv/build/bin/example_datasets_hpe_humaneva -p=/home/user/path_to_unpacked_folders/
~~~

### PARSE Dataset

Implements loading dataset:

"PARSE Dataset": <http://www.ics.uci.edu/~dramanan/papers/parse/>

Usage:
-# From link above download dataset file: `people.zip`.
-# Unpack it.
-# To load data run:
~~~
./opencv/build/bin/example_datasets_hpe_parse -p=/home/user/path_to_unpacked_folder/people_all/
~~~

@defgroup datasets_ir Image Registration

### Affine Covariant Regions Datasets

Implements loading dataset:

"Affine Covariant Regions Datasets": <http://www.robots.ox.ac.uk/~vgg/data/data-aff.html>

Usage:
-# From link above download dataset files:
`bark\bikes\boat\graf\leuven\trees\ubc\wall.tar.gz`.
-# Unpack them.
-# To load data, for example, for "bark", run:
```
./opencv/build/bin/example_datasets_ir_affine -p=/home/user/path_to_unpacked_folder/bark/
```

### Robot Data Set

Implements loading dataset:

"Robot Data Set, Point Feature Data Set – 2010": <http://roboimagedata.compute.dtu.dk/?page_id=24>

Usage:
-# From link above download dataset files: `SET001_6.tar.gz`-`SET055_60.tar.gz`
-# Unpack them to one folder.
-# To load data run:
~~~
./opencv/build/bin/example_datasets_ir_robot -p=/home/user/path_to_unpacked_folder/
~~~

@defgroup datasets_is Image Segmentation

### The Berkeley Segmentation Dataset and Benchmark

Implements loading dataset:

"The Berkeley Segmentation Dataset and Benchmark": <https://www.eecs.berkeley.edu/Research/Projects/CS/vision/bsds/>

Usage:
-# From link above download dataset files: `BSDS300-human.tgz` & `BSDS300-images.tgz`.
-# Unpack them.
-# To load data run:
~~~
./opencv/build/bin/example_datasets_is_bsds -p=/home/user/path_to_unpacked_folder/BSDS300/
~~~

### Weizmann Segmentation Evaluation Database

Implements loading dataset:

"Weizmann Segmentation Evaluation Database": <http://www.wisdom.weizmann.ac.il/~vision/Seg_Evaluation_DB/>

Usage:
-# From link above download dataset files: `Weizmann_Seg_DB_1obj.ZIP` & `Weizmann_Seg_DB_2obj.ZIP`.
-# Unpack them.
-# To load data, for example, for `1 object` dataset, run:
~~~
./opencv/build/bin/example_datasets_is_weizmann -p=/home/user/path_to_unpacked_folder/1obj/
~~~

@defgroup datasets_msm Multiview Stereo Matching

### EPFL Multi-View Stereo

Implements loading dataset:

"EPFL Multi-View Stereo": <http://cvlab.epfl.ch/data/strechamvs>

Usage:
-# From link above download dataset files:
`castle_dense\castle_dense_large\castle_entry\fountain\herzjesu_dense\herzjesu_dense_large_bounding\cameras\images\p.tar.gz`.
-# Unpack them in separate folder for each object. For example, for "fountain", in folder `fountain/` :
`fountain_dense_bounding.tar.gz -> bounding/`,
`fountain_dense_cameras.tar.gz -> camera/`,
`fountain_dense_images.tar.gz -> png/`,
`fountain_dense_p.tar.gz -> P/`
-# To load data, for example, for "fountain", run:
~~~
./opencv/build/bin/example_datasets_msm_epfl -p=/home/user/path_to_unpacked_folder/fountain/
~~~

### Stereo – Middlebury Computer Vision

Implements loading dataset:

"Stereo – Middlebury Computer Vision": <http://vision.middlebury.edu/mview/>

Usage:
-# From link above download dataset files:
`dino\dinoRing\dinoSparseRing\temple\templeRing\templeSparseRing.zip`
-# Unpack them.
-# To load data, for example "temple" dataset, run:
~~~
./opencv/build/bin/example_datasets_msm_middlebury -p=/home/user/path_to_unpacked_folder/temple/
~~~

@defgroup datasets_or Object Recognition

### ImageNet

Implements loading dataset:  "ImageNet": <http://www.image-net.org/>

Usage:
-# From link above download dataset files:
`ILSVRC2010_images_train.tar\ILSVRC2010_images_test.tar\ILSVRC2010_images_val.tar` & devkit:
`ILSVRC2010_devkit-1.0.tar.gz` (Implemented loading of 2010 dataset as only this dataset has ground
truth for test data, but structure for ILSVRC2014 is similar)
-# Unpack them to: `some_folder/train/`, `some_folder/test/`, `some_folder/val` &
`some_folder/ILSVRC2010_validation_ground_truth.txt`,
`some_folder/ILSVRC2010_test_ground_truth.txt`.
-# Create file with labels: `some_folder/labels.txt`, for example, using python script below (each
file's row format: `synset,labelID,description`. For example: "n07751451,18,plum").
-# Unpack all tar files in train.
-# To load data run:
~~~
./opencv/build/bin/example_datasets_or_imagenet -p=/home/user/some_folder/
~~~

Python script to parse `meta.mat`:
~~~{py}
    import scipy.io
    meta_mat = scipy.io.loadmat("devkit-1.0/data/meta.mat")

    labels_dic = dict((m[0][1][0], m[0][0][0][0]-1) for m in meta_mat['synsets']
    label_names_dic = dict((m[0][1][0], m[0][2][0]) for m in meta_mat['synsets']

    for label in labels_dic.keys():
        print "{0},{1},{2}".format(label, labels_dic[label], label_names_dic[label])
~~~

### MNIST

Implements loading dataset:

"MNIST": <http://yann.lecun.com/exdb/mnist/>

Usage:
-# From link above download dataset files:
`t10k-images-idx3-ubyte.gz`, `t10k-labels-idx1-ubyte.gz`, `train-images-idx3-ubyte.gz`, `train-labels-idx1-ubyte.gz`.
-# Unpack them.
-# To load data run:
~~~
./opencv/build/bin/example_datasets_or_mnist -p=/home/user/path_to_unpacked_files/
~~~

### SUN Database

Implements loading dataset:

"SUN Database, Scene Recognition Benchmark. SUN397": <http://vision.cs.princeton.edu/projects/2010/SUN/>

Usage:
-# From link above download dataset file: `SUN397.tar` & file with splits: `Partitions.zip`
-# Unpack `SUN397.tar` into folder: `SUN397/` & `Partitions.zip` into folder: `SUN397/Partitions/`
-# To load data run:
~~~
./opencv/build/bin/example_datasets_or_sun -p=/home/user/path_to_unpacked_files/SUN397/
~~~

@defgroup datasets_pd Pedestrian Detection

### Caltech Pedestrian Detection Benchmark

Implements loading dataset:

"Caltech Pedestrian Detection Benchmark": <http://www.vision.caltech.edu/Image_Datasets/CaltechPedestrians/>

@note First version of Caltech Pedestrian dataset loading. Code to unpack all frames from seq files
commented as their number is huge! So currently load only meta information without data. Also
ground truth isn't processed, as need to convert it from mat files first.

Usage:
-# From link above download dataset files: `set00.tar`-`set10.tar`.
-# Unpack them to separate folder.
-# To load data run:
~~~
./opencv/build/bin/example_datasets_pd_caltech -p=/home/user/path_to_unpacked_folders/
~~~

@defgroup datasets_slam SLAM

### KITTI Vision Benchmark

Implements loading dataset:

"KITTI Vision Benchmark": <http://www.cvlibs.net/datasets/kitti/eval_odometry.php>

Usage:
-# From link above download "Odometry" dataset files:
`data_odometry_gray\data_odometry_color\data_odometry_velodyne\data_odometry_poses\data_odometry_calib.zip`.
-# Unpack `data_odometry_poses.zip`, it creates folder `dataset/poses/`. After that unpack
`data_odometry_gray.zip`, `data_odometry_color.zip`, `data_odometry_velodyne.zip`. Folder
`dataset/sequences/` will be created with folders `00/..21/`. Each of these folders will contain:
`image_0/`, `image_1/`, `image_2/`, `image_3/`, `velodyne/` and files `calib.txt` & `times.txt`.
These two last files will be replaced after unpacking `data_odometry_calib.zip` at the end.
-# To load data run:
~~~
./opencv/build/bin/example_datasets_slam_kitti -p=/home/user/path_to_unpacked_folder/dataset/
~~~

### TUMindoor Dataset

Implements loading dataset:

"TUMindoor Dataset": <http://www.navvis.lmt.ei.tum.de/dataset/>

Usage:
-# From link above download dataset files: `dslr\info\ladybug\pointcloud.tar.bz2` for each dataset:
`11-11-28 (1st floor)\11-12-13 (1st floor N1)\11-12-17a (4th floor)\11-12-17b (3rd floor)\11-12-17c (Ground I)\11-12-18a (Ground II)\11-12-18b (2nd floor)`
-# Unpack them in separate folder for each dataset.
`dslr.tar.bz2 -> dslr/`,
`info.tar.bz2 -> info/`,
`ladybug.tar.bz2 -> ladybug/`,
`pointcloud.tar.bz2 -> pointcloud/`.
-# To load each dataset run:
~~~
./opencv/build/bin/example_datasets_slam_tumindoor -p=/home/user/path_to_unpacked_folders/
~~~

@defgroup datasets_tr Text Recognition

### The Chars74K Dataset

Implements loading dataset:

"The Chars74K Dataset": <http://www.ee.surrey.ac.uk/CVSSP/demos/chars74k/>

Usage:
-# From link above download dataset files:
`EnglishFnt\EnglishHnd\EnglishImg\KannadaHnd\KannadaImg.tgz`, `ListsTXT.tgz`.
-# Unpack them.
-# Move `.m` files from folder `ListsTXT/` to appropriate folder. For example,
`English/list_English_Img.m` for `EnglishImg.tgz`.
-# To load data, for example "EnglishImg", run:
~~~
./opencv/build/bin/example_datasets_tr_chars -p=/home/user/path_to_unpacked_folder/English/
~~~

### The Street View Text Dataset

Implements loading dataset:

"The Street View Text Dataset": <http://vision.ucsd.edu/~kai/svt/>

Usage:
-# From link above download dataset file: `svt.zip`.
-# Unpack it.
-# To load data run:
~~~
./opencv/build/bin/example_datasets_tr_svt -p=/home/user/path_to_unpacked_folder/svt/svt1/
~~~

#### Benchmark

For this dataset was implemented benchmark with accuracy (mean f1): 0.217

To run benchmark execute:
~~~
./opencv/build/bin/example_datasets_tr_svt_benchmark -p=/home/user/path_to_unpacked_folders/svt/svt1/
~~~

@defgroup datasets_track Tracking

### VOT 2015 Database

Implements loading dataset:

"VOT 2015 dataset comprises 60 short sequences showing various objects in challenging backgrounds.
The sequences were chosen from a large pool of sequences including the ALOV dataset, OTB2 dataset,
non-tracking datasets, Computer Vision Online, Professor Bob Fisher's Image Database, Videezy,
Center for Research in Computer Vision, University of Central Florida, USA, NYU Center for Genomics
and Systems Biology, Data Wrangling, Open Access Directory and Learning and Recognition in Vision
Group, INRIA, France. The VOT sequence selection protocol was applied to obtain a representative
set of challenging sequences.": <http://box.vicos.si/vot/vot2015.zip>

Usage:
-# From link above download dataset file: `vot2015.zip`
-# Unpack `vot2015.zip` into folder: `VOT2015/`
-# To load data run:
~~~
./opencv/build/bin/example_datasets_track_vot -p=/home/user/path_to_unpacked_files/VOT2015/
~~~
@}

*/

namespace cv
{
namespace datasets
{

//! @addtogroup datasets
//! @{

struct Object
{
};

class CV_EXPORTS Dataset
{
public:
    Dataset() {}
    virtual ~Dataset() {}

    virtual void load(const std::string &path) = 0;

    std::vector< Ptr<Object> >& getTrain(int splitNum = 0);
    std::vector< Ptr<Object> >& getTest(int splitNum = 0);
    std::vector< Ptr<Object> >& getValidation(int splitNum = 0);

    int getNumSplits() const;

protected:
    std::vector< std::vector< Ptr<Object> > > train;
    std::vector< std::vector< Ptr<Object> > > test;
    std::vector< std::vector< Ptr<Object> > > validation;

private:
    std::vector< Ptr<Object> > empty;
};

//! @}

}
}

#endif
