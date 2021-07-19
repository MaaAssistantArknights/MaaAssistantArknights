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
		m_pIder->addImage(name, GetResourceDir() + info.filename);
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
	if (ret && m_pWindow->showWindow() && m_pWindow->resizeWindow()) {
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

	Configer::clearExecTimes();
	m_pIder->clear_cache();
	m_next_tasks.clear();
	m_next_tasks.emplace_back(task);
	m_thread_running = true;
	m_condvar.notify_one();
}

void Assistance::stop(bool block)
{
	std::unique_lock<std::mutex> lock;
	if (block) { // 外部调用
		lock = std::unique_lock<std::mutex>(m_mutex);
		Configer::clearExecTimes();
	}
	m_thread_running = false;
	m_next_tasks.clear();
	m_pIder->clear_cache();
}

bool Assistance::setParam(const std::string& type, const std::string& param, const std::string& value)
{
	return Configer::setParam(type, param, value);
}

std::optional<std::string> Assistance::getParam(const std::string& type, const std::string& param)
{
	return Configer::getParam(type, param);
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
				auto&& task = Configer::m_tasks[task_name];
				double threshold = task.threshold;
				auto&& [algorithm, value, rect] = pThis->m_pIder->findImage(curImg, task_name, threshold);
				DebugTrace(task_name, "Type:", algorithm, "Value:", value);
				if (algorithm == 0 ||
					(algorithm == 1 && value >= threshold)
					|| (algorithm == 2 && value >= task.cache_threshold)) {
					matched_task = task_name;
					matched_rect = rect;
					break;
				}
			}

			if (!matched_task.empty()) {
				auto&& task = Configer::m_tasks[matched_task];
				DebugTraceInfo("***Matched***", matched_task, "Type:", static_cast<int>(task.type));
				if (task.pre_delay > 0) {
					DebugTrace("PreDelay", task.pre_delay);
					std::this_thread::sleep_for(std::chrono::milliseconds(task.pre_delay));
				}

				if (task.max_times != INT_MAX) {
					DebugTrace("CurTimes:", task.exec_times, "MaxTimes:", task.max_times);
				}
				if (task.exec_times < task.max_times) {
					switch (task.type) {
					case TaskType::ClickRect:
						matched_rect = task.specific_area;
					case TaskType::ClickSelf:
						pThis->m_pCtrl->clickRange(matched_rect);
						break;
					case TaskType::ClickRand:
						pThis->m_pCtrl->clickRange(pThis->m_pCtrl->getWindowRect());
						break;
					case TaskType::DoNothing:
						break;
					case TaskType::Stop:
						DebugTrace("TaskType is Stop");
						pThis->stop(false);
						continue;
						break;
					default:
						DebugTraceError("Unknown option type:", static_cast<int>(task.type));
						break;
					}
					++task.exec_times;
					if (task.rear_delay > 0) {
						DebugTrace("RearDelay", task.rear_delay);
						std::this_thread::sleep_for(std::chrono::milliseconds(task.rear_delay));
					}
					pThis->m_next_tasks = Configer::m_tasks[matched_task].next;
				}
				else {
					DebugTraceInfo("Reached limit");
					pThis->m_next_tasks = Configer::m_tasks[matched_task].exceeded_next;
				}

				std::string nexts_str;
				for (auto&& name : pThis->m_next_tasks) {
					nexts_str += name + ",";
				}
				if (nexts_str.back() == ',') {
					nexts_str.pop_back();
				}
				DebugTrace("Next:", nexts_str);
			}
			pThis->m_condvar.wait_for(lock, std::chrono::milliseconds(Configer::m_options.delayFixedTime));
		}
		else {
			pThis->m_condvar.wait(lock);
		}
	}
}
