#include "Assistance.h"

#include "WinMacro.h"

using namespace MeoAssistance;

Assistance::Assistance()
	: m_control_thread(control_function, this),
	m_identify_thread(identify_function, this)
{
}

Assistance::~Assistance()
{
	m_control_exit = true;
	m_control_running = false;
	m_control_cv.notify_all();

	m_identify_exit = true;
	m_identify_running = false;
	m_identify_cv.notify_all();

	if (m_control_thread.joinable()) {
		m_control_thread.join();
	}
	if (m_identify_thread.joinable()) {
		m_identify_thread.join();
	}
}

bool Assistance::setSimulatorType(SimulatorType type)
{
	stop();

	std::unique_lock<std::mutex> lock(m_tasks_mutex);
	std::queue<Task> empty;
	m_tasks.swap(empty);
	int int_type = static_cast<int>(type);
	m_pCtrl = std::make_shared<WinMacro>(static_cast<HandleType>(int_type | static_cast<int>(HandleType::Control)));
	m_pWindow = std::make_shared<WinMacro>(static_cast<HandleType>(int_type | static_cast<int>(HandleType::Window)));
	m_pView = std::make_shared<WinMacro>(static_cast<HandleType>(int_type | static_cast<int>(HandleType::View)));
	return m_pCtrl->findHandle() && m_pWindow->findHandle() && m_pView->findHandle();
}

void Assistance::start()
{
	std::unique_lock<std::mutex> lock(m_tasks_mutex);

	m_control_running = true;
	m_identify_running = true;
	m_control_cv.notify_all();
	m_identify_cv.notify_all();
}

void Assistance::stop()
{
	std::unique_lock<std::mutex> lock(m_tasks_mutex);

	m_control_running = false;
	m_identify_running = false;
}

void Assistance::identify_function(Assistance* pThis)
{
	while (!pThis->m_identify_exit) {
		std::unique_lock<std::mutex> lock(pThis->m_tasks_mutex);
		if (pThis->m_identify_running) {
			pThis->m_pView->getImage({ 0, 0, 1000, 1000 });
			pThis->m_tasks.emplace(Task::StartButton1);
			pThis->m_control_cv.notify_all();
			pThis->m_identify_cv.wait_for(lock, std::chrono::milliseconds(3000));
		}
		else {
			pThis->m_identify_cv.wait(lock);
		}
	}
}

void Assistance::control_function(Assistance* pThis)
{
	pThis->m_pWindow->resizeWindow(1200, 720);

	while (!pThis->m_control_exit) {
		std::unique_lock<std::mutex> lock(pThis->m_tasks_mutex);
		if (pThis->m_control_running && !pThis->m_tasks.empty()) {
			const Task task = pThis->m_tasks.front();
			pThis->m_tasks.pop();
			lock.unlock();

			pThis->run_task(task);
		}
		else {
			pThis->m_control_cv.wait(lock);
		}
	}
}

bool Assistance::run_task(Task task)
{
	switch (task)
	{
	case MeoAssistance::Assistance::Task::StartButton1:
		return m_pCtrl->click({ 1060, 600 });
		break;
	case MeoAssistance::Assistance::Task::StartButton2:
		break;
	default:
		break;
	}
}