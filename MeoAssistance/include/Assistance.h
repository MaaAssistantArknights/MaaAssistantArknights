#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <optional>
#include <unordered_map>

#include "AsstDef.h"
#include "Configer.h"

namespace asst {
	class WinMacro;
	class Identify;

	class __declspec(dllexport) Assistance
	{
	public:
		Assistance();
		~Assistance();

		std::optional<std::string> setSimulator(const std::string & simulator_name = std::string());
		
		void start(const std::string & task);
		void stop(bool block = true);

		bool set_param(const std::string& type, const std::string& param, const std::string& value);
		std::optional<std::string> get_param(const std::string& type, const std::string& param);
	private:
		static void workingProc(Assistance* pThis);

		std::shared_ptr<WinMacro> m_pWindow = nullptr;
		std::shared_ptr<WinMacro> m_pView = nullptr;
		std::shared_ptr<WinMacro> m_pCtrl = nullptr;
		std::shared_ptr<Identify> m_pIder = nullptr;
		bool m_inited = false;

		std::thread m_working_thread;
		std::mutex m_mutex;
		std::condition_variable m_condvar;
		bool m_thread_exit = false;
		bool m_thread_running = false;
		std::vector<std::string> m_next_tasks;

		Configer m_configer;
	};

}