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
		bool show_window();
		bool hide_window();

		cv::Mat get_image(bool raw = false);

		bool click(const Point& p);
		bool click(const Rect& rect);
		bool click_without_scale(const Point& p);
		bool click_without_scale(const Rect& rect);
		bool swipe(const Point& p1, const Point& p2, int duration = 0);
		bool swipe(const Rect& r1, const Rect& r2, int duration = 0);
		bool swipe_without_scale(const Point& p1, const Point& p2, int duration = 0);
		bool swipe_without_scale(const Rect& r1, const Rect& r2, int duration = 0);
	private:
		bool find_handle();
		std::optional<std::string> call_command(const std::string& cmd, bool use_pipe = true);
		Point rand_point_in_rect(const Rect& rect);

		EmulatorInfo m_emulator_info;
		HWND m_handle = nullptr;
		std::minstd_rand m_rand_engine;
		std::pair<int, int> m_scale_size;
		double m_control_scale = 1.0;

		const std::string m_screen_filename;
	};
}
