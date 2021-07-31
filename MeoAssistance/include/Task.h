#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

#include "AsstDef.h"

namespace cv
{
	class Mat;
}

namespace asst {

	class WinMacro;
	class Identify;
	class Configer;

	enum class TaskMsg {
		/* Error Msg */
		PtrIsNull,
		ImageIsEmpty,
		WindowMinimized,
		/* Info Msg */
		TaskMatched,
		ReachedLimit,
		ReadyToSleep,
		TaskCompleted,
		MissionStop
		/* Trace Msg */
		// Todo
	};

	typedef void (*TaskCallback)(TaskMsg msg, const std::string& detail_json);

	class AbstractTask
	{
	public:
		AbstractTask(TaskCallback callback);
		~AbstractTask() = default;
		AbstractTask(const AbstractTask&) = default;
		AbstractTask(AbstractTask&&) = default;

		virtual void set_ptr(
			std::shared_ptr<WinMacro> window_ptr,
			std::shared_ptr<WinMacro> view_ptr,
			std::shared_ptr<WinMacro> control_ptr,
			std::shared_ptr<Identify> identify_ptr);
		virtual bool run() = 0;

		virtual void set_exit_flag(bool* exit_flag);
	protected:
		virtual cv::Mat get_format_image();
		virtual bool set_control_scale(int cur_width, int cur_height);
		virtual void sleep(unsigned millisecond);

		std::shared_ptr<WinMacro> m_window_ptr = nullptr;
		std::shared_ptr<WinMacro> m_view_ptr = nullptr;
		std::shared_ptr<WinMacro> m_control_ptr = nullptr;
		std::shared_ptr<Identify> m_identify_ptr = nullptr;

		TaskCallback m_callback = NULL;
		bool* m_exit_flag = NULL;
	};

	class MatchTask : public AbstractTask
	{
	public:
		MatchTask(TaskCallback callback,
			std::unordered_map<std::string, TaskInfo>* all_tasks_ptr, 
			Configer* configer_ptr);

		virtual bool run() override;

		virtual void set_tasks(const std::vector<std::string>& cur_tasks_name) { m_cur_tasks_name = cur_tasks_name; }

	protected:
		bool match_image(TaskInfo * task_info, asst::Rect* matched_rect = NULL);
		void exec_click_task(TaskInfo& task, const asst::Rect& matched_rect);

		std::unordered_map<std::string, TaskInfo>* m_all_tasks_ptr = NULL;
		Configer* m_configer_ptr = NULL;
		std::vector<std::string> m_cur_tasks_name;
	};
}
