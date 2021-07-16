#include "Assistance.h"

#include "WinMacro.h"
#include "Configer.h"
#include "Identify.h"

using namespace asst;

Assistance::Assistance()
{
	Configer::reload();

	m_pIder = std::make_shared<Identify>();
	for (auto&& [name, info] : Configer::m_tasks)
	{
		m_pIder->addImage(name, Configer::getResDir() + info.filename);
	}
	m_pIder->setUseCache(Configer::m_options.cache);

	m_working_thread = std::thread(workingProc, this);

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

	auto create_handles = [&](const std::string& name) -> bool {
		m_pWindow = std::make_shared<WinMacro>(name, HandleType::Window);
		m_pView = std::make_shared<WinMacro>(name, HandleType::View);
		m_pCtrl = std::make_shared<WinMacro>(name, HandleType::Control);
		return m_pWindow->captured() && m_pView->captured() && m_pCtrl->captured();
	};

	bool ret = false;
	std::string cor_name = simulator_name;

	std::unique_lock<std::mutex> lock(m_mutex);

	if (simulator_name.empty()) {
		for (auto&& [name, value] : Configer::m_handles)
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
	m_pIder->clear_cache();
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
	m_pIder->clear_cache();
}

bool Assistance::setParam(const std::string& type, const std::string& param, const std::string& value)
{
	return Configer::setParam(type, param, value);
}

void Assistance::workingProc(Assistance* pThis)
{
	while (!pThis->m_thread_exit) {
		std::unique_lock<std::mutex> lock(pThis->m_mutex);
		if (pThis->m_thread_running) {
			auto curImg = pThis->m_pView->getImage(pThis->m_pView->getWindowRect());

			std::string matched_task;
			Rect matched_rect;
			for (auto&& task_name : pThis->m_next_tasks) {
				double threshold = Configer::m_tasks[task_name].threshold;
				auto&& [algorithm, value, rect] = pThis->m_pIder->findImage(curImg, task_name, threshold);
				DebugTrace("%-20s Type:%d, Value:%f", task_name.c_str(), algorithm, value);
				if (algorithm == 0 ||
					(algorithm == 1 && value >= threshold)
					|| (algorithm == 2 && value >= 0.9999)) {
					matched_task = task_name;
					matched_rect = rect;
					break;
				}
			}

			if (!matched_task.empty()) {
				auto task = Configer::m_tasks[matched_task];
				DebugTraceInfo("Matched: %s, Type: %d", matched_task.c_str(), task.type);

				switch (task.type) {
				case TaskType::ClickSelf:
					if (task.exec_times >= task.max_times) {
						pThis->m_thread_running = false;
						pThis->m_next_tasks.clear();
						continue;
					}
					pThis->m_pCtrl->clickRange(matched_rect);
					++task.exec_times;
					break;
				case TaskType::ClickRand:
					pThis->m_pCtrl->clickRange(pThis->m_pCtrl->getWindowRect());
					break;
				case TaskType::DoNothing:
					break;
				case TaskType::Stop:
					DebugTrace("TaskType is Stop");
					pThis->m_thread_running = false;
					pThis->m_next_tasks.clear();
					continue;
					break;
				default:
					DebugTraceError("Unknown option type: %d", task.type);
					break;
				}

				pThis->m_next_tasks = Configer::m_tasks[matched_task].next;
				std::string nexts;
				for (auto&& name : pThis->m_next_tasks) {
					nexts += name + ",";
				}
				if (nexts.back() == ',') {
					nexts.pop_back();
				}
				DebugTrace("Next: %s", nexts.c_str());
			}
			pThis->m_condvar.wait_for(lock, std::chrono::milliseconds(Configer::m_options.delayFixedTime));
		}
		else {
			pThis->m_condvar.wait(lock);
		}
	}
}
