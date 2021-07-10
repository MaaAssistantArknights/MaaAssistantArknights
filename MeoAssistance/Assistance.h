#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <queue>

#include "AssDef.h"

namespace MeoAssistance {
	class WinMacro;

	class Assistance
	{
	public:
		Assistance();
		~Assistance();

		bool setSimulatorType(SimulatorType type);
		void start();
		// void pause();
		void stop();

	private:
		static void control_function(Assistance* pThis);	// 控制线程，消费者
		static void identify_function(Assistance* pThis);	// 识别线程，生产者

		std::shared_ptr<WinMacro> m_pWinmarco = nullptr;
		std::queue<PointRange> m_click_queue;
		std::mutex m_queue_mutex;

		std::thread m_control_thread;
		std::thread m_identify_thread;
		bool m_control_exit = false;
		bool m_identify_exit = false;
		bool m_control_running = false;
		bool m_identify_running = false;
		std::condition_variable m_control_cv;
		std::condition_variable m_identify_cv;
	};

}