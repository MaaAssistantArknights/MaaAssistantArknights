#ifndef OPENCV_CVCONFIG_H_INCLUDED
#define OPENCV_CVCONFIG_H_INCLUDED

/* OpenCV compiled as static or dynamic libs */
#define BUILD_SHARED_LIBS

/* OpenCV intrinsics optimized code */
#define CV_ENABLE_INTRINSICS

/* OpenCV additional optimized code */
/* #undef CV_DISABLE_OPTIMIZATION */

/* Compile for 'real' NVIDIA GPU architectures */
#define CUDA_ARCH_BIN ""

/* NVIDIA GPU features are used */
#define CUDA_ARCH_FEATURES ""

/* Compile for 'virtual' NVIDIA PTX architectures */
#define CUDA_ARCH_PTX ""

/* AVFoundation video libraries */
/* #undef HAVE_AVFOUNDATION */

/* V4L capturing support */
/* #undef HAVE_CAMV4L */

/* V4L2 capturing support */
/* #undef HAVE_CAMV4L2 */

/* Carbon windowing environment */
/* #undef HAVE_CARBON */

/* AMD's Basic Linear Algebra Subprograms Library*/
/* #undef HAVE_CLAMDBLAS */

/* AMD's OpenCL Fast Fourier Transform Library*/
/* #undef HAVE_CLAMDFFT */

/* Clp support */
/* #undef HAVE_CLP */

/* Cocoa API */
/* #undef HAVE_COCOA */

/* C= */
/* #undef HAVE_CSTRIPES */

/* NVIDIA CUDA Basic Linear Algebra Subprograms (BLAS) API*/
/* #undef HAVE_CUBLAS */

/* NVIDIA CUDA Runtime API*/
/* #undef HAVE_CUDA */

/* NVIDIA CUDA Fast Fourier Transform (FFT) API*/
/* #undef HAVE_CUFFT */

/* IEEE1394 capturing support */
/* #undef HAVE_DC1394 */

/* IEEE1394 capturing support - libdc1394 v2.x */
/* #undef HAVE_DC1394_2 */

/* DirectX */
#define HAVE_DIRECTX
#define HAVE_DIRECTX_NV12
#define HAVE_D3D11
#define HAVE_D3D10
#define HAVE_D3D9

/* DirectShow Video Capture library */
#define HAVE_DSHOW

/* Eigen Matrix & Linear Algebra Library */
/* #undef HAVE_EIGEN */

/* FFMpeg video library */
#define HAVE_FFMPEG

/* Geospatial Data Abstraction Library */
/* #undef HAVE_GDAL */

/* GStreamer multimedia framework */
/* #undef HAVE_GSTREAMER */

/* GTK+ 2.0 Thread support */
/* #undef HAVE_GTHREAD */

/* GTK+ 2.x toolkit */
/* #undef HAVE_GTK */

/* Halide support */
/* #undef HAVE_HALIDE */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Intel Perceptual Computing SDK library */
/* #undef HAVE_INTELPERC */

/* Intel Integrated Performance Primitives */
#define HAVE_IPP
#define HAVE_IPP_ICV
#define HAVE_IPP_IW
#define HAVE_IPP_IW_LL

/* JPEG-2000 codec */
#define HAVE_JASPER

/* IJG JPEG codec */
#define HAVE_JPEG

/* libpng/png.h needs to be included */
/* #undef HAVE_LIBPNG_PNG_H */

/* GDCM DICOM codec */
/* #undef HAVE_GDCM */

/* V4L/V4L2 capturing support via libv4l */
/* #undef HAVE_LIBV4L */

/* Microsoft Media Foundation Capture library */
#define HAVE_MSMF

/* NVIDIA Video Decoding API*/
/* #undef HAVE_NVCUVID */
/* #undef HAVE_NVCUVID_HEADER */
/* #undef HAVE_DYNLINK_NVCUVID_HEADER */

/* NVIDIA Video Encoding API*/
/* #undef HAVE_NVCUVENC */

/* OpenCL Support */
#define HAVE_OPENCL
/* #undef HAVE_OPENCL_STATIC */
/* #undef HAVE_OPENCL_SVM */

/* NVIDIA OpenCL D3D Extensions support */
#define HAVE_OPENCL_D3D11_NV

/* OpenEXR codec */
#define HAVE_OPENEXR

/* OpenGL support*/
/* #undef HAVE_OPENGL */

/* OpenNI library */
/* #undef HAVE_OPENNI */

/* OpenNI library */
/* #undef HAVE_OPENNI2 */

/* PNG codec */
#define HAVE_PNG

/* Posix threads (pthreads) */
/* #undef HAVE_PTHREAD */

/* parallel_for with pthreads */
/* #undef HAVE_PTHREADS_PF */

/* Qt support */
/* #undef HAVE_QT */

/* Qt OpenGL support */
/* #undef HAVE_QT_OPENGL */

/* QuickTime video libraries */
/* #undef HAVE_QUICKTIME */

/* QTKit video libraries */
/* #undef HAVE_QTKIT */

/* Intel Threading Building Blocks */
/* #undef HAVE_TBB */

/* TIFF codec */
#define HAVE_TIFF

/* Unicap video capture library */
/* #undef HAVE_UNICAP */

/* Video for Windows support */
/* #undef HAVE_VFW */

/* V4L2 capturing support in videoio.h */
/* #undef HAVE_VIDEOIO */

/* Win32 UI */
#define HAVE_WIN32UI

/* XIMEA camera support */
/* #undef HAVE_XIMEA */

/* Xine video library */
/* #undef HAVE_XINE */

/* Define if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */

/* gPhoto2 library */
/* #undef HAVE_GPHOTO2 */

/* VA library (libva) */
/* #undef HAVE_VA */

/* Intel VA-API/OpenCL */
/* #undef HAVE_VA_INTEL */

/* Intel Media SDK */
/* #undef HAVE_MFX */

/* Lapack */
/* #undef HAVE_LAPACK */

/* Library was compiled with functions instrumentation */
/* #undef ENABLE_INSTRUMENTATION */

/* OpenVX */
/* #undef HAVE_OPENVX */

#if defined(HAVE_XINE)         || \
    defined(HAVE_GSTREAMER)    || \
    defined(HAVE_QUICKTIME)    || \
    defined(HAVE_QTKIT)        || \
    defined(HAVE_AVFOUNDATION) || \
    /*defined(HAVE_OPENNI)     || too specialized */ \
    defined(HAVE_FFMPEG)       || \
    defined(HAVE_MSMF)
#define HAVE_VIDEO_INPUT
#endif

#if /*defined(HAVE_XINE)       || */\
    defined(HAVE_GSTREAMER)    || \
    defined(HAVE_QUICKTIME)    || \
    defined(HAVE_QTKIT)        || \
    defined(HAVE_AVFOUNDATION) || \
    defined(HAVE_FFMPEG)       || \
    defined(HAVE_MSMF)
#define HAVE_VIDEO_OUTPUT
#endif

/* OpenCV trace utilities */
#define OPENCV_TRACE

/* Library QR-code decoding */
#define HAVE_QUIRC

#endif // OPENCV_CVCONFIG_H_INCLUDED
