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

	m_configer.load(GetResourceDir() + "config.json");
	m_all_tasks_info = std::move(m_configer.m_all_tasks_info);
	m_recruit_configer.load(GetResourceDir() + "operInfo.json");

	m_identify_ptr = std::make_shared<Identify>();
	for (const auto& [name, info] : m_all_tasks_info)
	{
		m_identify_ptr->add_image(name, GetResourceDir() + "template\\" + info.template_filename);
	}
	m_identify_ptr->set_use_cache(m_configer.m_options.identify_cache);

	m_identify_ptr->set_ocr_param(m_configer.m_options.ocr_gpu_index, m_configer.m_options.ocr_thread_number);
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
		for (const auto& [name, info] : m_configer.m_handles)
		{
			ret = create_handles(info);
			if (ret) {
				cor_name = name;
				break;
			}
		}
	}
	else {	// 指定的模拟器
		ret = create_handles(m_configer.m_handles[emulator_name]);
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

void Assistance::start(const std::string& task)
{
	DebugTraceFunction;
	DebugTrace("Start |", task);
	if (!m_thread_idle || !m_inited) {
		return;
	}

	std::unique_lock<std::mutex> lock(m_mutex);
	clear_exec_times();
	m_identify_ptr->clear_cache();

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

	auto task_ptr = std::make_shared<OpenRecruitTask>(task_callback, (void*)this, &m_configer, &m_recruit_configer);
	task_ptr->set_param(required_level, set_time);
	m_tasks_queue.emplace(task_ptr);

	m_thread_idle = false;
	m_condvar.notify_one();
}

void Assistance::stop(bool block)
{
	DebugTraceFunction;
	DebugTrace("Stop |", block ? "block" : "non block");

	std::unique_lock<std::mutex> lock;
	if (block) { // 外部调用
		lock = std::unique_lock<std::mutex>(m_mutex);
		clear_exec_times();
	}
	m_thread_idle = true;
	std::queue<std::shared_ptr<AbstractTask>> empty;
	m_tasks_queue.swap(empty);
	m_identify_ptr->clear_cache();
}

bool Assistance::set_param(const std::string& type, const std::string& param, const std::string& value)
{
	DebugTraceFunction;
	DebugTrace("SetParam |", type, param, value);

	std::unique_lock<std::mutex> lock(m_mutex);
	return m_configer.set_param(type, param, value);
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

	std::unique_lock<std::mutex> lock(m_mutex);
	return m_configer.get_param(type, param);
}

bool asst::Assistance::print_window(const std::string& filename, bool block)
{
	DebugTraceFunction;
	DebugTrace("print_window |", block ? "block" : "non block");

	std::unique_lock<std::mutex> lock;
	if (block) { // 外部调用
		lock = std::unique_lock<std::mutex>(m_mutex);
	}

	const cv::Mat& image = get_format_image();
	// 保存的截图额外再裁剪掉一圈，不然企鹅物流识别不出来
	int offset = m_configer.m_options.print_window_crop_offset;
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

std::optional<std::vector<std::pair<std::vector<std::string>, OperCombs>>> 
	asst::Assistance::open_recruit(const std::vector<int>& required_level, bool set_time)
{
	DebugTraceFunction;

	std::unique_lock<std::mutex> lock(m_mutex);

	const cv::Mat& image = get_format_image();
	set_control_scale(image.cols, image.rows);
	lock.unlock();	// 后面的计算耗时太长，先解锁

	if (set_time) {
		start("RecruitTime");
	}

	std::vector<TextArea> ider_result = m_identify_ptr->ocr_detect(image);
	std::vector<TextArea> filt_result;
	std::string ider_str;
	for (TextArea& res : ider_result) {
		// 替换一些常见的文字识别错误
		// TODO: 这块时间复杂度有点高，待优化
		for (const auto& [src, cor] : m_configer.m_ocr_replace) {
			res.text = StringReplaceAll(res.text, src, cor);
		}
		ider_str += res.text + " ,";
		for (const std::string& t : m_recruit_configer.m_all_tags) {
			if (res.text == t) {
				filt_result.emplace_back(std::move(res));
			}
		}
	}
	if (ider_str.back() == ',') {
		ider_str.pop_back();
	}
	DebugTrace("All ocr text: ", Utf8ToGbk(ider_str));

	if (filt_result.size() != 5) {
		DebugTraceError("Error, Tags recognition error!!!");
		stop(true);
		return std::nullopt;
	}

	std::vector<std::string> tags;
	for (const TextArea& t_a : filt_result) {
		tags.emplace_back(t_a.text);
	}
	DebugTraceInfo("All Tags", VectorToString(tags, true));

	// Tags全组合
	std::vector<std::vector<std::string>> all_combs;
	int len = tags.size();
	int count = std::pow(2, len);
	for (int i = 0; i < count; ++i) {
		std::vector<std::string> temp;
		for (int j = 0, mask = 1; j < len; ++j) {
			if ((i & mask) != 0) {	// What the fuck???
				temp.emplace_back(tags.at(j));
			}
			mask = mask * 2;
		}
		// 游戏里最多选择3个tag
		if (!temp.empty() && temp.size() <= 3) {
			all_combs.emplace_back(std::move(temp));
		}
	}

	std::map<std::vector<std::string>, OperCombs> result_map;
	for (const std::vector<std::string>& comb : all_combs) {
		for (const OperInfo& cur_oper : m_recruit_configer.m_opers) {
			int matched_count = 0;
			// 组合中每一个tag，是否在干员tags中
			for (const std::string& tag : comb) {
				if (cur_oper.tags.find(tag) != cur_oper.tags.cend()) {
					++matched_count;
				}
				else {
					break;
				}
			}
			if (matched_count == comb.size()) {
				static const std::string SeniorOper = GbkToUtf8("高级资深干员");
				// 高资tag和六星强绑定，如果没有高资tag，即使其他tag匹配上了也不可能出六星
				if (cur_oper.level == 6
					&& std::find(comb.cbegin(), comb.cend(), SeniorOper) == comb.cend()) {
					continue;
				}
				OperCombs& oper_combs = result_map[comb];
				oper_combs.opers.emplace_back(cur_oper);

				static const std::string SupportMachine = GbkToUtf8("支援机械");
				if (cur_oper.level == 1
					&& std::find(comb.cbegin(), comb.cend(), SupportMachine) != comb.cend()) {
					DebugTraceInfo("Has Level 1");
				}
				// 一星小车不计入最低等级
				if (cur_oper.level != 1 && (
					oper_combs.min_level == 0 || oper_combs.min_level > cur_oper.level)) {
					oper_combs.min_level = cur_oper.level;
				}

				if (oper_combs.max_level == 0 || oper_combs.max_level < cur_oper.level) {
					oper_combs.max_level = cur_oper.level;
				}
			}
		}
	}

	// map没法按值排序，转个vector再排
	std::vector<std::pair<std::vector<std::string>, OperCombs>> result_vector;
	for (auto&& pair : result_map) {
		result_vector.emplace_back(std::move(pair));
	}
	std::sort(result_vector.begin(), result_vector.end(), []( const auto& lhs, const auto& rhs)
		->bool {
			// 1、最小等级小的，排最后
			// 2、最小等级相同，最大等级小的，排后面
			// 3、1 2都相同，标签多的，排后面
			if (lhs.second.min_level != rhs.second.min_level) {
				return lhs.second.min_level > rhs.second.min_level;
			}
			else if (lhs.second.max_level != rhs.second.max_level) {
				return lhs.second.max_level > rhs.second.max_level;
			}
			else {
				return lhs.first.size() < rhs.first.size();
			}
		});

	for (const auto& [combs, oper_combs] : result_vector) {
		std::string tag_str;
		for (const std::string& tag : combs) {
			tag_str += tag + " ,";
		}
		if (tag_str.back() == ',') {
			tag_str.pop_back();
		}

		std::string opers_str;
		for (const OperInfo& oper : oper_combs.opers) {
			opers_str += std::to_string(oper.level) + "-" + oper.name + " ,";
		}
		if (opers_str.back() == ',') {
			opers_str.pop_back();
		}
		DebugTraceInfo("Tags:", VectorToString(combs, true), "May be recruited: ", Utf8ToGbk(opers_str));
	}

	stop(true);
	if (!required_level.empty() && !result_vector.empty()) {
		if (std::find(required_level.cbegin(), required_level.cend(), result_vector[0].second.min_level)
			== required_level.cend()) {
			return result_vector;
		}
		const std::vector<std::string>& final_tags = result_vector[0].first;
		std::vector<TextArea> final_text_areas;

		lock.lock();
		for (const TextArea& text_area : filt_result) {
			if (std::find(final_tags.cbegin(), final_tags.cend(), text_area.text) != final_tags.cend()) {
				final_text_areas.emplace_back(text_area);
				m_control_ptr->click(text_area.rect);
				Sleep(300);
			}
		}
		lock.unlock();
	}
	return result_vector;
}

void Assistance::working_proc(Assistance* pThis)
{
	DebugTraceFunction;

	while (!pThis->m_thread_exit) {
		std::unique_lock<std::mutex> lock(pThis->m_mutex);
		if (!pThis->m_thread_idle && !pThis->m_tasks_queue.empty()) {

			auto start_time = std::chrono::system_clock::now();
			std::shared_ptr<AbstractTask> task_ptr = pThis->m_tasks_queue.front();
			task_ptr->set_ptr(pThis->m_window_ptr, pThis->m_view_ptr, pThis->m_control_ptr, pThis->m_identify_ptr);
			task_ptr->set_exit_flag(&pThis->m_thread_idle);
			bool ret = task_ptr->run();
			if (ret) {
				pThis->m_tasks_queue.pop();
			} // else {} 失败了不pop，一直跑。 Todo: 设一个上限

			pThis->m_condvar.wait_until(lock,
				start_time + std::chrono::milliseconds(pThis->m_configer.m_options.identify_delay),
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
	case TaskMsg::PtrIsNull:
		break;
	case TaskMsg::ImageIsEmpty:
		break;
	case TaskMsg::WindowMinimized:
		break;
	case TaskMsg::TaskMatched:
		break;
	case TaskMsg::ReachedLimit:
		break;
	case TaskMsg::ReadyToSleep:
		break;
	case TaskMsg::TaskCompleted:
		break;
	case TaskMsg::MissionStop:
		break;
	case TaskMsg::AppendTask:
	{
		json::array next_arr = detail.at("next").as_array();
		std::vector<std::string> next_vec;
		for (const json::value& next_json : next_arr) {
			next_vec.emplace_back(next_json.as_string());
		}
		p_this->append_match_task(next_vec);
	}
		break;
	default:
		break;
	}
}

void asst::Assistance::append_match_task(const std::vector<std::string>& tasks)
{
	auto task_ptr = std::make_shared<MatchTask>(task_callback, (void*)this, &m_all_tasks_info, &m_configer);
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
	//cv::resize(cropped, dst, cv::Size(m_configer.DefaultWindowWidth, m_configer.DefaultWindowHeight));

	return cropped;
}

void asst::Assistance::set_control_scale(int cur_width, int cur_height)
{
	double scale_width = static_cast<double>(cur_width) / m_configer.DefaultWindowWidth;
	double scale_height = static_cast<double>(cur_height) / m_configer.DefaultWindowHeight;
	// 有些模拟器有可收缩的侧边，会增加宽度。
	// config.json中设置的是侧边展开后的offset
	// 如果用户把侧边收起来了，则有侧边的那头会额外裁剪掉一些，长度偏小
	// 所以按这里面长、宽里大的那个算，大的那边没侧边
	double scale = std::max(scale_width, scale_height);
	m_control_ptr->setControlScale(scale);
}

void Assistance::clear_exec_times()
{
	for (auto&& pair : m_all_tasks_info) {
		pair.second.exec_times = 0;
	}
}
