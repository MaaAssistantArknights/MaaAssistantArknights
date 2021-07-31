#include "Task.h"
#include "AsstDef.h"
#include "WinMacro.h"
#include "Identify.h"
#include "Configer.h"
#include "RecruitConfiger.h"
#include "json.h"

#include <chrono>
#include <thread>
#include <utility>

using namespace asst;

AbstractTask::AbstractTask(TaskCallback callback)
	:m_callback(callback)
{
	;
}

void AbstractTask::set_ptr(
	std::shared_ptr<WinMacro> window_ptr,
	std::shared_ptr<WinMacro> view_ptr,
	std::shared_ptr<WinMacro> control_ptr,
	std::shared_ptr<Identify> identify_ptr)
{
	m_window_ptr = window_ptr;
	m_view_ptr = view_ptr;
	m_control_ptr = control_ptr;
	m_identify_ptr = identify_ptr;
}

void asst::AbstractTask::set_exit_flag(bool* exit_flag)
{
	m_exit_flag = exit_flag;
}

cv::Mat AbstractTask::get_format_image()
{
	const cv::Mat& raw_image = m_view_ptr->getImage(m_view_ptr->getWindowRect());
	if (raw_image.empty()) {
		m_callback(TaskMsg::ImageIsEmpty, std::string());
		return raw_image;
	}
	if (raw_image.rows < 100) {
		m_callback(TaskMsg::WindowMinimized, std::string());
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

bool AbstractTask::set_control_scale(int cur_width, int cur_height)
{
	double scale_width = static_cast<double>(cur_width) / Configer::DefaultWindowWidth;
	double scale_height = static_cast<double>(cur_height) / Configer::DefaultWindowHeight;
	double scale = std::max(scale_width, scale_height);
	m_control_ptr->setControlScale(scale);
	return true;
}

void asst::AbstractTask::sleep(unsigned millisecond)
{
	if (millisecond == 0) {
		return;
	}
	auto start = std::chrono::system_clock::now();
	unsigned duration = 0;

	json::value callback_json;
	callback_json["time"] = millisecond;
	m_callback(TaskMsg::ReadyToSleep, callback_json.to_string());

	while ((m_exit_flag != NULL && *m_exit_flag == false)
		|| duration < millisecond) {
		duration = std::chrono::duration_cast<std::chrono::milliseconds>(
			start - std::chrono::system_clock::now()).count();
		std::this_thread::yield();
	}
}

MatchTask::MatchTask(TaskCallback callback,
	std::unordered_map<std::string, TaskInfo>* all_tasks_ptr,
	Configer* configer_ptr)
	: AbstractTask(callback),
	m_all_tasks_ptr(all_tasks_ptr),
	m_configer_ptr(configer_ptr)
{
	;
}

bool MatchTask::run()
{
	if (m_view_ptr == NULL
		|| m_control_ptr == NULL 
		|| m_identify_ptr == NULL
		|| m_all_tasks_ptr == NULL 
		|| m_control_ptr == NULL)
	{
		m_callback(TaskMsg::PtrIsNull, std::string());
		return false;
	}

	TaskInfo task;
	Rect rect;
	bool ret = match_image(&task, &rect);
	if (!ret) {
		return false;
	}

	json::value callback_json = json::object{
		{ "name", task.name },
		{ "type", static_cast<int>(task.type) },
		{ "exec_times", task.exec_times },
		{ "max_times", task.max_times }
	};
	m_callback(TaskMsg::TaskMatched, callback_json.to_string());

	if (task.exec_times < task.max_times)
	{
		m_callback(TaskMsg::ReachedLimit, callback_json.to_string());
		return true;
	}

	// 前置固定延时
	sleep(task.pre_delay);

	switch (task.type) {
	case TaskType::ClickRect:
		rect = task.specific_area;
		[[fallthrough]];
	case TaskType::ClickSelf:
		exec_click_task(task, rect);
		break;
	case TaskType::DoNothing:
		break;
	case TaskType::Stop:
		m_callback(TaskMsg::MissionStop, std::string());
		break;
	default:
		break;
	}

	++task.exec_times;

	// 减少其他任务的执行次数
	// 例如，进入吃理智药的界面了，相当于上一次点蓝色开始行动没生效
	// 所以要给蓝色开始行动的次数减一
	for (const std::string& reduce : task.reduce_other_times) {
		--(*m_all_tasks_ptr)[reduce].exec_times;
	}

	// 后置固定延时
	sleep(task.rear_delay);

	callback_json["exec_times"] = task.exec_times;
	m_callback(TaskMsg::TaskCompleted, callback_json.to_string());

	return true;
}


bool MatchTask::match_image(TaskInfo* task_info, asst::Rect* matched_rect)
{
	const cv::Mat& cur_image = get_format_image();
	if (cur_image.empty() || cur_image.rows < 100) {
		return false;
	} 
	set_control_scale(cur_image.cols, cur_image.rows);

	// 逐个匹配当前可能的图像
	for (const std::string& task_name : m_cur_tasks_name) {
		*task_info = (*m_all_tasks_ptr)[task_name];
		double templ_threshold = task_info->templ_threshold;
		double hist_threshold = task_info->hist_threshold;

		auto&& [algorithm, value, rect] = m_identify_ptr->find_image(cur_image, task_name, templ_threshold);

		if (algorithm == AlgorithmType::JustReturn
			|| (algorithm == AlgorithmType::MatchTemplate && value >= templ_threshold)
			|| (algorithm == AlgorithmType::CompareHist && value >= hist_threshold)) {

			if (matched_rect != NULL) {
				*matched_rect = std::move(rect);
			}
			return true;
		}
	}
	return false;
}

void MatchTask::exec_click_task(TaskInfo& task, const asst::Rect& matched_rect)
{
	// 随机延时功能
	if (m_configer_ptr->m_options.control_delay_upper != 0) {
		static std::default_random_engine rand_engine(std::chrono::system_clock::now().time_since_epoch().count());
		static std::uniform_int_distribution<unsigned> rand_uni(
			m_configer_ptr->m_options.control_delay_lower,
			m_configer_ptr->m_options.control_delay_upper);
		
		unsigned rand_delay = rand_uni(rand_engine);
		sleep(rand_delay);
	}

	m_control_ptr->click(matched_rect);
}