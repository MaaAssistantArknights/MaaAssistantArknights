#pragma once

#include <memory>

#include "AsstMsg.h"

namespace cv
{
	class Mat;
}

namespace asst {
	class WinMacro;
	class Identify;

	enum TaskType {
		TaskTypeInvalid = 0,
		TaskTypeWindowZoom = 1,
		TaskTypeRecognition = 2,
		TaskTypeClick = 4
	};

	// 设施类型
	enum class FacilityType {
		Invalid,
		Manufacturing,		// 制造站
		Trade,				// 贸易站
		PowerStation,		// 发电站
		ControlInterface,	// 控制中枢
		ReceptionRoom,		// 会客室
		Dormitory,			// 宿舍
		Office				// 办公室
		// 训练室和加工站用不上，不加了
	};

	class AbstractTask
	{
	public:
		AbstractTask(AsstCallback callback, void* callback_arg);
		virtual ~AbstractTask() = default;
		AbstractTask(const AbstractTask&) = default;
		AbstractTask(AbstractTask&&) = default;

		virtual void set_ptr(
			std::shared_ptr<WinMacro> window_ptr,
			std::shared_ptr<WinMacro> view_ptr,
			std::shared_ptr<WinMacro> control_ptr,
			std::shared_ptr<Identify> identify_ptr);
		virtual bool run() = 0;

		virtual void set_exit_flag(bool* exit_flag);
		virtual int get_task_type() { return m_task_type; }
		virtual void set_retry_times(int times) { m_retry_times = times; }
		virtual int get_retry_times() { return m_retry_times; }
		virtual void set_task_chain(std::string name) { m_task_chain = std::move(name); }
		virtual const std::string& get_task_chain() { return m_task_chain; }
	protected:
		virtual cv::Mat get_format_image(bool hd = false);	// 参数hd：高清截图，会慢一点
		virtual bool set_control_scale(double scale);
		virtual bool set_control_scale(int cur_width, int cur_height);
		virtual bool sleep(unsigned millisecond);
		virtual bool print_window(const std::string& dir);

		std::shared_ptr<WinMacro> m_window_ptr = nullptr;
		std::shared_ptr<WinMacro> m_view_ptr = nullptr;
		std::shared_ptr<WinMacro> m_control_ptr = nullptr;
		std::shared_ptr<Identify> m_identify_ptr = nullptr;

		AsstCallback m_callback;
		void* m_callback_arg = NULL;
		bool* m_exit_flag = NULL;
		std::string m_task_chain;
		int m_task_type = TaskType::TaskTypeInvalid;
		int m_retry_times = INT_MAX;
	};
}