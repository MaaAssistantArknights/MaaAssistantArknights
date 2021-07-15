#include "Assistance.h"

#include "WinMacro.h"
#include "Configer.h"
#include "Identify.h"

using namespace asst;

Assistance::Assistance()
{
	Configer::reload();

	m_Ider = std::make_shared<Identify>();
	for (auto&& pair : Configer::tasksJson)
	{
		m_Ider->addImage(pair.first, Configer::getResDir() + pair.second["filename"].as_string());
	}
	m_Ider->setUseCache(Configer::optionsJson["cache"].as_boolean());

	m_working_thread = std::thread(working_proc, this);

}

Assistance::~Assistance()
{
	if (m_pWindow != nullptr) {
		m_pWindow->showWindow();
	}

	m_thread_exit = true;
	m_thread_running = false;
	m_condvar.notify_one();

	if (m_working_thread.joinable()) {
		m_working_thread.join();
	}
}

std::optional<std::string> Assistance::setSimulator(const std::string& simulator_name)
{
	stop();

	auto create_handles = [&](const std::string name) -> bool {
		m_pWindow = std::make_shared<WinMacro>(name, HandleType::Window);
		m_pView = std::make_shared<WinMacro>(name, HandleType::View);
		m_pCtrl = std::make_shared<WinMacro>(name, HandleType::Control);
		return m_pWindow->captured() && m_pView->captured() && m_pCtrl->captured();
	};

	bool ret = false;
	std::string cor_name = simulator_name;

	std::unique_lock<std::mutex> lock(m_mutex);

	if (simulator_name.empty()) {
		for (auto&& [name, value] : Configer::handleJson)
		{
			ret = create_handles(name);
			if (ret) {
				cor_name = name;
				break;
			}
		}
	}
	else {
		ret = create_handles(simulator_name);
	}
	if (ret && m_pWindow->resizeWindow()) {
		m_inited = true;
		return cor_name;
	}
	else {
		m_inited = false;
		return std::nullopt;
	}
}

void Assistance::start(const std::string& task)
{
	if (m_thread_running || !m_inited) {
		return;
	}

	std::unique_lock<std::mutex> lock(m_mutex);
	m_next_tasks.clear();
	m_next_tasks.emplace_back(task);
	m_thread_running = true;
	m_condvar.notify_one();
}

void Assistance::stop()
{
	std::unique_lock<std::mutex> lock(m_mutex);

	m_thread_running = false;
	m_next_tasks.clear();
}

void Assistance::working_proc(Assistance* pThis)
{
	while (!pThis->m_thread_exit) {
		std::unique_lock<std::mutex> lock(pThis->m_mutex);
		if (pThis->m_thread_running) {
			auto curImg = pThis->m_pView->getImage(pThis->m_pView->getWindowRect());
			
			std::string matched_task;
			Rect matched_rect;
			for (auto&& task_jstr : pThis->m_next_tasks) {
				std::string task_name = task_jstr.as_string();
				double threshold = Configer::tasksJson[task_name]["threshold"].as_double();
				auto&& [algorithm, value, rect] = pThis->m_Ider->findImage(curImg, task_name, threshold);
				DebugTrace("%-20s %f", task_name.c_str(), value);
				if ( algorithm == 0 ||
					(algorithm == 1 && value >= threshold)
					|| (algorithm == 2 && value >= 0.9999)) {
					matched_task = task_name;
					matched_rect = rect;
					break;
				}
			}

			if (!matched_task.empty()) {
				auto task = Configer::tasksJson[matched_task].as_object();
				std::string opType = task["type"].as_string();
				DebugTraceInfo("Matched: %s, type: %s", matched_task.c_str(), opType.c_str());

				if (opType == "clickSelf") {
					pThis->m_pCtrl->clickRange(matched_rect);
				}
				else if (opType == "clickRand") {
					pThis->m_pCtrl->clickRange(pThis->m_pCtrl->getWindowRect());
				}
				else if (opType == "doNothing") {
					// do nothing
				}
				else if (opType == "stop") {
					DebugTrace("opType == stop");
					pThis->m_thread_running = false;
					pThis->m_next_tasks.clear();
					continue;
				}
				else {
					DebugTraceError("Unknow option type: %s", opType.c_str());
				}

				pThis->m_next_tasks = Configer::tasksJson[matched_task]["next"].as_array();
				DebugTrace("Next: %s", pThis->m_next_tasks.to_string().c_str());
			}
			pThis->m_condvar.wait_for(lock, std::chrono::milliseconds(Configer::optionsJson["delay"]["fixedTime"].as_integer()));
		}
		else {
			pThis->m_condvar.wait(lock);
		}
	}
}