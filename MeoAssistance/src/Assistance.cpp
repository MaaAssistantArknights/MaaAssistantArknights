#include "Assistance.h"

#include "WinMacro.h"
#include "Configer.h"
#include "Identify.h"
#include "Logger.hpp"
#include "AsstAux.h"
#include "Task.h"
#include "json.h"

#include <opencv2/opencv.hpp>

#include <time.h>
#include <filesystem>

using namespace asst;

Assistance::Assistance()
{
	DebugTraceFunction;

	Configer::get_instance().load(GetResourceDir() + "config.json");
	OpenRecruitConfiger::get_instance().load(GetResourceDir() + "operInfo.json");

	m_identify_ptr = std::make_shared<Identify>();
	for (const auto& [name, info] : Configer::get_instance().m_all_tasks_info)
	{
		m_identify_ptr->add_image(name, GetResourceDir() + "template\\" + info.template_filename);
	}
	m_identify_ptr->set_use_cache(Configer::get_instance().m_options.identify_cache);

	m_identify_ptr->set_ocr_param(Configer::get_instance().m_options.ocr_gpu_index, Configer::get_instance().m_options.ocr_thread_number);
	m_identify_ptr->ocr_init_models(GetResourceDir() + "OcrLiteNcnn\\models\\");

	m_working_thread = std::thread(working_proc, this);
}

Assistance::~Assistance()
{
	DebugTraceFunction;

	//if (m_window_ptr != nullptr) {
	//	m_window_ptr->showWindow();
	//}

	m_thread_exit = true;
	m_thread_idle = true;
	m_condvar.notify_all();

	if (m_working_thread.joinable()) {
		m_working_thread.join();
	}
}

std::optional<std::string> Assistance::catch_emulator(const std::string& emulator_name)
{
	DebugTraceFunction;

	stop();

	auto create_handles = [&](const EmulatorInfo& info) -> bool {
		m_window_ptr = std::make_shared<WinMacro>(info, HandleType::Window);
		m_view_ptr = std::make_shared<WinMacro>(info, HandleType::View);
		m_control_ptr = std::make_shared<WinMacro>(info, HandleType::Control);
		return m_window_ptr->captured() && m_view_ptr->captured() && m_control_ptr->captured();
	};

	bool ret = false;
	std::string cor_name = emulator_name;

	std::unique_lock<std::mutex> lock(m_mutex);

	// 自动匹配模拟器，逐个找
	if (emulator_name.empty()) {
		for (const auto& [name, info] : Configer::get_instance().m_handles)
		{
			ret = create_handles(info);
			if (ret) {
				cor_name = name;
				break;
			}
		}
	}
	else {	// 指定的模拟器
		ret = create_handles(Configer::get_instance().m_handles[emulator_name]);
	}
	if (ret && m_window_ptr->showWindow() && m_window_ptr->resizeWindow()) {
		m_inited = true;
		return cor_name;
	}
	else {
		m_inited = false;
		return std::nullopt;
	}
}

void asst::Assistance::start_sanity()
{
	start_match_task("SanityBegin");
}

void asst::Assistance::start_visit()
{
	start_match_task("VisitBegin");
}

void Assistance::start_match_task(const std::string& task, bool block)
{
	DebugTraceFunction;
	DebugTrace("Start |", task, block ? "block" : "non block");
	if (!m_thread_idle || !m_inited) {
		return;
	}

	std::unique_lock<std::mutex> lock;
	if (block) {
		lock = std::unique_lock<std::mutex>(m_mutex);
		clear_exec_times();
		m_identify_ptr->clear_cache();
	}

	append_match_task({ task });

	m_thread_idle = false;
	m_condvar.notify_one();
}

void asst::Assistance::start_open_recruit(const std::vector<int>& required_level, bool set_time)
{
	DebugTraceFunction;
	if (!m_thread_idle || !m_inited) {
		return;
	}

	std::unique_lock<std::mutex> lock(m_mutex);

	auto task_ptr = std::make_shared<OpenRecruitTask>(task_callback, (void*)this);
	task_ptr->set_param(required_level, set_time);
	m_tasks_queue.emplace(task_ptr);

	m_thread_idle = false;
	m_condvar.notify_one();
}

void Assistance::stop(bool block)
{
	DebugTraceFunction;
	DebugTrace("Stop |", block ? "block" : "non block");

	m_thread_idle = true;

	std::unique_lock<std::mutex> lock;
	if (block) { // 外部调用
		lock = std::unique_lock<std::mutex>(m_mutex);
		clear_exec_times();
	}
	decltype(m_tasks_queue) empty;
	m_tasks_queue.swap(empty);
	m_identify_ptr->clear_cache();
}

bool Assistance::set_param(const std::string& type, const std::string& param, const std::string& value)
{
	DebugTraceFunction;
	DebugTrace("SetParam |", type, param, value);

	return Configer::get_instance().set_param(type, param, value);
}

std::optional<std::string> Assistance::get_param(const std::string& type, const std::string& param)
{
	// DebugTraceFunction;
	if (type == "status") {
		if (param == "running") {
			return std::to_string(!m_thread_idle);
		}
		else {
			return std::nullopt;
		}
	}

	return Configer::get_instance().get_param(type, param);
}

bool asst::Assistance::print_window(const std::string& filename)
{
	DebugTraceFunction;

	const cv::Mat& image = get_format_image();
	// 保存的截图额外再裁剪掉一圈，不然企鹅物流识别不出来
	int offset = Configer::get_instance().m_options.print_window_crop_offset;
	cv::Rect rect(offset, offset, image.cols - offset * 2, image.rows - offset * 2);
	bool ret = cv::imwrite(filename.c_str(), image(rect));

	if (ret) {
		DebugTraceInfo("PrintWindow to", filename);
	}
	else {
		DebugTraceError("PrintWindow error", filename);
	}
	return ret;
}

bool asst::Assistance::find_text_and_click(const std::string& text, bool block)
{
	DebugTraceFunction;
	DebugTrace("find_text_and_click |", Utf8ToGbk(text), block ? "block" : "non block");

	std::unique_lock<std::mutex> lock;
	if (block) { // 外部调用
		lock = std::unique_lock<std::mutex>(m_mutex);
	}
	const cv::Mat& image = get_format_image();
	std::optional<Rect>&& result = m_identify_ptr->find_text(image, text);

	if (!result) {
		DebugTrace("Cannot found", Utf8ToGbk(text));
		return false;
	}

	set_control_scale(image.cols, image.rows);
	return m_control_ptr->click(result.value());
}

void Assistance::working_proc(Assistance* pThis)
{
	DebugTraceFunction;

	int retry_times = 0;
	while (!pThis->m_thread_exit) {
		std::unique_lock<std::mutex> lock(pThis->m_mutex);
		if (!pThis->m_thread_idle && !pThis->m_tasks_queue.empty()) {

			auto start_time = std::chrono::system_clock::now();
			std::shared_ptr<AbstractTask> task_ptr = pThis->m_tasks_queue.front();
			task_ptr->set_ptr(pThis->m_window_ptr, pThis->m_view_ptr, pThis->m_control_ptr, pThis->m_identify_ptr);
			task_ptr->set_exit_flag(&pThis->m_thread_idle);
			bool ret = task_ptr->run();
			if (ret) {
				retry_times = 0;
				pThis->m_tasks_queue.pop();
			}
			else // 失败了不pop，一直跑。 Todo: 设一个上限
			{
				++retry_times;
			}

			// 如果下个任务是识别，就按识别的延时来；如果下个任务是点击，就按点击的延时来；……
			// 如果都符合，就取个最大值
			int delay = 0;
			if (!pThis->m_tasks_queue.empty()) {
				int next_type = pThis->m_tasks_queue.front()->get_task_type();
				std::vector<int> candidate_delay = { 0 };
				if (next_type & TaskType::TaskTypeClick) {
					candidate_delay.emplace_back(Configer::get_instance().m_options.task_control_delay);
				}
				if (next_type & TaskType::TaskTypeRecognition) {
					candidate_delay.emplace_back(Configer::get_instance().m_options.task_identify_delay);
				}
				delay = *std::max_element(candidate_delay.cbegin(), candidate_delay.cend());
			}
			pThis->m_condvar.wait_until(lock,
				start_time + std::chrono::milliseconds(delay),
				[&]() -> bool { return pThis->m_thread_idle; });
		}
		else {
			pThis->m_thread_idle = true;
			pThis->m_condvar.wait(lock);
		}
	}
}

void Assistance::task_callback(TaskMsg msg, const json::value& detail, void* custom_arg)
{
	DebugTraceFunction;
	DebugTrace(msg, detail.to_string(), custom_arg);

	Assistance* p_this = (Assistance*)custom_arg;
	switch (msg)
	{
	case TaskMsg::ReadyToSleep:
		break;
	case TaskMsg::EndOfSleep:
		break;
	case TaskMsg::AppendMatchTask:
	{
		std::vector<std::string> next_vec;
		if (detail.exist("tasks")) {
			json::array next_arr = detail.at("tasks").as_array();
			for (const json::value& next_json : next_arr) {
				next_vec.emplace_back(next_json.as_string());
			}
		}
		else if (detail.exist("task")) {
			next_vec.emplace_back(detail.at("task").as_string());
		}
		if (next_vec.size() == 1 && next_vec.at(0) == "Stop") {
			break;
		}
		p_this->append_match_task(next_vec);
	}
	break;
	case TaskMsg::AppendTask:
	{
		std::string task = detail.at("type").as_string();
		if (task == "ClickTask") {
			auto task_ptr = std::make_shared<ClickTask>(task_callback, custom_arg);
			json::array rect_json = detail.at("rect").as_array();
			Rect rect(rect_json[0].as_integer(), rect_json[1].as_integer(), rect_json[2].as_integer(), rect_json[3].as_integer());
			task_ptr->set_rect(std::move(rect));
			p_this->m_tasks_queue.emplace(task_ptr);
		} // else if  // TODO
	}
	break;
	default:
		break;
	}
}

void asst::Assistance::append_match_task(const std::vector<std::string>& tasks)
{
	auto task_ptr = std::make_shared<MatchTask>(task_callback, (void*)this);
	task_ptr->set_tasks(tasks);
	m_tasks_queue.emplace(task_ptr);
}

cv::Mat Assistance::get_format_image()
{
	const cv::Mat& raw_image = m_view_ptr->getImage(m_view_ptr->getWindowRect());
	if (raw_image.empty() || raw_image.rows < 100) {
		DebugTraceError("Window image error");
		return raw_image;
	}
	// 把模拟器边框的一圈裁剪掉
	const EmulatorInfo& window_info = m_view_ptr->getEmulatorInfo();
	int x_offset = window_info.x_offset;
	int y_offset = window_info.y_offset;
	int width = raw_image.cols - x_offset - window_info.right_offset;
	int height = raw_image.rows - y_offset - window_info.bottom_offset;

	cv::Mat cropped(raw_image, cv::Rect(x_offset, y_offset, width, height));

	//// 调整尺寸，与资源中截图的标准尺寸一致
	//cv::Mat dst;
	//cv::resize(cropped, dst, cv::Size(Configer::get_instance().DefaultWindowWidth, Configer::get_instance().DefaultWindowHeight));

	return cropped;
}

void asst::Assistance::set_control_scale(int cur_width, int cur_height)
{
	double scale_width = static_cast<double>(cur_width) / Configer::get_instance().DefaultWindowWidth;
	double scale_height = static_cast<double>(cur_height) / Configer::get_instance().DefaultWindowHeight;
	// 有些模拟器有可收缩的侧边，会增加宽度。
	// config.json中设置的是侧边展开后的offset
	// 如果用户把侧边收起来了，则有侧边的那头会额外裁剪掉一些，长度偏小
	// 所以按这里面长、宽里大的那个算，大的那边没侧边
	double scale = std::max(scale_width, scale_height);
	m_control_ptr->setControlScale(scale);
}

void Assistance::clear_exec_times()
{
	for (auto&& pair : Configer::get_instance().m_all_tasks_info) {
		pair.second.exec_times = 0;
	}
}
