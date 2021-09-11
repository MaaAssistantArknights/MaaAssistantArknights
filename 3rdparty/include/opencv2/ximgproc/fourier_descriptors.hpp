// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#ifndef __OPENCV_FOURIERDESCRIPTORS_HPP__
#define __OPENCV_FOURIERDESCRIPTORS_HPP__

#include <opencv2/core.hpp>

namespace cv {
namespace ximgproc {

    //! @addtogroup ximgproc_fourier
    //! @{

    /** @brief Class for ContourFitting algorithms.
    ContourFitting match two contours \f$ z_a \f$ and \f$ z_b \f$ minimizing distance
    \f[ d(z_a,z_b)=\sum (a_n - s  b_n e^{j(n \alpha +\phi )})^2 \f] where \f$ a_n \f$ and \f$ b_n \f$ are Fourier descriptors of \f$ z_a \f$ and \f$ z_b \f$ and s is a scaling factor and  \f$ \phi \f$ is angle rotation and \f$ \alpha \f$ is starting point factor adjustement
    */
    class CV_EXPORTS_W ContourFitting : public Algorithm
    {
        int ctrSize;
        int fdSize;
        std::vector<std::complex<double> > b;
        std::vector<std::complex<double> > a;
        std::vector<double> frequence;
        std::vector<double> rho, psi;
        void frequencyInit();
        void fAlpha(double x, double &fn, double &df);
        double distance(std::complex<double> r, double alpha);
        double  newtonRaphson(double x1, double x2);
    public:
        /** @brief Fit two closed curves using fourier descriptors. More details in @cite PersoonFu1977 and @cite BergerRaghunathan1998

        * @param ctr number of Fourier descriptors equal to number of contour points after resampling.
        * @param fd Contour defining second shape (Target).
        */
        ContourFitting(int ctr=1024,int fd=16):ctrSize(ctr),fdSize(fd){};
        /** @brief Fit two closed curves using fourier descriptors. More details in @cite PersoonFu1977 and @cite BergerRaghunathan1998

        @param src Contour defining first shape.
        @param dst Contour defining second shape (Target).
        @param alphaPhiST : \f$ \alpha \f$=alphaPhiST(0,0), \f$ \phi \f$=alphaPhiST(0,1) (in radian), s=alphaPhiST(0,2), Tx=alphaPhiST(0,3), Ty=alphaPhiST(0,4) rotation center
        @param dist distance between src and dst after matching.
        @param fdContour false then src and dst are contours and true src and dst are fourier descriptors.
        */
        void estimateTransformation(InputArray src, InputArray dst, OutputArray alphaPhiST, double *dist = 0, bool fdContour = false);
        /** @brief Fit two closed curves using fourier descriptors. More details in @cite PersoonFu1977 and @cite BergerRaghunathan1998

        @param src Contour defining first shape.
        @param dst Contour defining second shape (Target).
        @param alphaPhiST : \f$ \alpha \f$=alphaPhiST(0,0), \f$ \phi \f$=alphaPhiST(0,1) (in radian), s=alphaPhiST(0,2), Tx=alphaPhiST(0,3), Ty=alphaPhiST(0,4) rotation center
        @param dist distance between src and dst after matching.
        @param fdContour false then src and dst are contours and true src and dst are fourier descriptors.
        */
        CV_WRAP void estimateTransformation(InputArray src, InputArray dst, OutputArray alphaPhiST, CV_OUT double &dist , bool fdContour = false);
        /** @brief set number of Fourier descriptors used in estimateTransformation

        @param n number of Fourier descriptors equal to number of contour points after resampling.
        */
        CV_WRAP void setCtrSize(int n);
        /** @brief set number of Fourier descriptors when estimateTransformation used vector<Point>

        @param n number of fourier descriptors used for optimal curve matching.
        */
        CV_WRAP void setFDSize(int n);
        /**
        @returns number of fourier descriptors
        */
        CV_WRAP int getCtrSize() { return ctrSize; };
        /**
        @returns number of fourier descriptors used for optimal curve matching
        */
        CV_WRAP int getFDSize() { return fdSize; };
    };
    /**
    * @brief   Fourier descriptors for planed closed curves
    *
    * For more details about this implementation, please see @cite PersoonFu1977
    *
    * @param   src   contour type vector<Point> , vector<Point2f>  or vector<Point2d>
    * @param   dst   Mat of type CV_64FC2 and nbElt rows A VERIFIER
    * @param   nbElt number of rows in dst or getOptimalDFTSize rows if nbElt=-1
    * @param   nbFD number of FD return in dst dst = [FD(1...nbFD/2) FD(nbFD/2-nbElt+1...:nbElt)]
    *
    */
    CV_EXPORTS_W void fourierDescriptor(InputArray src, OutputArray dst, int nbElt=-1,int nbFD=-1);
    /**
    * @brief   transform a contour
    *
    * @param   src   contour or Fourier Descriptors if fd is true
    * @param   t   transform Mat given by estimateTransformation
    * @param   dst   Mat of type CV_64FC2 and nbElt rows
    * @param   fdContour true src are Fourier Descriptors. fdContour false src is a contour
    *
    */
    CV_EXPORTS_W void transformFD(InputArray src, InputArray t,OutputArray dst, bool fdContour=true);
    /**
    * @brief   Contour sampling .
    *
    * @param   src   contour type vector<Point> , vector<Point2f>  or vector<Point2d>
    * @param   out   Mat of type CV_64FC2 and nbElt rows
    * @param   nbElt number of points in out contour
    *
    */
    CV_EXPORTS_W void contourSampling(InputArray src, OutputArray out, int nbElt);

    /**
    * @brief create ContourFitting algorithm object
    *
    * @param ctr number of Fourier descriptors equal to number of contour points after resampling.
    * @param fd Contour defining second shape (Target).
    */
    CV_EXPORTS_W Ptr<ContourFitting> createContourFitting(int ctr = 1024, int fd = 16);

    //! @} ximgproc_fourier
}
}
#endif
