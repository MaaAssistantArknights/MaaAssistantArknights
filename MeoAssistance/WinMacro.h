#pragma once

#include <string>
#include <random>
#include <optional>

#include <Windows.h>

#include "AsstDef.h"

namespace cv {
	class Mat;
}

namespace asst {
	class WinMacro
	{
	public:
		WinMacro(const EmulatorInfo& info);
		~WinMacro() = default;

		bool captured() const noexcept;
		bool showWindow();
		bool hideWindow();
		bool click(const Point& p);
		bool click(const Rect& rect);
		bool swipe(const Point& p1, const Point& p2, int duration = 0);
		bool swipe(const Rect& r1, const Rect& r2, int duration = 0);
		void setControlScale(double scale) noexcept;
		cv::Mat getImage();				// 通过Adb截图，会高清一点，但是比较慢（通过adb pull出来，有io操作）
		std::pair<int, int> getDisplaySize();
	private:
		bool findHandle();
		std::optional<std::string> callCmd(const std::string& cmd, bool use_pipe = true);
		Point randPointInRect(const Rect& rect);

		EmulatorInfo m_emulator_info;
		HWND m_handle = nullptr;
		std::minstd_rand m_rand_engine;
		double m_control_scale = 1.0;
		const std::string m_screen_filename;
	};
}
