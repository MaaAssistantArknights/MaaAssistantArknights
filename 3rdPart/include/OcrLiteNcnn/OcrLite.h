#ifndef __OCR_LITE_H__
#define __OCR_LITE_H__

#include "opencv2/core.hpp"
#include "OcrStruct.h"
#include "DbNet.h"
#include "AngleNet.h"
#include "CrnnNet.h"

namespace ocr {
	class OCRLITE_EXPORT OcrLite {
	public:
		OcrLite();

		~OcrLite();

		void setNumThread(int numOfThread);

		void initLogger(bool isConsole, bool isPartImg, bool isResultImg);

		void enableResultTxt(const char* path, const char* imgName);

		void setGpuIndex(int gpuIndex);

		bool initModels(const std::string& detPath, const std::string& clsPath,
			const std::string& recPath, const std::string& keysPath);

		void Logger(const char* format, ...);

		/*
		* padding：图像预处理，在图片外周添加白边，用于提升识别率，文字框没有正确框住所有文字时，增加此值。
		* maxSideLen ：按图片最长边的长度，此值为0代表不缩放，例：1024，如果图片长边大于1024则把图像整体缩小到1024再进行图像分割计算，如果图片长边小于1024则不缩放，如果图片长边小于32，则缩放到32。
		* boxScoreThresh：文字框置信度门限，文字框没有正确框住所有文字时，减小此值。
		* boxThresh：请自行试验。
		* unClipRatio：单个文字框大小倍率，越大时单个文字框越大。此项与图片的大小相关，越大的图片此值应该越大。
		* doAngle：启用(1)/禁用(0) 文字方向检测，只有图片倒置的情况下(旋转90~270度的图片)，才需要启用文字方向检测。
		* mostAngle：启用(1)/禁用(0) 角度投票(整张图片以最大可能文字方向来识别)，当禁用文字方向检测时，此项也不起作用。
		*/
		OcrResult detect(const char* path, const char* imgName,
			int padding, int maxSideLen,
			float boxScoreThresh, float boxThresh, float unClipRatio, bool doAngle, bool mostAngle);

		/*
		* padding：图像预处理，在图片外周添加白边，用于提升识别率，文字框没有正确框住所有文字时，增加此值。
		* maxSideLen ：按图片最长边的长度，此值为0代表不缩放，例：1024，如果图片长边大于1024则把图像整体缩小到1024再进行图像分割计算，如果图片长边小于1024则不缩放，如果图片长边小于32，则缩放到32。
		* boxScoreThresh：文字框置信度门限，文字框没有正确框住所有文字时，减小此值。
		* boxThresh：请自行试验。
		* unClipRatio：单个文字框大小倍率，越大时单个文字框越大。此项与图片的大小相关，越大的图片此值应该越大。
		* doAngle：启用(1)/禁用(0) 文字方向检测，只有图片倒置的情况下(旋转90~270度的图片)，才需要启用文字方向检测。
		* mostAngle：启用(1)/禁用(0) 角度投票(整张图片以最大可能文字方向来识别)，当禁用文字方向检测时，此项也不起作用。
		*/
		OcrResult detect(const cv::Mat& mat,
			int padding, int maxSideLen,
			float boxScoreThresh, float boxThresh, float unClipRatio, bool doAngle, bool mostAngle);
	private:
		bool isOutputConsole = false;
		bool isOutputPartImg = false;
		bool isOutputResultTxt = false;
		bool isOutputResultImg = false;
		FILE* resultTxt;
		DbNet dbNet;
		AngleNet angleNet;
		CrnnNet crnnNet;

		std::vector<cv::Mat> getPartImages(cv::Mat& src, std::vector<TextBox>& textBoxes,
			const char* path, const char* imgName);

		OcrResult detect(const char* path, const char* imgName,
			cv::Mat& src, cv::Rect& originRect, ScaleParam& scale,
			float boxScoreThresh = 0.6f, float boxThresh = 0.3f,
			float unClipRatio = 2.0f, bool doAngle = true, bool mostAngle = true);
	};
}

#endif //__OCR_LITE_H__
