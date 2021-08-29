#include "Assistance.h"

#include <time.h>
#include <filesystem>

#include <json.h>
#include <opencv2/opencv.hpp>

#include "WinMacro.h"
#include "Configer.h"
#include "Identify.h"
#include "Logger.hpp"
#include "AsstAux.h"
#include "Task.h"
#include "RecruitConfiger.h"
#include "InfrastConfiger.h"

using namespace asst;

Assistance::Assistance(AsstCallback callback, void* callback_arg)
	: m_callback(callback), m_callback_arg(callback_arg)
{
	DebugTraceFunction;

	// 检查返回值，若为false则回调错误
	auto callback_error = [&](const std::string& filename = std::string()) {
		json::value callback_json;
		callback_json["filename"] = filename;
		callback_json["what"] = "Resource";
		m_callback(AsstMsg::InitFaild, callback_json, m_callback_arg);
	};

	bool ret = Configer::get_instance().load(GetResourceDir() + "config.json");
	if (!ret) {
		callback_error();
		return;
	}
	ret = RecruitConfiger::get_instance().load(GetResourceDir() + "recruit.json");
	if (!ret) {
		callback_error();
		return;
	}
	ret = InfrastConfiger::get_instance().load(GetResourceDir() + "infrast.json");
	if (!ret) {
		callback_error();
		return;
	}

	m_identify_ptr = std::make_shared<Identify>();
	for (const auto& [name, info] : Configer::get_instance().m_all_tasks_info)
	{
		if (info->algorithm != AlgorithmType::MatchTemplate)
		{
			continue;
		}
		std::string filename = std::dynamic_pointer_cast<MatchTaskInfo>(info)->template_filename;
		ret = m_identify_ptr->add_image(name, GetResourceDir() + "template\\" + filename);
		if (!ret) {
			callback_error();
			return;
		}
	}
	m_identify_ptr->set_use_cache(Configer::get_instance().m_options.identify_cache);

	m_identify_ptr->set_ocr_param(Configer::get_instance().m_options.ocr_gpu_index, Configer::get_instance().m_options.ocr_thread_number);
	ret = m_identify_ptr->ocr_init_models(GetResourceDir() + "OcrLiteOnnx\\models\\");
	if (!ret) {
		callback_error();
		return;
	}

	for (const auto& [key, name] : InfrastConfiger::get_instance().m_oper_name_feat) {
		ret = m_identify_ptr->add_text_image(name, GetResourceDir() + "operators\\" + Utf8ToGbk(name) + ".png");
		if (!ret) {
			callback_error(Utf8ToGbk(name));
			return;
		}
	}
	for (const auto& name : InfrastConfiger::get_instance().m_oper_name_feat_whatever) {
		ret = m_identify_ptr->add_text_image(name, GetResourceDir() + "operators\\" + Utf8ToGbk(name) + ".png");
		if (!ret) {
			callback_error(Utf8ToGbk(name));
			return;
		}
	}

	// 精一和精二的图片，调试用
	ret = m_identify_ptr->add_text_image("Elite1", GetResourceDir() + "operators\\Elite1.png");
	ret &= m_identify_ptr->add_text_image("Elite2", GetResourceDir() + "operators\\Elite2.png");
	if (!ret) {
		callback_error();
		return;
	}

	m_working_thread = std::thread(working_proc, this);
	m_msg_thread = std::thread(msg_proc, this);
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
	m_msg_condvar.notify_all();

	if (m_working_thread.joinable())
	{
		m_working_thread.join();
	}
	if (m_msg_thread.joinable())
	{
		m_msg_thread.join();
	}
}

std::optional<std::string> Assistance::catch_emulator(const std::string& emulator_name)
{
	DebugTraceFunction;

	stop();

	auto create_handles = [&](const EmulatorInfo& info) -> bool
	{
		m_window_ptr = std::make_shared<WinMacro>(info, HandleType::Window);
		m_view_ptr = std::make_shared<WinMacro>(info, HandleType::View);
		m_control_ptr = std::make_shared<WinMacro>(info, HandleType::Control);
		return m_window_ptr->captured() && m_view_ptr->captured() && m_control_ptr->captured();
	};

	bool ret = false;
	std::string cor_name = emulator_name;

	std::unique_lock<std::mutex> lock(m_mutex);

	// 自动匹配模拟器，逐个找
	if (emulator_name.empty())
	{
		for (const auto& [name, info] : Configer::get_instance().m_handles)
		{
			ret = create_handles(info);
			if (ret)
			{
				cor_name = name;
				break;
			}
		}
	}
	else
	{ // 指定的模拟器
		ret = create_handles(Configer::get_instance().m_handles[emulator_name]);
	}
	if (ret)
	{
		m_inited = true;
		return cor_name;
	}
	else
	{
		m_inited = false;
		return std::nullopt;
	}
}

bool asst::Assistance::start_sanity()
{
	return start_match_task("SanityBegin", ProcessTaskRetryTimesDefault);
}

bool asst::Assistance::start_visit()
{
	return start_match_task("VisitBegin", ProcessTaskRetryTimesDefault);
}

bool Assistance::start_match_task(const std::string& task, int retry_times, bool block)
{
	DebugTraceFunction;
	DebugTrace("Start |", task, block ? "block" : "non block");
	if (!m_thread_idle || !m_inited)
	{
		return false;
	}

	std::unique_lock<std::mutex> lock;
	if (block)
	{
		lock = std::unique_lock<std::mutex>(m_mutex);
		//clear_exec_times();
		m_identify_ptr->clear_cache();
	}

	append_match_task(task, { task }, retry_times);

	m_thread_idle = false;
	m_condvar.notify_one();

	return true;
}

bool Assistance::start_ocr_test_task(std::vector<std::string> text_vec, bool need_click)
{
	DebugTraceFunction;
	if (!m_thread_idle || !m_inited)
	{
		return false;
	}

	std::unique_lock<std::mutex> lock(m_mutex);

	auto task_ptr = std::make_shared<TestOcrTask>(task_callback, (void*)this);
	task_ptr->set_text(std::move(text_vec), need_click);
	m_tasks_queue.emplace(task_ptr);

	m_thread_idle = false;
	m_condvar.notify_one();

	return true;
}

bool Assistance::start_open_recruit(const std::vector<int>& required_level, bool set_time)
{
	DebugTraceFunction;
	if (!m_thread_idle || !m_inited)
	{
		return false;
	}

	std::unique_lock<std::mutex> lock(m_mutex);

	auto task_ptr = std::make_shared<OpenRecruitTask>(task_callback, (void*)this);
	task_ptr->set_param(required_level, set_time);
	task_ptr->set_retry_times(OpenRecruitTaskRetyrTimesDefault);
	task_ptr->set_task_chain("OpenRecruit");
	m_tasks_queue.emplace(task_ptr);

	m_thread_idle = false;
	m_condvar.notify_one();

	return true;
}

bool asst::Assistance::start_to_identify_opers()
{
	DebugTraceFunction;
	if (!m_thread_idle || !m_inited)
	{
		return false;
	}

	std::unique_lock<std::mutex> lock(m_mutex);

	auto task_ptr = std::make_shared<IdentifyOperTask>(task_callback, (void*)this);
	task_ptr->set_task_chain("IdentifyOpers");
	m_tasks_queue.emplace(task_ptr);

	m_thread_idle = false;
	m_condvar.notify_one();

	return true;
}

bool asst::Assistance::start_infrast()
{
	DebugTraceFunction;
	if (!m_thread_idle || !m_inited)
	{
		return false;
	}

	std::unique_lock<std::mutex> lock(m_mutex);
	auto ret = get_opers_idtf_result();
	if (!ret) {
		DebugTraceInfo("Get opers info error");
		return false;
	}
	/* 基建任务整体流程：
	1. 从任意界面进入基建，使用ProcessTask
	2. 一键收获贸易站、制造站、干员信赖，使用ProcessTask
		1) 如果收获了，使用基建全缩放到最小的模板匹配
		2) 如果没收获，使用基建默认大小的模板匹配
	3. 进入宿舍，把心情低于阈值的、心情没满但不在工作的，都换下去，TODO
	4. 根据用户设置，按顺序进入指定基建，使用ProcessTask
	5. 按顺序对不同的基建设施进行换班，使用InfrastStationTask
	6. 会客室线索处理，TODO
	*/
	constexpr static const char* InfrastTaskCahin = "Infrast";

	// 从进入制造站，到进入干员选择界面清空选择
	append_match_task(InfrastTaskCahin, { "Manufacturing" });

	// 识别并选择最优解干员组合
	//auto task_ptr = std::make_shared<InfrastStationTask>(task_callback, (void*)this);
	//task_ptr->set_facility("Manufacturing");
	//task_ptr->set_task_chain(InfrastTaskCahin);
	//task_ptr->set_all_opers_info(std::move(ret.value()));
	//m_tasks_queue.emplace(task_ptr);

	m_thread_idle = false;
	m_condvar.notify_one();

	return true;
}

void Assistance::stop(bool block)
{
	DebugTraceFunction;
	DebugTrace("Stop |", block ? "block" : "non block");

	m_thread_idle = true;

	std::unique_lock<std::mutex> lock;
	if (block)
	{ // 外部调用
		lock = std::unique_lock<std::mutex>(m_mutex);
	}
	decltype(m_tasks_queue) empty;
	m_tasks_queue.swap(empty);
	clear_exec_times();
	m_identify_ptr->clear_cache();
}

bool Assistance::set_param(const std::string& type, const std::string& param, const std::string& value)
{
	DebugTraceFunction;
	DebugTrace("SetParam |", type, param, value);

	return Configer::get_instance().set_param(type, param, value);
}

bool asst::Assistance::swipe(const Point& p1, const Point& p2)
{
	return m_control_ptr->swipe(p1, p2);
}

void Assistance::working_proc(Assistance* p_this)
{
	DebugTraceFunction;

	int retry_times = 0;
	while (!p_this->m_thread_exit)
	{
		//DebugTraceScope("Assistance::working_proc Loop");
		std::unique_lock<std::mutex> lock(p_this->m_mutex);
		if (!p_this->m_thread_idle && !p_this->m_tasks_queue.empty())
		{
			auto start_time = std::chrono::system_clock::now();
			std::shared_ptr<AbstractTask> task_ptr = p_this->m_tasks_queue.front();
			task_ptr->set_ptr(p_this->m_window_ptr, p_this->m_view_ptr, p_this->m_control_ptr, p_this->m_identify_ptr);
			task_ptr->set_exit_flag(&p_this->m_thread_idle);
			bool ret = task_ptr->run();
			if (ret)
			{
				retry_times = 0;
				// 任务执行成功了直接pop
				p_this->m_tasks_queue.pop();
			}
			else
			{
				// 失败了累加失败次数，超限了再pop
				if (retry_times >= task_ptr->get_retry_times())
				{
					json::value task_error_json;
					task_error_json["retry_limit"] = task_ptr->get_retry_times();
					task_error_json["retry_times"] = retry_times;
					task_error_json["task_chain"] = task_ptr->get_task_chain();
					p_this->task_callback(AsstMsg::TaskError, std::move(task_error_json), p_this);

					retry_times = 0;
					p_this->m_tasks_queue.pop();
				}
				else
				{
					++retry_times;
				}
			}

			// 如果下个任务是识别，就按识别的延时来；如果下个任务是点击，就按点击的延时来；……
			// 如果都符合，就取个最大值
			int delay = 0;
			if (!p_this->m_tasks_queue.empty())
			{
				int next_type = p_this->m_tasks_queue.front()->get_task_type();
				std::vector<int> candidate_delay = { 0 };
				if (next_type & TaskType::TaskTypeClick)
				{
					candidate_delay.emplace_back(Configer::get_instance().m_options.task_control_delay);
				}
				if (next_type & TaskType::TaskTypeRecognition)
				{
					candidate_delay.emplace_back(Configer::get_instance().m_options.task_identify_delay);
				}
				delay = *std::max_element(candidate_delay.cbegin(), candidate_delay.cend());
			}
			p_this->m_condvar.wait_until(lock,
				start_time + std::chrono::milliseconds(delay),
				[&]() -> bool
				{ return p_this->m_thread_idle; });
		}
		else
		{
			p_this->m_thread_idle = true;
			p_this->m_condvar.wait(lock);
		}
	}
}

void Assistance::msg_proc(Assistance* p_this)
{
	DebugTraceFunction;

	while (!p_this->m_thread_exit)
	{
		//DebugTraceScope("Assistance::msg_proc Loop");
		std::unique_lock<std::mutex> lock(p_this->m_msg_mutex);
		if (!p_this->m_msg_queue.empty())
		{
			// 结构化绑定只能是引用，后续的pop会使引用失效，所以需要重新构造一份，这里采用了move的方式
			auto&& [temp_msg, temp_detail] = p_this->m_msg_queue.front();
			AsstMsg msg = std::move(temp_msg);
			json::value detail = std::move(temp_detail);
			p_this->m_msg_queue.pop();
			lock.unlock();

			if (p_this->m_callback)
			{
				p_this->m_callback(msg, detail, p_this->m_callback_arg);
			}
		}
		else
		{
			p_this->m_msg_condvar.wait(lock);
		}
	}
}

void Assistance::task_callback(AsstMsg msg, const json::value& detail, void* custom_arg)
{
	DebugTrace("Assistance::task_callback |", msg, detail.to_string());

	Assistance* p_this = (Assistance*)custom_arg;
	json::value more_detail = detail;
	switch (msg)
	{
	case AsstMsg::PtrIsNull:
	case AsstMsg::ImageIsEmpty:
		p_this->stop(false);
		break;
	case AsstMsg::WindowMinimized:
		p_this->m_window_ptr->showWindow();
		break;
	case AsstMsg::AppendProcessTask:
		more_detail["type"] = "ProcessTask";
		[[fallthrough]];
	case AsstMsg::AppendTask:
		p_this->append_task(more_detail);
		return;	// 这俩消息Assistance会新增任务，外部不需要处理，直接拦掉
		break;
	case AsstMsg::OpersIdtfResult:
		set_opers_idtf_result(more_detail);	// 保存到文件
		break;
	default:
		break;
	}

	// Todo: 有些不需要回调给外部的消息，得在这里给拦截掉
	// 加入回调消息队列，由回调消息线程外抛给外部
	p_this->append_callback(msg, std::move(more_detail));
}

void asst::Assistance::append_match_task(const std::string& task_chain, const std::vector<std::string>& tasks, int retry_times)
{
	auto task_ptr = std::make_shared<ProcessTask>(task_callback, (void*)this);
	task_ptr->set_task_chain(task_chain);
	task_ptr->set_tasks(tasks);
	task_ptr->set_retry_times(retry_times);
	m_tasks_queue.emplace(task_ptr);
}

void asst::Assistance::append_task(const json::value& detail)
{
	std::string task_type = detail.at("type").as_string();
	std::string task_chain = detail.get("task_chain", "");
	int retry_times = detail.get("retry_times", INT_MAX);

	if (task_type == "ClickTask")
	{
		auto task_ptr = std::make_shared<ClickTask>(task_callback, (void*)this);
		task_ptr->set_task_chain(task_chain);
		task_ptr->set_retry_times(retry_times);

		json::array rect_json = detail.at("rect").as_array();
		Rect rect(rect_json[0].as_integer(), rect_json[1].as_integer(), rect_json[2].as_integer(), rect_json[3].as_integer());
		task_ptr->set_rect(std::move(rect));

		m_tasks_queue.emplace(task_ptr);
	}
	else if (task_type == "ProcessTask")
	{
		std::vector<std::string> next_vec;
		if (detail.exist("tasks"))
		{
			json::array next_arr = detail.at("tasks").as_array();
			for (const json::value& next_json : next_arr)
			{
				next_vec.emplace_back(next_json.as_string());
			}
		}
		else if (detail.exist("task"))
		{
			next_vec.emplace_back(detail.at("task").as_string());
		}
		append_match_task(task_chain, next_vec, retry_times);
	}
	// else if  // TODO
}

void asst::Assistance::append_callback(AsstMsg msg, json::value detail)
{
	if (m_callback)
	{
		std::unique_lock<std::mutex> lock(m_msg_mutex);
		m_msg_queue.emplace(msg, std::move(detail));
		m_msg_condvar.notify_one();
	}
}

void Assistance::clear_exec_times()
{
	for (auto&& pair : Configer::get_instance().m_all_tasks_info)
	{
		pair.second->exec_times = 0;
	}
}

void asst::Assistance::set_opers_idtf_result(const json::value& detail)
{
	constexpr static const char* Filename = "operators.json";
	std::ofstream ofs(GetCurrentDir() + Filename, std::ios::out);
	ofs << detail.format() << std::endl;
	ofs.close();
}

std::optional<std::unordered_map<std::string, OperInfrastInfo>> Assistance::get_opers_idtf_result()
{
	constexpr static const char* Filename = "operators.json";
	std::ifstream ifs(GetCurrentDir() + Filename, std::ios::in);
	if (!ifs.is_open()) {
		return std::nullopt;
	}
	std::stringstream iss;
	iss << ifs.rdbuf();
	ifs.close();
	std::string content(iss.str());

	auto&& parse_ret = json::parse(content);
	if (!parse_ret) {
		DebugTraceError(__FUNCTION__, "json parsing error!");
		return std::nullopt;
	}
	json::value root = std::move(parse_ret.value());
	std::unordered_map<std::string, OperInfrastInfo> opers_info;
	try {
		for (const json::value& info_json : root["all"].as_array())
		{
			OperInfrastInfo info;
			info.name = info_json.at("name").as_string();
			info.elite = info_json.at("elite").as_integer();
			info.level = info_json.get("level", 0);
			opers_info.emplace(info.name, std::move(info));
		}
	}
	catch (json::exception& exp) {
		DebugTraceError(__FUNCTION__, "json parsing error!");
		return std::nullopt;
	}
	return opers_info;
}
