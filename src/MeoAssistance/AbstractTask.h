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

	class AbstractTask
	{
	public:
		AbstractTask(AsstCallback callback, void* callback_arg);
		virtual ~AbstractTask() = default;
		AbstractTask(const AbstractTask&) = default;
		AbstractTask(AbstractTask&&) = default;

		virtual void set_ptr(std::shared_ptr<WinMacro> controller_ptr, std::shared_ptr<Identify> identify_ptr);
		virtual bool run() = 0;

		virtual void set_exit_flag(bool* exit_flag);
		virtual void set_retry_times(int times) { m_retry_times = times; }
		virtual int get_retry_times() { return m_retry_times; }
		virtual void set_task_chain(std::string name) { m_task_chain = std::move(name); }
		virtual const std::string& get_task_chain() { return m_task_chain; }
		virtual void on_run_fails(int retry_times) { ; }
	protected:
		virtual bool sleep(unsigned millisecond);
		virtual bool print_window(const std::string& dir);
		virtual bool need_exit() const noexcept;
		virtual bool is_ptr_inited() const noexcept;

		std::shared_ptr<WinMacro> m_controller_ptr = nullptr;
		std::shared_ptr<Identify> m_identify_ptr = nullptr;

		AsstCallback m_callback;
		void* m_callback_arg = NULL;
		bool* m_exit_flag = NULL;
		std::string m_task_chain;
		int m_retry_times = INT_MAX;
	};
}