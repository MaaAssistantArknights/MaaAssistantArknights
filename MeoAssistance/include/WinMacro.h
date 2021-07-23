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
		WinMacro(const SimulatorInfo & info, HandleType type);
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

		const SimulatorInfo m_simulator_info;
		const HandleType m_handle_type;
		HWND m_handle = NULL;
		bool m_is_adb = false;
		std::string m_click_cmd;
		std::minstd_rand m_rand_engine;
		int m_width = 0;
		int m_height = 0;
		int m_x_offset = 0;
		int m_y_offset = 0;
	};
}
