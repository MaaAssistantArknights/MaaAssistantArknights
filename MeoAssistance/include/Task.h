#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <unordered_map>
#include <ostream>

#include "AsstDef.h"
#include "AsstAux.h"

namespace cv
{
	class Mat;
}
namespace json
{
	class value;
}

namespace asst {

	class WinMacro;
	class Identify;
	class Configer;
	class RecruitConfiger;

	enum class TaskMsg {
		/* Error Msg */
		PtrIsNull,
		ImageIsEmpty,
		WindowMinimized,
		/* Info Msg */
		TaskMatched,
		ReachedLimit,
		ReadyToSleep,
		AppendTask,
		TaskCompleted,
		MissionStop,
		/* Open Recruit Msg */
		DetectText,
		DetectTags,
		RecruitCombs
	};
	static std::ostream& operator<<(std::ostream& os, const TaskMsg& type)
	{
		static const std::unordered_map<TaskMsg, std::string> _type_name = {
			{TaskMsg::PtrIsNull, "PtrIsNull"},
			{TaskMsg::ImageIsEmpty, "ImageIsEmpty"},
			{TaskMsg::WindowMinimized, "WindowMinimized"},
			{TaskMsg::TaskMatched, "TaskMatched"},
			{TaskMsg::ReachedLimit, "ReachedLimit"},
			{TaskMsg::ReadyToSleep, "ReadyToSleep"},
			{TaskMsg::AppendTask, "AppendTask"},
			{TaskMsg::TaskCompleted, "TaskCompleted"},
			{TaskMsg::MissionStop, "MissionStop"},
			{TaskMsg::DetectText, "DetectText"},
			{TaskMsg::DetectTags, "DetectTags"},
			{TaskMsg::RecruitCombs, "RecruitCombs"}
		};
		return os << _type_name.at(type);
	}

	using TaskCallback = std::function<void(TaskMsg, const json::value&, void*)>;

	class AbstractTask
	{
	public:
		AbstractTask(TaskCallback callback, void* callback_arg);
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

		TaskCallback m_callback;
		void* m_callback_arg = NULL;
		bool* m_exit_flag = NULL;
	};

	class MatchTask : public AbstractTask
	{
	public:
		MatchTask(TaskCallback callback, void* callback_arg,
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

	class OcrAbstractTask : public AbstractTask
	{
	public:
		OcrAbstractTask(TaskCallback callback, void* callback_arg);
		virtual bool run() override = 0 ;

	protected:
		std::vector<TextArea> ocr_detect();

		template<typename FilterArray, typename ReplaceMap> 
		std::vector<TextArea> text_filter(const std::vector<TextArea>& src,
			const FilterArray& filter_array, const ReplaceMap& replace_map)
		{
			std::vector<TextArea> dst;
			for (const TextArea& text_area : src) {
				TextArea temp = text_area;
				for (const auto& [old_str, new_str] : replace_map) {
					temp.text = StringReplaceAll(temp.text, old_str, new_str);
				}
				for (const auto& text : filter_array) {
					if (temp.text == text) {
						dst.emplace_back(std::move(temp));
					}
				}
			}
			return dst;
		}
	};

	class OpenRecruitTask : public OcrAbstractTask
	{
	public:
		OpenRecruitTask(TaskCallback callback, void* callback_arg,
			Configer * configer_ptr, RecruitConfiger * recruit_configer_ptr);

		virtual bool run() override;
		virtual void set_param(std::vector<int> required_level, bool set_time = true);

	protected:
		std::vector<int> m_required_level;
		bool m_set_time = false;
		Configer* m_configer_ptr = NULL;
		RecruitConfiger* m_recruit_configer_ptr = NULL;
	};
}
