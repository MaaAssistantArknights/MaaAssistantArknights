#pragma once

#include <string>
#include <random>
#include <Windows.h>
#include <opencv2/opencv.hpp>

#include "AsstDef.h"

namespace asst {
	class WinMacro
	{
	public:
		WinMacro(const std::string & simulator_name, HandleType type);
		~WinMacro() = default;

		bool captured() const noexcept;
		bool resizeWindow(int Width, int Height);
		bool resizeWindow();	// by configer
		bool showWindow();
		bool hideWindow();
		bool click(const Point & p);
		bool clickRange(const Rect & rect);
		cv::Mat getImage(const Rect& rect);
		Rect getWindowRect();
		static double getScreenScale();
	private:
		bool findHandle();

		const std::string m_simulator_name;
		const HandleType m_handle_type;
		HWND m_handle = NULL;
		std::minstd_rand m_rand_engine;
		int m_width = 0;
		int m_height = 0;
		int m_xOffset = 0;
		int m_yOffset = 0;
	};
}
