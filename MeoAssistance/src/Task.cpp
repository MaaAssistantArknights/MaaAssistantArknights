#include "Task.h"
#include "AsstDef.h"
#include "WinMacro.h"
#include "Identify.h"
#include "Configer.h"
#include "OpenRecruitConfiger.h"
#include <json.h>
#include "AsstAux.h"

#include <chrono>
#include <thread>
#include <utility>
#include <cmath>
#include <mutex>
#include <filesystem>

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

void AbstractTask::set_exit_flag(bool* exit_flag)
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

void AbstractTask::sleep(unsigned millisecond)
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
	m_callback(TaskMsg::EndOfSleep, callback_json, m_callback_arg);
}

bool AbstractTask::print_window(const std::string& dir)
{
	const cv::Mat& image = get_format_image();
	if (image.empty()) {
		return false;
	}
	// 保存的截图额外再裁剪掉一圈，不然企鹅物流识别不出来
	int offset = Configer::get_instance().m_options.print_window_crop_offset;
	cv::Rect rect(offset, offset, image.cols - offset * 2, image.rows - offset * 2);

	std::filesystem::create_directory(dir);
	const std::string time_str = StringReplaceAll(StringReplaceAll(GetFormatTimeString(), " ", "_"), ":", "-");
	const std::string filename = dir + time_str + ".png";

	bool ret = cv::imwrite(filename.c_str(), image(rect));

	json::value callback_json;
	callback_json["filename"] = filename;
	callback_json["ret"] = ret;
	callback_json["offset"] = offset;
	m_callback(TaskMsg::PrintWindow, callback_json, m_callback_arg);

	return ret;
}

MatchTask::MatchTask(TaskCallback callback, void* callback_arg)
	: AbstractTask(callback, callback_arg)
{
	m_task_type = TaskType::TaskTypeRecognition | TaskType::TaskTypeClick;
}

bool MatchTask::run()
{
	if (m_view_ptr == NULL
		|| m_control_ptr == NULL
		|| m_identify_ptr == NULL
		|| m_control_ptr == NULL)
	{
		m_callback(TaskMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}

	m_callback(TaskMsg::TaskStart, json::value(), m_callback_arg);

	Rect rect;
	auto&& ret = match_image(&rect);
	if (!ret) {
		return false;
	}
	TaskInfo& task = Configer::get_instance().m_all_tasks_info[ret.value()];

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
		next_json["tasks"] = json::array(task.exceeded_next);
		m_callback(TaskMsg::AppendMatchTask, next_json, m_callback_arg);
		return true;
	}

	// 前置固定延时
	sleep(task.pre_delay);

	bool need_stop = false;
	switch (task.type) {
	case MatchTaskType::ClickRect:
		rect = task.specific_area;
		[[fallthrough]];
	case MatchTaskType::ClickSelf:
		exec_click_task(task, rect);
		break;
	case MatchTaskType::DoNothing:
		break;
	case MatchTaskType::Stop:
		m_callback(TaskMsg::TaskStop, json::value(), m_callback_arg);
		need_stop = true;
		break;
	case MatchTaskType::PrintWindow:
	{
		sleep(Configer::get_instance().m_options.print_window_delay);
		static const std::string dirname = GetCurrentDir() + "screenshot\\";
		print_window(dirname);
	}
	break;
	default:
		break;
	}

	++task.exec_times;

	// 减少其他任务的执行次数
	// 例如，进入吃理智药的界面了，相当于上一次点蓝色开始行动没生效
	// 所以要给蓝色开始行动的次数减一
	for (const std::string& reduce : task.reduce_other_times) {
		--Configer::get_instance().m_all_tasks_info[reduce].exec_times;
	}

	if (need_stop) {
		return true;
	}

	callback_json["exec_times"] = task.exec_times;
	m_callback(TaskMsg::TaskCompleted, callback_json, m_callback_arg);

	// 后置固定延时
	sleep(task.rear_delay);

	json::value next_json = callback_json;
	next_json["tasks"] = json::array(task.next);
	m_callback(TaskMsg::AppendMatchTask, next_json, m_callback_arg);

	return true;
}


std::optional<std::string> MatchTask::match_image(Rect* matched_rect)
{
	const cv::Mat& cur_image = get_format_image();
	if (cur_image.empty() || cur_image.rows < 100) {
		return std::nullopt;
	}
	set_control_scale(cur_image.cols, cur_image.rows);

	// 逐个匹配当前可能的图像
	for (const std::string& task_name : m_cur_tasks_name) {
		TaskInfo& task_info = Configer::get_instance().m_all_tasks_info[task_name];
		double templ_threshold = task_info.templ_threshold;
		double hist_threshold = task_info.hist_threshold;

		auto&& [algorithm, value, rect] = m_identify_ptr->find_image(cur_image, task_name, templ_threshold);

		json::value callback_json;
		if (algorithm == AlgorithmType::JustReturn) {
			callback_json["threshold"] = 0.0;
			callback_json["algorithm"] = "JustReturn";
		}
		else if (algorithm == AlgorithmType::MatchTemplate && value >= templ_threshold) {
			callback_json["threshold"] = templ_threshold;
			callback_json["algorithm"] = "MatchTemplate";
		}
		else if (algorithm == AlgorithmType::CompareHist && value >= hist_threshold) {
			callback_json["threshold"] = hist_threshold;
			callback_json["algorithm"] = "CompareHist";
		}
		else {
			continue;
		}

		if (matched_rect != NULL) {
			callback_json["rect"] = json::array({rect.x, rect.y, rect.width, rect.height});
			*matched_rect = std::move(rect);
		}
		callback_json["algorithm_id"] = static_cast<std::underlying_type<MatchTaskType>::type>(algorithm);
		callback_json["value"] = value;
		m_callback(TaskMsg::ImageMatched, callback_json, m_callback_arg);

		return task_name;
	}
	return std::nullopt;
}

void MatchTask::exec_click_task(TaskInfo& task, const Rect& matched_rect)
{
	// 随机延时功能
	if (Configer::get_instance().m_options.control_delay_upper != 0) {
		static std::default_random_engine rand_engine(
			std::chrono::system_clock::now().time_since_epoch().count());
		static std::uniform_int_distribution<unsigned> rand_uni(
			Configer::get_instance().m_options.control_delay_lower,
			Configer::get_instance().m_options.control_delay_upper);

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

	auto&& dst = m_identify_ptr->ocr_detect(image);

	return dst;
}

OpenRecruitTask::OpenRecruitTask(TaskCallback callback, void* callback_arg)
	: OcrAbstractTask(callback, callback_arg)
{
	m_task_type = TaskType::TaskTypeRecognition;
}

bool OpenRecruitTask::run()
{
	if (m_view_ptr == NULL
		|| m_identify_ptr == NULL)
	{
		m_callback(TaskMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}

	m_callback(TaskMsg::TaskStart, json::value(), m_callback_arg);

	/* Find all text */
	std::vector<TextArea> all_text_area = ocr_detect();

	std::vector<json::value> all_text_json_vector;
	for (const TextArea& text_area : all_text_area) {
		all_text_json_vector.emplace_back(Utf8ToGbk(text_area.text));
	}
	json::value all_text_json;
	all_text_json["text"] = json::array(all_text_json_vector);
	m_callback(TaskMsg::TextDetected, all_text_json, m_callback_arg);

	/* Filter out all tags from all text */
	std::vector<TextArea> all_tags = text_filter(
		all_text_area, OpenRecruitConfiger::get_instance().m_all_tags, Configer::get_instance().m_ocr_replace);

	std::unordered_set<std::string> all_tags_name;
	std::vector<json::value> all_tags_json_vector;
	for (const TextArea& text_area : all_tags) {
		all_tags_name.emplace(text_area.text);
		all_tags_json_vector.emplace_back(Utf8ToGbk(text_area.text));
	}
	json::value all_tags_json;
	all_tags_json["tags"] = json::array(all_tags_json_vector);
	m_callback(TaskMsg::RecruitTagsDetected, all_tags_json, m_callback_arg);

	/* 过滤tags数量不足的情况（可能是识别漏了） */
	if (all_tags.size() != OpenRecruitConfiger::CorrectNumberOfTags) {
		all_tags_json["type"] = "OpenRecruit";
		m_callback(TaskMsg::OcrResultError, all_tags_json, m_callback_arg);
		return false;
	}

	/* 设置招募时间9小时，加入任务队列*/
	if (m_set_time) {
		json::value settime_json;
		settime_json["task"] = "RecruitTime";
		m_callback(TaskMsg::AppendMatchTask, settime_json, m_callback_arg);
	}

	/* 针对一星干员的额外回调消息 */
	static const std::string SupportMachine_GBK = "支援机械";
	static const std::string SupportMachine = GbkToUtf8(SupportMachine_GBK);
	if (std::find(all_tags_name.cbegin(), all_tags_name.cend(), SupportMachine) != all_tags_name.cend()) {
		json::value special_tag_json;
		special_tag_json["tag"] = SupportMachine_GBK;
		m_callback(TaskMsg::RecruitSpecialTag, special_tag_json, m_callback_arg);
	}

	// 识别到的5个Tags，全组合排列
	std::vector<std::vector<std::string>> all_combs;
	int len = all_tags.size();
	int count = std::pow(2, len);
	for (int i = 0; i < count; ++i) {
		std::vector<std::string> temp;
		for (int j = 0, mask = 1; j < len; ++j) {
			if ((i & mask) != 0) {	// What the fuck???
				temp.emplace_back(all_tags.at(j).text);
			}
			mask = mask * 2;
		}
		// 游戏里最多选择3个tag
		if (!temp.empty() && temp.size() <= 3) {
			all_combs.emplace_back(std::move(temp));
		}
	}

	// key: tags comb, value: 干员组合
	// 例如 key: { "狙击"、"群攻" }，value: OperCombs.opers{ "陨星", "白雪", "空爆" }
	std::map<std::vector<std::string>, OperCombs> result_map;
	for (const std::vector<std::string>& comb : all_combs) {
		for (const OperInfo& cur_oper : OpenRecruitConfiger::get_instance().m_all_opers) {
			int matched_count = 0;
			for (const std::string& tag : comb) {
				if (cur_oper.tags.find(tag) != cur_oper.tags.cend()) {
					++matched_count;
				}
				else {
					break;
				}
			}

			// 单个tags comb中的每一个tag，这个干员都有，才算该干员符合这个tags comb
			if (matched_count != comb.size()) {
				continue;
			}

			if (cur_oper.level == 6) {
				// 高资tag和六星强绑定，如果没有高资tag，即使其他tag匹配上了也不可能出六星
				static const std::string SeniorOper = GbkToUtf8("高级资深干员");
				if (std::find(comb.cbegin(), comb.cend(), SeniorOper) == comb.cend()) {
					continue;
				}
			}

			OperCombs& oper_combs = result_map[comb];
			oper_combs.opers.emplace_back(cur_oper);

			if (cur_oper.level == 1 || cur_oper.level == 2) {
				if (oper_combs.min_level == 0) oper_combs.min_level = cur_oper.level;
				if (oper_combs.max_level == 0) oper_combs.max_level = cur_oper.level;
				// 一星、二星干员不计入最低等级，因为拉满9小时之后不可能出1、2星
				continue;
			}
			if (oper_combs.min_level == 0 || oper_combs.min_level > cur_oper.level) {
				oper_combs.min_level = cur_oper.level;
			}
			if (oper_combs.max_level == 0 || oper_combs.max_level < cur_oper.level) {
				oper_combs.max_level = cur_oper.level;
			}
			oper_combs.avg_level += cur_oper.level;
		}
		if (result_map.find(comb) != result_map.cend()) {
			OperCombs& oper_combs = result_map[comb];
			oper_combs.avg_level /= oper_combs.opers.size();
		}
	}

	// map没法按值排序，转个vector再排序
	std::vector<std::pair<std::vector<std::string>, OperCombs>> result_vector;
	for (auto&& pair : result_map) {
		result_vector.emplace_back(std::move(pair));
	}
	std::sort(result_vector.begin(), result_vector.end(), [](const auto& lhs, const auto& rhs)
		->bool {
			// 最小等级大的，排前面
			if (lhs.second.min_level != rhs.second.min_level) {
				return lhs.second.min_level > rhs.second.min_level;
			}
			// 最大等级大的，排前面
			else if (lhs.second.max_level != rhs.second.max_level) {
				return lhs.second.max_level > rhs.second.max_level;
			}
			// 平均等级高的，排前面
			else if (std::fabs(lhs.second.avg_level - rhs.second.avg_level) < DoubleDiff) {
				return lhs.second.avg_level > rhs.second.avg_level;
			}
			// Tag数量少的，排前面
			else {
				return lhs.first.size() < rhs.first.size();
			}
		});

	/* 整理识别结果 */
	std::vector<json::value> result_json_vector;
	for (const auto& [tags_comb, oper_comb] : result_vector) {
		json::value comb_json;

		std::vector<json::value> tags_json_vector;
		for (const std::string& tag : tags_comb) {
			tags_json_vector.emplace_back(Utf8ToGbk(tag));
		}
		comb_json["tags"] = json::array(std::move(tags_json_vector));

		std::vector<json::value> opers_json_vector;
		for (const OperInfo& oper_info : oper_comb.opers) {
			json::value oper_json;
			oper_json["name"] = Utf8ToGbk(oper_info.name);
			oper_json["level"] = oper_info.level;
			opers_json_vector.emplace_back(std::move(oper_json));
		}
		comb_json["opers"] = json::array(std::move(opers_json_vector));
		comb_json["tag_level"] = oper_comb.min_level;
		result_json_vector.emplace_back(std::move(comb_json));
	}
	json::value results_json;
	results_json["result"] = json::array(std::move(result_json_vector));
	m_callback(TaskMsg::RecruitResult, results_json, m_callback_arg);

	/* 点击最优解的tags（添加点击任务） */
	if (!m_required_level.empty() && !result_vector.empty()) {
		if (std::find(m_required_level.cbegin(), m_required_level.cend(), result_vector[0].second.min_level)
			== m_required_level.cend()) {
			return true;
		}
		const std::vector<std::string>& final_tags_name = result_vector[0].first;

		json::value task_json;
		task_json["type"] = "ClickTask";
		for (const TextArea& text_area : all_tags) {
			if (std::find(final_tags_name.cbegin(), final_tags_name.cend(), text_area.text) != final_tags_name.cend()) {
				task_json["rect"] = json::array({ text_area.rect.x, text_area.rect.y, text_area.rect.width, text_area.rect.height });
				m_callback(TaskMsg::AppendTask, task_json, m_callback_arg);
			}
		}
	}

	return true;
}

void OpenRecruitTask::set_param(std::vector<int> required_level, bool set_time)
{
	m_required_level = std::move(required_level);
	m_set_time = set_time;
}

bool ClickTask::run()
{
	if (m_control_ptr == NULL)
	{
		m_callback(TaskMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}
	m_callback(TaskMsg::TaskStart, json::value(), m_callback_arg);

	m_control_ptr->click(m_rect);
	return true;
}
