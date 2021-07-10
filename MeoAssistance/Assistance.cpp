#include "Assistance.h"

#include "WinMacro.h"
#include "Configer.h"
#include "Identify.h"

using namespace MeoAssistance;

Assistance::Assistance()
{
	Configer::reload();

	m_Ider = std::make_shared<Identify>();
	for (auto&& pair : Configer::dataObj)
	{
		m_Ider->addImage(pair.first, Configer::getResDir() + pair.second["filename"].as_string());
	}

	m_control_thread = std::thread(control_function, this);
	m_identify_thread = std::thread(identify_function, this);

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
	std::queue<Rect> empty;
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
			auto curImg = pThis->m_pView->getImage(pThis->m_pView->getWindowRect());

			std::pair<std::string, json::value> matched;
			double max_similarity = 0;
			for (auto&& pair : Configer::dataObj) {
				double similarity = pThis->m_Ider->imgHistComp(curImg, pair.first, jsonToRect(pair.second["viewRect"].as_array()));
#ifdef _DEBUG
				std::cout << pair.first << " Similarity: " << similarity << std::endl;
#endif
				if (similarity >= pair.second["similarity"].as_double()) {
					if (similarity > max_similarity) {
						max_similarity = similarity;
						matched = pair;
					}
				}
			}
			if (max_similarity != 0) {
#ifdef _DEBUG
				std::cout << "Max: " << matched.first << " , Similarity: " << max_similarity << std::endl;
#endif
				std::string opType = matched.second["type"].as_string();
				if (opType == "click") {
					pThis->m_tasks.emplace(jsonToRect(matched.second["ctrlRect"].as_array()));
				}
				else if (opType == "stop") {
#ifdef _DEBUG
					std::cout << "opType == stop " << std::endl;
#endif
					pThis->m_control_running = false;
					pThis->m_identify_running = false;
					continue;
				}
				else if (opType == "donothing") {
					pThis->m_identify_cv.wait_for(lock, std::chrono::milliseconds(1000));
					continue;
				}
				pThis->m_control_cv.notify_all();
			}
#ifdef _DEBUG
			std::cout << std::endl;
#endif
			pThis->m_identify_cv.wait_for(lock, std::chrono::milliseconds(1000));
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
			const Rect rect = pThis->m_tasks.front();
			pThis->m_tasks.pop();
			lock.unlock();

			pThis->m_pCtrl->clickRange(rect);
		}
		else {
			pThis->m_control_cv.wait(lock);
		}
	}
}