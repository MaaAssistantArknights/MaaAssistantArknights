#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <optional>
#include <unordered_map>
#include <queue>

#include "AsstPort.h"
#include "AsstDef.h"
#include "Configer.h"
#include "RecruitConfiger.h"
#include "Task.h"

namespace cv {
	class Mat;
}

namespace asst {
	class WinMacro;
	class Identify;
	class AbstractTask;
	enum class TaskMsg;

	class Assistance
	{
	public:
		Assistance();
		~Assistance();

		std::optional<std::string> catch_emulator(const std::string& emulator_name = std::string());

		void start(const std::string& task);

		void stop(bool block = true);

		bool set_param(const std::string& type, const std::string& param, const std::string& value);
		std::optional<std::string> get_param(const std::string& type, const std::string& param);

		bool print_window(const std::string& filename, bool block = true);

		// 计算公开招募，需要当前处在选择公招Tag的页面
		// 参数：
		// required_level：需要的等级，最优组合的最低等级，在required_level中，才会进行点击
		// set_time：是否自动设置时间（9小时）
		// 返回值：
		// std::vector< std::pair < Tags名vector，这个Tags组合可能出的干员组合 > >
		std::optional<std::vector<std::pair<std::vector<std::string>, OperCombs>>> 
			open_recruit(const std::vector<int>& required_level, bool set_time = true);
	private:
		static void working_proc(Assistance* pThis);
		static void task_callback(TaskMsg msg, const json::value& detail, void* custom_arg);

		void append_match_task(const std::vector<std::string>& tasks);
		cv::Mat get_format_image();
		void set_control_scale(int cur_width, int cur_height);
		void clear_exec_times();

		// for debug
		bool find_text_and_click(const std::string& text, bool block = true);

		std::shared_ptr<WinMacro> m_window_ptr = nullptr;
		std::shared_ptr<WinMacro> m_view_ptr = nullptr;
		std::shared_ptr<WinMacro> m_control_ptr = nullptr;
		std::shared_ptr<Identify> m_identify_ptr = nullptr;
		bool m_inited = false;

		std::thread m_working_thread;
		std::mutex m_mutex;
		std::condition_variable m_condvar;
		bool m_thread_exit = false;
		bool m_thread_idle = true;
		std::queue<std::shared_ptr<AbstractTask>> m_tasks_queue;

		std::unordered_map<std::string, TaskInfo> m_all_tasks_info;
		Configer m_configer;
		RecruitConfiger m_recruit_configer;
	};

}