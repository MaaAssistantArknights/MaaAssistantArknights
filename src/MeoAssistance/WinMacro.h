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

#include "AsstDef.h"

namespace cv {
	class Mat;
}

namespace asst {
	class WinMacro
	{
	public:
		WinMacro(const EmulatorInfo& info);
		~WinMacro();

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

		const std::string ScreenFilename;
	private:
		void pipe_working_proc();
		bool find_handle(); // TODO，可以考虑也做成异步的
		std::optional<std::string> call_command(const std::string& cmd, bool use_pipe = true);
		Point rand_point_in_rect(const Rect& rect);

		int push_cmd(const std::string& cmd);
		void wait(unsigned id);

		bool m_thread_exit = false;
		std::thread m_cmd_thread;
		std::mutex m_cmd_queue_mutex;
		std::condition_variable m_cmd_condvar;
		std::queue<std::string> m_cmd_queue;
		std::atomic<unsigned> m_completed_id = 0;
		unsigned m_push_id = 0;						// push_id的自增总是伴随着queue的push，肯定是要上锁的，所以没必要原子

		std::shared_mutex m_image_mutex;
		std::shared_ptr<cv::Mat> m_cache_image;

		EmulatorInfo m_emulator_info;
		HWND m_handle = nullptr;
		std::minstd_rand m_rand_engine;
		std::pair<int, int> m_scale_size;
		double m_control_scale = 1.0;
	};
}
