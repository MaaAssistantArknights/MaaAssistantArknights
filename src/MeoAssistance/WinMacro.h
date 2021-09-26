#pragma once

#include <string>
#include <random>
#include <optional>

#include <memory>	// for pimpl
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <queue>
#include <condition_variable>

#include <Windows.h>

#include <opencv2/opencv.hpp>

#include "AsstDef.h"

namespace asst {
	class WinMacro
	{
	public:
		WinMacro();
		~WinMacro();

		bool try_capture(const EmulatorInfo& info, bool without_handle = false);
		//void set_idle(bool flag);

		cv::Mat get_image(bool raw = false);

		// 点击和滑动都是异步执行，返回该任务的id
		int click(const Point& p, bool block = true);
		int click(const Rect& rect, bool block = true);
		int click_without_scale(const Point& p, bool block = true);
		int click_without_scale(const Rect& rect, bool block = true);
		int swipe(const Point& p1, const Point& p2, int duration = 0, bool block = true);
		int swipe(const Rect& r1, const Rect& r2, int duration = 0, bool block = true);
		int swipe_without_scale(const Point& p1, const Point& p2, int duration = 0, bool block = true);
		int swipe_without_scale(const Rect& r1, const Rect& r2, int duration = 0, bool block = true);

		// 异形屏矫正
		Rect shaped_correct(const Rect& rect) const;
	private:
		void pipe_working_proc();
		std::vector<unsigned char> call_command(const std::string& cmd);
		int push_cmd(const std::string& cmd);
		void wait(unsigned id) const noexcept;
		bool screencap();
		Point rand_point_in_rect(const Rect& rect);

		// 转换data中所有的crlf为lf：有些模拟器自带的adb，exec-out输出的\n，会被替换成\r\n，导致解码错误，所以这里转一下回来（点名批评mumu）
		static void convert_lf(std::vector<unsigned char>& data);

		bool m_thread_exit = false;
		//bool m_thread_idle = true;
		std::thread m_cmd_thread;
		std::mutex m_cmd_queue_mutex;
		std::condition_variable m_cmd_condvar;
		std::queue<std::string> m_cmd_queue;
		std::atomic<unsigned> m_completed_id = 0;
		unsigned m_push_id = 0;						// push_id的自增总是伴随着queue的push，肯定是要上锁的，所以没必要原子

		//std::shared_mutex m_image_mutex;
		cv::Mat m_cache_image;
		bool m_image_convert_lf = false;

		constexpr static int PipeBuffSize = 1048576;	// 管道缓冲区大小
		HANDLE m_pipe_read = nullptr;					// 读管道句柄
		HANDLE m_pipe_write = nullptr;					// 写管道句柄
		HANDLE m_pipe_child_read = nullptr;				// 子进程的读管道句柄
		HANDLE m_pipe_child_write = nullptr;			// 子进程的写管道句柄
		SECURITY_ATTRIBUTES m_pipe_sec_attr = { 0 };	// 管道安全描述符
		STARTUPINFOA m_child_startup_info = { 0 };		// 子进程启动信息

		EmulatorInfo m_emulator_info;
		HWND m_handle = nullptr;
		std::minstd_rand m_rand_engine;
		std::pair<int, int> m_scale_size;
		double m_control_scale = 1.0;
	};
}
