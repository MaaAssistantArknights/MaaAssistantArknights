#include "Task.h"
#include "AsstDef.h"
#include "WinMacro.h"
#include "Identify.h"
#include "Configer.h"
#include "RecruitConfiger.h"
#include "json.h"
#include "AsstAux.h"

#include <chrono>
#include <thread>
#include <utility>

using namespace asst;

AbstractTask::AbstractTask(TaskCallback callback, void* callback_arg)
	:m_callback(callback),
	m_callback_arg(callback_arg)
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
		m_callback(TaskMsg::ImageIsEmpty, json::value(), m_callback_arg);
		return raw_image;
	}
	if (raw_image.rows < 100) {
		m_callback(TaskMsg::WindowMinimized, json::value(), m_callback_arg);
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
	m_callback(TaskMsg::ReadyToSleep, callback_json, m_callback_arg);

	while ((m_exit_flag == NULL || *m_exit_flag == false)
		&& duration < millisecond) {
		duration = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now() - start).count();
		std::this_thread::yield();
	}
}

MatchTask::MatchTask(TaskCallback callback, void* callback_arg,
	std::unordered_map<std::string, TaskInfo>* all_tasks_ptr,
	Configer* configer_ptr)
	: AbstractTask(callback, callback_arg),
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
		m_callback(TaskMsg::PtrIsNull, json::value(), m_callback_arg);
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
	m_callback(TaskMsg::TaskMatched, callback_json, m_callback_arg);

	if (task.exec_times >= task.max_times)
	{
		m_callback(TaskMsg::ReachedLimit, callback_json, m_callback_arg);

		json::value next_json = callback_json;
		next_json["next"] = json::array(task.exceeded_next);
		m_callback(TaskMsg::AppendTask, next_json, m_callback_arg);
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
		m_callback(TaskMsg::MissionStop, json::value(), m_callback_arg);
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
	m_callback(TaskMsg::TaskCompleted, callback_json, m_callback_arg);

	json::value next_json = callback_json;
	next_json["next"] = json::array(task.next);
	m_callback(TaskMsg::AppendTask, next_json, m_callback_arg);

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


OcrAbstractTask::OcrAbstractTask(TaskCallback callback, void* callback_arg)
	: AbstractTask(callback, callback_arg)
{
	;
}

std::vector<TextArea> OcrAbstractTask::ocr_detect()
{
	const cv::Mat& image = get_format_image();
	return m_identify_ptr->ocr_detect(image);
}

OpenRecruitTask::OpenRecruitTask(TaskCallback callback, void* callback_arg,
	Configer* configer_ptr, RecruitConfiger* recruit_configer_ptr)
	: OcrAbstractTask(callback, callback_arg),
	m_configer_ptr(configer_ptr),
	m_recruit_configer_ptr(recruit_configer_ptr)
{
	;
}

bool OpenRecruitTask::run()
{
	if (m_view_ptr == NULL
		|| m_control_ptr == NULL
		|| m_identify_ptr == NULL
		|| m_control_ptr == NULL
		|| m_recruit_configer_ptr == NULL)
	{
		m_callback(TaskMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}

	/* Find all text */
	std::vector<TextArea> all_text_area = ocr_detect();

	std::vector<json::value> all_text_json_vector;
	for (const TextArea& text_area : all_text_area) {
		all_text_json_vector.emplace_back(Utf8ToGbk(text_area.text));
	}
	json::value all_text_json;
	all_text_json["text"] = json::array(all_text_json_vector);
	m_callback(TaskMsg::DetectText, all_text_json, m_callback_arg);


	/* Filter out all tags from all text */
	std::vector<TextArea> all_tags = text_filter(all_text_area, m_recruit_configer_ptr->m_all_tags, m_configer_ptr->m_ocr_replace);

	std::vector<json::value> all_tags_json_vector;
	for (const TextArea& text_area : all_tags) {
		all_tags_json_vector.emplace_back(Utf8ToGbk(text_area.text));
	}
	json::value all_tags_json;
	all_tags_json["tags"] = json::array(all_tags_json_vector);
	m_callback(TaskMsg::DetectText, all_tags_json, m_callback_arg);

	return true;
}

void OpenRecruitTask::set_param(std::vector<int> required_level, bool set_time)
{
	m_required_level = std::move(required_level);
	m_set_time = set_time;
}

