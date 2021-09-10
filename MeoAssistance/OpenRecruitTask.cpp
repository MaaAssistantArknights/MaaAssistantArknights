#include "OpenRecruitTask.h"

#include <map>
#include <vector>
#include <algorithm>
#include <future>

#include "WinMacro.h"
#include "Configer.h"
#include "Identify.h"
#include "RecruitConfiger.h"
#include "AsstAux.h"

using namespace asst;

bool OpenRecruitTask::run()
{
	if (m_controller_ptr == NULL
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

	const cv::Mat& image = m_controller_ptr->get_image();

	/* 设置招募时间9小时 */
	auto set_time_foo = [&](bool flag) -> void {
		if (!flag) {
			return;
		}
		auto&& [algorithm, score, second_confirm_rect] =
			m_identify_ptr->find_image(m_controller_ptr->get_image(), "RecruitTime");

		auto time_reduce_task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
			Configer::get_instance().m_all_tasks_info["RecruitTime"]);

		if (score >= time_reduce_task_ptr->templ_threshold) {
			m_controller_ptr->click(time_reduce_task_ptr->specific_area);
		}
	};
	std::future<void> set_time_future = std::async(std::launch::async, set_time_foo, m_set_time);

	/* Find all text */
	std::vector<TextArea> all_text_area = ocr_detect(image);

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

	/* 过滤tags数量不足的情况（可能是识别漏了） */
	if (all_tags.size() != RecruitConfiger::CorrectNumberOfTags) {
		all_tags_json["type"] = "OpenRecruit";
		m_callback(AsstMsg::OcrResultError, all_tags_json, m_callback_arg);
		return false;
	}

	m_callback(AsstMsg::RecruitTagsDetected, all_tags_json, m_callback_arg);

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
	// 例如 key: { "狙击"、"群攻" }，value: OperRecruitCombs.opers{ "陨星", "白雪", "空爆" }
	std::map<std::vector<std::string>, OperRecruitCombs> result_map;
	for (const std::vector<std::string>& comb : all_combs) {
		for (const OperRecruitInfo& cur_oper : RecruitConfiger::get_instance().m_all_opers) {
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

			OperRecruitCombs& oper_combs = result_map[comb];
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
			OperRecruitCombs& oper_combs = result_map[comb];
			oper_combs.avg_level /= oper_combs.opers.size();
		}
	}

	// map没法按值排序，转个vector再排序
	std::vector<std::pair<std::vector<std::string>, OperRecruitCombs>> result_vector;
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
			else if (std::fabs(lhs.second.avg_level - rhs.second.avg_level) > DoubleDiff) {
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
		for (const OperRecruitInfo& oper_info : oper_comb.opers) {
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

	set_time_future.wait();

	/* 点击最优解的tags */
	if (!m_required_level.empty() && !result_vector.empty()) {
		if (std::find(m_required_level.cbegin(), m_required_level.cend(), result_vector[0].second.min_level)
			== m_required_level.cend()) {
			return true;
		}
		const std::vector<std::string>& final_tags_name = result_vector[0].first;

		for (const TextArea& text_area : all_tags) {
			if (std::find(final_tags_name.cbegin(), final_tags_name.cend(), text_area.text) != final_tags_name.cend()) {
				m_controller_ptr->click(text_area.rect);
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