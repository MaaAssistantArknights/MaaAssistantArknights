#pragma once

#include <string>
#include <random>
#include <Windows.h>
#include <opencv2/opencv.hpp>

#include "AssDef.h"

namespace MeoAssistance {
	class WinMacro
	{
	public:
		WinMacro(HandleType type);
		~WinMacro() = default;

		bool findHandle();
		bool resizeWindow(int Width, int Height);
		bool click(Point p);
		bool clickRange(Rect rect);
		cv::Mat getImage(Rect rect);
		Rect getWindowRect();
		double getScreenScale();
	private:

		HandleType m_handle_type;
		HWND m_handle = NULL;
		std::minstd_rand m_rand_engine;
	};
}
