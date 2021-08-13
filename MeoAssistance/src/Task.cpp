#include "Task.h"
#include "AsstDef.h"
#include "WinMacro.h"
#include "Identify.h"
#include "Configer.h"
#include "RecruitConfiger.h"
#include "InfrastConfiger.h"
#include <json.h>
#include "AsstAux.h"

#include <chrono>
#include <thread>
#include <utility>
#include <cmath>
#include <mutex>
#include <filesystem>
#include <future>

using namespace asst;

AbstractTask::AbstractTask(AsstCallback callback, void* callback_arg)
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
		m_callback(AsstMsg::ImageIsEmpty, json::value(), m_callback_arg);
		return raw_image;
	}
	if (raw_image.rows < 100) {
		m_callback(AsstMsg::WindowMinimized, json::value(), m_callback_arg);
		return raw_image;
	}

	// 把模拟器边框的一圈裁剪掉
	const EmulatorInfo& window_info = m_view_ptr->getEmulatorInfo();
	int x_offset = window_info.x_offset;
	int y_offset = window_info.y_offset;
	int width = raw_image.cols - x_offset - window_info.right_offset;
	int height = raw_image.rows - y_offset - window_info.bottom_offset;

	cv::Mat cropped(raw_image, cv::Rect(x_offset, y_offset, width, height));

	// 根据图像尺寸，调整控制的缩放
	set_control_scale(cropped.cols, cropped.rows);

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

bool AbstractTask::sleep(unsigned millisecond)
{
	if (millisecond == 0) {
		return true;
	}
	auto start = std::chrono::system_clock::now();
	unsigned duration = 0;

	json::value callback_json;
	callback_json["time"] = millisecond;
	m_callback(AsstMsg::ReadyToSleep, callback_json, m_callback_arg);

	while ((m_exit_flag == NULL || *m_exit_flag == false)
		&& duration < millisecond) {
		duration = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now() - start).count();
		std::this_thread::yield();
	}
	m_callback(AsstMsg::EndOfSleep, callback_json, m_callback_arg);

	return (m_exit_flag == NULL || *m_exit_flag == false);
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
	m_callback(AsstMsg::PrintWindow, callback_json, m_callback_arg);

	return ret;
}

ProcessTask::ProcessTask(AsstCallback callback, void* callback_arg)
	: OcrAbstractTask(callback, callback_arg)
{
	m_task_type = TaskType::TaskTypeRecognition | TaskType::TaskTypeClick;
}

bool ProcessTask::run()
{
	if (m_view_ptr == NULL
		|| m_control_ptr == NULL
		|| m_identify_ptr == NULL
		|| m_control_ptr == NULL)
	{
		m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}

	json::value task_start_json = json::object{
		{ "task_type",  "ProcessTask" },
		{ "task_chain", m_task_chain},
	};
	m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

	std::shared_ptr<TaskInfo> task_info_ptr = nullptr;

	Rect rect;
	task_info_ptr = match_image(&rect);
	if (!task_info_ptr) {
		return false;
	}

	json::value callback_json = json::object{
		{ "name", task_info_ptr->name },
		{ "type", static_cast<int>(task_info_ptr->type) },
		{ "exec_times", task_info_ptr->exec_times },
		{ "max_times", task_info_ptr->max_times },
		{ "task_type", "ProcessTask"},
		{ "algorithm", static_cast<int>(task_info_ptr->algorithm)}
	};
	m_callback(AsstMsg::TaskMatched, callback_json, m_callback_arg);

	if (task_info_ptr->exec_times >= task_info_ptr->max_times)
	{
		m_callback(AsstMsg::ReachedLimit, callback_json, m_callback_arg);

		json::value next_json = callback_json;
		next_json["tasks"] = json::array(task_info_ptr->exceeded_next);
		next_json["retry_times"] = m_retry_times;
		next_json["task_chain"] = m_task_chain;
		m_callback(AsstMsg::AppendProcessTask, next_json, m_callback_arg);
		return true;
	}

	// 前置固定延时
	if (!sleep(task_info_ptr->pre_delay))
	{
		return false;
	}

	bool need_stop = false;
	switch (task_info_ptr->type) {
	case ProcessTaskType::ClickRect:
		rect = task_info_ptr->specific_area;
		[[fallthrough]];
	case ProcessTaskType::ClickSelf:
		exec_click_task(rect);
		break;
	case ProcessTaskType::DoNothing:
		break;
	case ProcessTaskType::Stop:
		m_callback(AsstMsg::TaskStop, json::object{ {"task_chain", m_task_chain} }, m_callback_arg);
		need_stop = true;
		break;
	case ProcessTaskType::PrintWindow:
	{
		if (!sleep(Configer::get_instance().m_options.print_window_delay))
		{
			return false;
		}
		static const std::string dirname = GetCurrentDir() + "screenshot\\";
		print_window(dirname);
	}
	break;
	default:
		break;
	}

	++task_info_ptr->exec_times;

	// 减少其他任务的执行次数
	// 例如，进入吃理智药的界面了，相当于上一次点蓝色开始行动没生效
	// 所以要给蓝色开始行动的次数减一
	for (const std::string& reduce : task_info_ptr->reduce_other_times) {
		--Configer::get_instance().m_all_tasks_info[reduce]->exec_times;
	}

	if (need_stop) {
		return true;
	}

	callback_json["exec_times"] = task_info_ptr->exec_times;
	m_callback(AsstMsg::TaskCompleted, callback_json, m_callback_arg);

	// 后置固定延时
	if (!sleep(task_info_ptr->rear_delay)) {
		return false;
	}

	json::value next_json = callback_json;
	next_json["task_chain"] = m_task_chain;
	next_json["retry_times"] = m_retry_times;
	next_json["tasks"] = json::array(task_info_ptr->next);
	m_callback(AsstMsg::AppendProcessTask, next_json, m_callback_arg);

	return true;
}

std::shared_ptr<TaskInfo> ProcessTask::match_image(Rect* matched_rect)
{
	//// 如果当前仅有一个任务，且这个任务的阈值是0（说明是justreturn的），就不抓图像识别了，直接返回就行
	//if (m_cur_tasks_name.size() == 1)
	//{
	//	// 能走到这个函数里的，肯定是ProcessTaskInfo，直接转
	//	auto task_info_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
	//		Configer::get_instance().m_all_tasks_info[m_cur_tasks_name[0]]);
	//	if (task_info_ptr->templ_threshold == 0) {
	//		json::value callback_json;
	//		callback_json["threshold"] = 0.0;
	//		callback_json["algorithm"] = "JustReturn";
	//		callback_json["rect"] = json::array({ 0, 0, 0, 0 });
	//		callback_json["name"] = m_cur_tasks_name[0];
	//		callback_json["algorithm_id"] = static_cast<std::underlying_type<ProcessTaskType>::type>(
	//			AlgorithmType::JustReturn);
	//		callback_json["value"] = 0;

	//		m_callback(AsstMsg::ImageFindResult, callback_json, m_callback_arg);
	//		m_callback(AsstMsg::ImageMatched, callback_json, m_callback_arg);
	//		return task_info_ptr;
	//	}
	//}

	const cv::Mat& cur_image = get_format_image();
	if (cur_image.empty() || cur_image.rows < 100) {
		return nullptr;
	}

	// 逐个匹配当前可能的任务
	for (const std::string& task_name : m_cur_tasks_name) {
		std::shared_ptr<TaskInfo> task_info_ptr = Configer::get_instance().m_all_tasks_info[task_name];
		if (task_info_ptr == nullptr) {
			m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
			continue;
		}
		json::value callback_json;
		bool matched = false;
		Rect rect;

		switch (task_info_ptr->algorithm)
		{
		case AlgorithmType::JustReturn:
			callback_json["algorithm"] = "JustReturn";
			matched = true;
			break;
		case AlgorithmType::MatchTemplate:
		{
			std::shared_ptr<MatchTaskInfo> process_task_info_ptr =
				std::dynamic_pointer_cast<MatchTaskInfo>(task_info_ptr);
			double templ_threshold = process_task_info_ptr->templ_threshold;
			double hist_threshold = process_task_info_ptr->hist_threshold;

			auto&& [algorithm, value, temp_rect] = m_identify_ptr->find_image(cur_image, task_name, templ_threshold);
			rect = std::move(temp_rect);
			callback_json["value"] = value;

			if (algorithm == AlgorithmType::MatchTemplate) {
				callback_json["threshold"] = templ_threshold;
				callback_json["algorithm"] = "MatchTemplate";
				if (value >= templ_threshold) {
					matched = true;
				}
			}
			else if (algorithm == AlgorithmType::CompareHist) {
				callback_json["threshold"] = hist_threshold;
				callback_json["algorithm"] = "CompareHist";
				if (value >= hist_threshold) {
					matched = true;
				}
			}
			else {
				continue;
			}
		}
		break;
		case AlgorithmType::OcrDetect:
		{
			std::shared_ptr<OcrTaskInfo> ocr_task_info_ptr =
				std::dynamic_pointer_cast<OcrTaskInfo>(task_info_ptr);
			std::vector<TextArea> all_text_area = ocr_detect(cur_image);
			std::vector<TextArea> match_result;
			if (ocr_task_info_ptr->need_match) {
				match_result = text_match(all_text_area,
					ocr_task_info_ptr->text, ocr_task_info_ptr->replace_map);
			}
			else {
				match_result = text_search(all_text_area,
					ocr_task_info_ptr->text, ocr_task_info_ptr->replace_map);
			}
			// TODO：如果搜出来多个结果，怎么处理？
			// 暂时通过配置文件，保证尽量只搜出来一个结果or一个都搜不到
			if (!match_result.empty()) {
				callback_json["text"] = Utf8ToGbk(match_result.at(0).text);
				rect = match_result.at(0).rect;
				matched = true;
			}
		}
		break;
		//CompareHist是MatchTemplate的衍生算法，不应作为单独的配置参数出现
		//case AlgorithmType::CompareHist:
		//	break;
		default:
			// TODO：抛个报错的回调出去
			break;
		}

		callback_json["rect"] = json::array({ rect.x, rect.y, rect.width, rect.height });
		callback_json["name"] = task_name;
		if (matched_rect != NULL) {
			*matched_rect = std::move(rect);
		}
		m_callback(AsstMsg::ImageFindResult, callback_json, m_callback_arg);
		if (matched) {
			m_callback(AsstMsg::ImageMatched, callback_json, m_callback_arg);
			return task_info_ptr;
		}
	}
	return nullptr;
}

void ProcessTask::exec_click_task(const Rect& matched_rect)
{
	// 随机延时功能
	if (Configer::get_instance().m_options.control_delay_upper != 0) {
		static std::default_random_engine rand_engine(
			std::chrono::system_clock::now().time_since_epoch().count());
		static std::uniform_int_distribution<unsigned> rand_uni(
			Configer::get_instance().m_options.control_delay_lower,
			Configer::get_instance().m_options.control_delay_upper);

		unsigned rand_delay = rand_uni(rand_engine);
		if (!sleep(rand_delay)) {
			return;
		}
	}

	m_control_ptr->click(matched_rect);
}

OcrAbstractTask::OcrAbstractTask(AsstCallback callback, void* callback_arg)
	: AbstractTask(callback, callback_arg)
{
	;
}

std::vector<TextArea> OcrAbstractTask::ocr_detect()
{
	const cv::Mat& image = get_format_image();

	return ocr_detect(image);
}

std::vector<TextArea> OcrAbstractTask::ocr_detect(const cv::Mat& image)
{
	std::vector<TextArea> dst = m_identify_ptr->ocr_detect(image);

	std::vector<json::value> all_text_json_vector;
	for (const TextArea& text_area : dst) {
		all_text_json_vector.emplace_back(Utf8ToGbk(text_area.text));
	}
	json::value all_text_json;
	all_text_json["text"] = json::array(all_text_json_vector);
	all_text_json["task_chain"] = m_task_chain;
	m_callback(AsstMsg::TextDetected, all_text_json, m_callback_arg);

	return dst;
}

OpenRecruitTask::OpenRecruitTask(AsstCallback callback, void* callback_arg)
	: OcrAbstractTask(callback, callback_arg)
{
	m_task_type = TaskType::TaskTypeRecognition & TaskType::TaskTypeClick;
}

bool OpenRecruitTask::run()
{
	if (m_view_ptr == NULL
		|| m_identify_ptr == NULL)
	{
		m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}

	json::value task_start_json = json::object{
		{ "task_type",  "OpenRecruitTask" },
		{ "task_chain", m_task_chain},
	};
	m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

	/* Find all text */
	std::vector<TextArea> all_text_area = ocr_detect();

	/* Filter out all tags from all text */
	std::vector<TextArea> all_tags = text_match(
		all_text_area, RecruitConfiger::get_instance().m_all_tags, Configer::get_instance().m_recruit_ocr_replace);

	std::unordered_set<std::string> all_tags_name;
	std::vector<json::value> all_tags_json_vector;
	for (const TextArea& text_area : all_tags) {
		all_tags_name.emplace(text_area.text);
		all_tags_json_vector.emplace_back(Utf8ToGbk(text_area.text));
	}
	json::value all_tags_json;
	all_tags_json["tags"] = json::array(all_tags_json_vector);
	m_callback(AsstMsg::RecruitTagsDetected, all_tags_json, m_callback_arg);

	/* 过滤tags数量不足的情况（可能是识别漏了） */
	if (all_tags.size() != RecruitConfiger::CorrectNumberOfTags) {
		all_tags_json["type"] = "OpenRecruit";
		m_callback(AsstMsg::OcrResultError, all_tags_json, m_callback_arg);
		return false;
	}

	/* 设置招募时间9小时，加入任务队列*/
	if (m_set_time) {
		json::value settime_json;
		settime_json["task"] = "RecruitTime";
		// 只有tag数量对了才能走到这里，界面一定是对的，所以找不到时间设置就说明时间已经手动修改过了，不用重试了
		settime_json["retry_times"] = 0;
		settime_json["task_chain"] = m_task_chain;
		m_callback(AsstMsg::AppendProcessTask, settime_json, m_callback_arg);
	}

	/* 针对一星干员的额外回调消息 */
	static const std::string SupportMachine_GBK = "支援机械";
	static const std::string SupportMachine = GbkToUtf8(SupportMachine_GBK);
	if (std::find(all_tags_name.cbegin(), all_tags_name.cend(), SupportMachine) != all_tags_name.cend()) {
		json::value special_tag_json;
		special_tag_json["tag"] = SupportMachine_GBK;
		m_callback(AsstMsg::RecruitSpecialTag, special_tag_json, m_callback_arg);
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
		for (const OperInfo& cur_oper : RecruitConfiger::get_instance().m_all_opers) {
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
	m_callback(AsstMsg::RecruitResult, results_json, m_callback_arg);

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
				task_json["retry_times"] = m_retry_times;
				task_json["task_chain"] = m_task_chain;
				m_callback(AsstMsg::AppendTask, task_json, m_callback_arg);
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

ClickTask::ClickTask(AsstCallback callback, void* callback_arg)
	: AbstractTask(callback, callback_arg)
{
	m_task_type = TaskType::TaskTypeClick;
}

bool ClickTask::run()
{
	if (m_control_ptr == NULL)
	{
		m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}
	json::value task_start_json = json::object{
		{ "task_type",  "ClickTask" },
		{ "task_chain", m_task_chain},
	};
	m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

	m_control_ptr->click(m_rect);
	return true;
}

TestOcrTask::TestOcrTask(AsstCallback callback, void* callback_arg)
	: OcrAbstractTask(callback, callback_arg)
{
	m_task_type = TaskType::TaskTypeRecognition & TaskType::TaskTypeClick;
}

bool TestOcrTask::run()
{
	if (m_view_ptr == NULL
		|| m_identify_ptr == NULL)
	{
		m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}

	json::value task_start_json = json::object{
		{ "task_type",  "TestOcrTask" },
		{ "task_chain", m_task_chain},
	};
	m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

	auto swipe_foo = [&](bool reverse = false) -> bool {
		const Point p1(600, 300);
		const Point p2(450, 300);
		bool ret = false;
		if (!reverse) {
			ret = m_control_ptr->swipe(p1, p2);
		}
		else {
			ret = m_control_ptr->swipe(p2, p1);
		}
		ret &= sleep(3000);
		return ret;
	};

	// 识别到的干员名
	std::unordered_set<std::string> all_names;

	// 一边识别一边滑动，把所有制造站干员名字抓出来
	for (int i = 0; i != 20; ++i) {
		const cv::Mat& image = get_format_image();
		// 异步进行滑动操作
		std::future<bool> swipe_future = std::async(std::launch::async, swipe_foo);

		std::vector<TextArea> all_text_area = ocr_detect(image);
		/* 过滤出所有制造站中的干员名 */
		std::vector<TextArea> cur_names = text_search(
			all_text_area,
			InfrastConfiger::get_instance().m_mfg_opers,
			Configer::get_instance().m_infrast_ocr_replace);

		for (TextArea& text_area : cur_names) {
			all_names.emplace(std::move(text_area.text));
		}
		bool break_flag = false;
		// 识别到了结束标记，就直接退出循环
		if (all_names.find(InfrastConfiger::get_instance().m_mfg_end) != all_names.cend()) {
			break_flag = true;
		}
		// 阻塞等待滑动结束
		if (!swipe_future.get()) {
			return false;
		}

		if (break_flag) {
			break;
		}
	}

	std::cout << "识别到制造站干员:" << std::endl;
	for (const std::string& name : all_names) {
		std::cout << Utf8ToGbk(name) << std::endl;
	}

	// 配置文件中的干员组合，和抓出来的干员名比对，如果组合中的干员都有，那就用这个组合
	// Todo 时间复杂度起飞了，需要优化下
	std::vector<std::string> optimal_comb;
	for (auto&& name_vec : InfrastConfiger::get_instance().m_mfg_combs) {
		int count = 0;
		for (std::string& name : name_vec) {
			if (all_names.find(name) != all_names.cend()) {
				++count;
			}
			else {
				break;
			}
		}
		if (count == name_vec.size()) {
			optimal_comb = name_vec;
			break;
		}
	}

	std::cout << "最优组合:" << std::endl;
	for (const std::string& name : optimal_comb) {
		std::cout << Utf8ToGbk(name) << std::endl;
	}

	// 一边滑动一边点击最优解中的干员
	for (int i = 0; i != 20; ++i) {
		const cv::Mat& image = get_format_image();
		std::vector<TextArea> all_text_area = ocr_detect(image);
		/* 过滤出所有制造站中的干员名 */
		std::vector<TextArea> cur_names = text_search(
			all_text_area,
			optimal_comb,
			Configer::get_instance().m_infrast_ocr_replace);

		for (TextArea& text_area : cur_names) {
			m_control_ptr->click(text_area.rect);
			// 点过了就不会再点了，直接从最优解vector里面删了
			auto iter = std::find(optimal_comb.begin(), optimal_comb.end(), text_area.text);
			if (iter != optimal_comb.end()) {
				optimal_comb.erase(iter);
			}
		}
		if (optimal_comb.empty()) {
			break;
		}
		// 因为滑动和点击是矛盾的，这里没法异步做
		if (!swipe_foo(true)) {
			return false;
		}
	}


	//// 点击识别到的文字，直接回调扔出去，交给外层做
	//if (m_need_click) {
	//	for (const TextArea& text_area : all_text) {
	//		json::value task_json;
	//		task_json["type"] = "ClickTask";
	//		task_json["rect"] = json::array({ text_area.rect.x, text_area.rect.y, text_area.rect.width, text_area.rect.height });
	//		task_json["retry_times"] = m_retry_times;
	//		task_json["task_chain"] = m_task_chain;
	//		m_callback(AsstMsg::AppendTask, task_json, m_callback_arg);
	//	}
	//}

	return true;
}

asst::InfrastStationTask::InfrastStationTask(AsstCallback callback, void* callback_arg)
	: OcrAbstractTask(callback, callback_arg)
{
}

bool asst::InfrastStationTask::run()
{
	if (m_view_ptr == NULL
		|| m_identify_ptr == NULL)
	{
		m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}
	
	std::vector<std::vector<std::string>> all_oper_combs;	// 所有的干员组合
	std::unordered_set<std::string> all_oper_name;			// 所有干员名
	std::string oper_end_flag;								// 干员名结束标记，识别到这个string就认为识别完成了

	switch (m_facility) {
	case FacilityType::Manufacturing:
		all_oper_combs = InfrastConfiger::get_instance().m_mfg_combs;
		all_oper_name = InfrastConfiger::get_instance().m_mfg_opers;
		oper_end_flag = InfrastConfiger::get_instance().m_mfg_end;
		break;
		// TODO 贸易站和其他啥的，有空再做
	default:
		break;
	}

	json::value task_start_json = json::object{
		{ "task_type",  "InfrastStationTask" },
		{ "task_chain", m_task_chain},
	};
	m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

	auto swipe_foo = [&](bool reverse = false) -> bool {
		bool ret = false;
		if (!reverse) {
			ret = m_control_ptr->swipe(m_swipe_begin, m_swipe_end);
		}
		else {
			ret = m_control_ptr->swipe(m_swipe_end, m_swipe_begin);
		}
		ret &= sleep(m_swipe_delay);
		return ret;
	};

	// 识别到的干员名
	std::unordered_set<std::string> detected_names;

	// 一边识别一边滑动，把所有制造站干员名字抓出来
	for (int i = 0; i != m_swipe_max_times; ++i) {
		const cv::Mat& image = get_format_image();
		// 异步进行滑动操作
		std::future<bool> swipe_future = std::async(std::launch::async, swipe_foo);

		std::vector<TextArea> all_text_area = ocr_detect(image);
		/* 过滤出所有制造站中的干员名 */
		std::vector<TextArea> cur_names = text_search(
			all_text_area,
			all_oper_name,
			Configer::get_instance().m_infrast_ocr_replace);

		for (TextArea& text_area : cur_names) {
			detected_names.emplace(std::move(text_area.text));
		}
		bool break_flag = false;
		// 识别到了结束标记，就直接退出循环
		if (detected_names.find(oper_end_flag) != detected_names.cend()) {
			break_flag = true;
		}
		// 阻塞等待滑动结束
		if (!swipe_future.get()) {
			return false;
		}

		if (break_flag) {
			break;
		}
	}

	json::value opers_json;
	std::vector<json::value> all_names_vector;
	for (const std::string& name : detected_names) {
		all_names_vector.emplace_back(Utf8ToGbk(name));
	}
	opers_json["names"] = json::array(all_names_vector);

	m_callback(AsstMsg::InfrastOpers, opers_json, m_callback_arg);

	// 配置文件中的干员组合，和抓出来的干员名比对，如果组合中的干员都有，那就用这个组合
	// Todo 时间复杂度起飞了，需要优化下
	std::vector<std::string> optimal_comb;
	for (auto&& name_vec :all_oper_combs) {
		int count = 0;
		for (std::string& name : name_vec) {
			if (detected_names.find(name) != detected_names.cend()) {
				++count;
			}
			else {
				break;
			}
		}
		if (count == name_vec.size()) {
			optimal_comb = name_vec;
			break;
		}
	}

	opers_json["comb"] = json::array(optimal_comb);
	m_callback(AsstMsg::InfrastComb, opers_json, m_callback_arg);


	// 一边滑动一边点击最优解中的干员
	for (int i = 0; i != m_swipe_max_times; ++i) {
		const cv::Mat& image = get_format_image();
		std::vector<TextArea> all_text_area = ocr_detect(image);
		/* 过滤出所有制造站中的干员名 */
		std::vector<TextArea> cur_names = text_search(
			all_text_area,
			optimal_comb,
			Configer::get_instance().m_infrast_ocr_replace);

		for (TextArea& text_area : cur_names) {
			m_control_ptr->click(text_area.rect);
			// 点过了就不会再点了，直接从最优解vector里面删了
			auto iter = std::find(optimal_comb.begin(), optimal_comb.end(), text_area.text);
			if (iter != optimal_comb.end()) {
				optimal_comb.erase(iter);
			}
		}
		if (optimal_comb.empty()) {
			break;
		}
		// 因为滑动和点击是矛盾的，这里没法异步做
		if (!swipe_foo(true)) {
			return false;
		}
	}
	return true;
}
