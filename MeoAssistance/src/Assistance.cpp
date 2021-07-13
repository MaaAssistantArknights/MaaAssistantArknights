#include "Assistance.h"

#include "WinMacro.h"
#include "Configer.h"
#include "Identify.h"

using namespace asst;

Assistance::Assistance()
{
	Configer::reload();

	m_Ider = std::make_shared<Identify>();
	for (auto&& pair : Configer::dataObj)
	{
		m_Ider->addImage(pair.first, Configer::getResDir() + pair.second["filename"].as_string());
	}

	m_working_thread = std::thread(working_proc, this);

}

Assistance::~Assistance()
{
	m_pWindow->showWindow();

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
		for (auto&& [name, value] : Configer::handleObj)
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
		return cor_name;
	}
	else {
		return std::nullopt;
	}
}

void Assistance::start()
{
	std::unique_lock<std::mutex> lock(m_mutex);

	m_thread_running = true;
	m_condvar.notify_one();
}

void Assistance::stop()
{
	std::unique_lock<std::mutex> lock(m_mutex);

	m_thread_running = false;
}

void Assistance::working_proc(Assistance* pThis)
{
	while (!pThis->m_thread_exit) {
		std::unique_lock<std::mutex> lock(pThis->m_mutex);
		if (pThis->m_thread_running) {
			auto curImg = pThis->m_pView->getImage(pThis->m_pView->getWindowRect());

			double max_value = -1;
			Rect cor_rect;
			std::pair<std::string, json::value> matched;
			for (auto&& pair : Configer::dataObj) {
				auto&& [value, rect] = pThis->m_Ider->findImage(curImg, pair.first);
				DebugTrace("%-20s %f", pair.first.c_str(), value);
				if (value >= pair.second["threshold"].as_double()) {
					if (value >= max_value) {
						max_value = value;
						cor_rect = rect;
						matched = pair;
					}
				}
			}
			if (max_value >= 0) {
				auto task = Configer::dataObj[matched.first].as_object();
				std::string opType = task["type"].as_string();
				DebugTraceInfo("Matched: %s, type: %s", matched.first.c_str(), opType.c_str());
				bool next_loop = true;
				while (next_loop) {
					if (opType == "clickSelf") {
						pThis->m_pCtrl->clickRange(cor_rect);
					}
					else if (opType == "clickRand") {
						pThis->m_pCtrl->clickRange(pThis->m_pCtrl->getWindowRect());
					}
					else if (opType == "next") {
						json::value nextObj = task["next"];
						std::string name = nextObj["name"].as_string();
						std::string filepath = Configer::getResDir() + nextObj["filename"].as_string();
						auto&& [value, rect] = pThis->m_Ider->findImageWithFile(curImg, filepath);
						DebugTrace("Next: %-20s %f", name.c_str(), value);

						if (value >= nextObj["threshold"].as_double()) {
							DebugTraceInfo("Next matched: %s", name.c_str());
							task = nextObj.as_object();
							opType = nextObj["type"].as_string();
							cor_rect = rect;
							max_value = value;
							next_loop = true;
							continue;
						}
						else {
							DebugTraceInfo("Next not matched: %s", name.c_str());
						}
					}
					else if (opType == "stop") {
						DebugTrace("opType == stop");
						pThis->m_thread_running = false;
						break;
					}
					else if (opType == "doNothing") {
						// do nothing
					}
					else {
						DebugTraceError("Unknow option type: %s", opType.c_str());
					}
					next_loop = false;
				}
			}
			pThis->m_condvar.wait_for(lock, std::chrono::milliseconds(Configer::optionsObj["delay"].as_integer()));
		}
		else {
			pThis->m_condvar.wait(lock);
		}
	}
}