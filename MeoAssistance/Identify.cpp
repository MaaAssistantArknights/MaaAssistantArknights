#include "Identify.h"

#include <opencv2/opencv.hpp>

using namespace MeoAssistance;

void Identify::hello_opencv()
{
	using namespace cv;
	Mat img = imread("D:\\Code\\MeoAssistance\\Debug\\startButton1.bmp");
	if (img.empty())
	{
		printf("Could not find the image!\n");
		return;
	}

	imshow("ImputImage", img);

	waitKey(0);
}