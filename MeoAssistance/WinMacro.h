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
		WinMacro(const EmulatorInfo& info, HandleType type);
		~WinMacro() = default;

		bool captured() const noexcept;
		bool resizeWindow(int Width, int Height);
		bool resizeWindow();	// by configer
		bool showWindow();
		bool hideWindow();
		bool click(const Point& p);
		bool click(const Rect& rect);
		bool swipe(const Point& p1, const Point& p2, int duration = 0);
		bool swipe(const Rect& r1, const Rect& r2, int duration = 0);
		void setControlScale(double scale);
		[[ deprecated ]] cv::Mat getImage(const Rect& rect);	// 通过Win32 Api对窗口截图
		cv::Mat getAdbImage();				// 通过Adb截图，会高清一点，但是比较慢（通过adb pull出来，有io操作）
		Rect getWindowRect();
		const EmulatorInfo& getEmulatorInfo() const noexcept { return m_emulator_info; }
		const HandleType& getHandleType() const noexcept { return m_handle_type; }

		static double getScreenScale();
	private:
		bool findHandle();
		std::optional<std::string> callCmd(const std::string& cmd, bool use_pipe = true);
		Point randPointInRect(const Rect& rect);

		const EmulatorInfo m_emulator_info;
		const HandleType m_handle_type;
		HWND m_handle = NULL;
		bool m_is_adb = false;
		std::string m_click_cmd;		// adb点击命令，不是adb的句柄用不到这个
		std::string m_swipe_cmd;		// adb滑动命令，不是adb的句柄用不到这个
		std::string m_screencap_cmd;	// adb截图命令，不是adb的句柄用不到这个
		std::string m_pullscreen_cmd;	// adb拉取截图命令，不是adb的句柄用不到这个
		const std::string m_adb_screen_filename;
		std::minstd_rand m_rand_engine;
		int m_width = 0;
		int m_height = 0;
		//int m_x_offset = 0;
		//int m_y_offset = 0;
		double m_control_scale = 1;
	};
}
