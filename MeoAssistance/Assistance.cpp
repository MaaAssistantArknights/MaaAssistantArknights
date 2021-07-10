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

	std::unique_lock<std::mutex> lock(m_queue_mutex);
	std::queue<PointRange> empty;
	m_click_queue.swap(empty);
	m_pWinmarco = std::make_shared<WinMacro>(type);
	return m_pWinmarco->findHandle();
}

void Assistance::start()
{
	std::unique_lock<std::mutex> lock(m_queue_mutex);

	m_control_running = true;
	m_identify_running = true;
	m_control_cv.notify_one();
	m_identify_cv.notify_one();
}

void Assistance::stop()
{
	std::unique_lock<std::mutex> lock(m_queue_mutex);

	m_control_running = false;
	m_identify_running = false;
}

void Assistance::control_function(Assistance* pThis)
{
	while (!pThis->m_control_exit) {
		std::unique_lock<std::mutex> lock(pThis->m_queue_mutex);

		if (pThis->m_control_running && !pThis->m_click_queue.empty()) {
			auto point = pThis->m_click_queue.front();
			pThis->m_click_queue.pop();
			lock.unlock();

			pThis->m_pWinmarco->clickRange(point);
		}
		else {
			pThis->m_control_cv.wait(lock);
		}
	}
}

void Assistance::identify_function(Assistance* pThis)
{
	while (!pThis->m_identify_exit) {
		std::unique_lock<std::mutex> lock(pThis->m_queue_mutex);
		if (pThis->m_identify_running) {

			pThis->m_click_queue.emplace(1060, 600, 0, 0);
			pThis->m_control_cv.notify_all();
			lock.unlock();

			std::this_thread::sleep_for(std::chrono::seconds(3));
		}
		else {
			pThis->m_identify_cv.wait(lock);
		}
	}
}