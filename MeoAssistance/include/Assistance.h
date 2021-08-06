#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <optional>
#include <unordered_map>
#include <queue>

#include "AsstDef.h"
#include "Task.h"

namespace cv {
	class Mat;
}

namespace asst {
	class WinMacro;
	class Identify;

	class Assistance
	{
	public:
		Assistance(TaskCallback callback = nullptr, void * callback_arg = nullptr);
		~Assistance();

		std::optional<std::string> catch_emulator(const std::string& emulator_name = std::string());

		// 开始刷理智
		void start_sanity();
		// 开始访问基建
		void start_visit();
		// 开始公开招募操作
		void start_open_recruit(const std::vector<int>& required_level, bool set_time = true);
		// 开始匹配任务，调试用
		void start_match_task(const std::string& task, bool block = true);

		void stop(bool block = true);

		bool set_param(const std::string& type, const std::string& param, const std::string& value);

	private:
		static void working_proc(Assistance* p_this);
		static void msg_proc(Assistance* p_this);
		static void task_callback(TaskMsg msg, const json::value& detail, void* custom_arg);

		void append_match_task(const std::vector<std::string>& tasks);
		void append_task(const json::value& detail);
		void clear_exec_times();

		std::shared_ptr<WinMacro> m_window_ptr = nullptr;
		std::shared_ptr<WinMacro> m_view_ptr = nullptr;
		std::shared_ptr<WinMacro> m_control_ptr = nullptr;
		std::shared_ptr<Identify> m_identify_ptr = nullptr;
		bool m_inited = false;

		bool m_thread_exit = false;
		std::queue<std::shared_ptr<AbstractTask>> m_tasks_queue;
		TaskCallback m_callback = nullptr;
		void* m_callback_arg = nullptr;

		bool m_thread_idle = true;
		std::thread m_working_thread;
		std::mutex m_mutex;
		std::condition_variable m_condvar;

		std::thread m_msg_thread;
		std::queue<std::pair<TaskMsg, json::value>> m_msg_queue;
		std::mutex m_msg_mutex;
		std::condition_variable m_msg_condvar;


	};

}