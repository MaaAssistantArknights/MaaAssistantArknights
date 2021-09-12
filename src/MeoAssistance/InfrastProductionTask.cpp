#include "InfrastProductionTask.h"

#include <thread>
#include <future>
#include <algorithm>
#include <future>
#include <list>

#include <opencv2/opencv.hpp>

#include "Configer.h"
#include "InfrastConfiger.h"
#include "WinMacro.h"
#include "Identify.h"
#include "AsstAux.h"

using namespace asst;

bool asst::InfrastProductionTask::run()
{
	if (m_controller_ptr == nullptr
		|| m_identify_ptr == nullptr)
	{
		m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}

	cv::Mat image = m_controller_ptr->get_image();
	// 先识别一下有几个制造站/贸易站
	const static std::array<std::string, 4> facility_number_key = { "02", "03", "04", "05" };
	std::vector<Rect> facility_number_rect;
	facility_number_rect.emplace_back(Rect());	// 假装给01 push一个，后面循环好写=。=

	for (const std::string& key : facility_number_key) {
		auto&& [algorithm, score, temp_rect] = m_identify_ptr->find_image(image, key);
		if (score >= Configer::TemplThresholdDefault) {
			facility_number_rect.emplace_back(temp_rect);
		}
		else {
			break;
		}
	}
	for (size_t i = 0; i != facility_number_rect.size(); ++i) {
		if (need_exit()) {
			return false;
		}
		if (i != 0) {
			// 点到这个基建
			m_controller_ptr->click(facility_number_rect[i]);
			sleep(300);
			image = m_controller_ptr->get_image();
		}
		// 如果当前界面没有添加干员的按钮，那就不换班
		auto&& [algorithm1, score1, add_rect1] = m_identify_ptr->find_image(image, "AddOperator");
		auto&& [algorithm2, score2, add_rect2] = m_identify_ptr->find_image(image, "AddOperator-Trade");
		if (score1 < Configer::TemplThresholdDefault && score2 < Configer::TemplThresholdDefault) {
			continue;
		}

		// 识别当前正在造什么
		for (const auto& [key, useless_value] : InfrastConfiger::get_instance().m_infrast_combs) {
			auto&& [algorithm, score, useless_rect] = m_identify_ptr->find_image(image, key);
			if (score >= Configer::TemplThresholdDefault) {
				m_facility = key;
				break;
			}
		}
		//点击添加干员的那个按钮
		Rect add_rect = score1 >= score2 ? std::move(add_rect1) : std::move(add_rect2);
		m_controller_ptr->click(add_rect);
		if (!sleep(2000)) {
			return false;
		}

		click_clear_button();
		sleep(300);

		auto detect_ret = swipe_and_detect();
		if (!detect_ret) {
			return false;
		}
		auto cur_opers_info = std::move(detect_ret.value());
		// TODO，可以识别一次，把前6个最优解都列出来（最多6个制造站），然后就不用每次都识别并计算最优解了
		std::list<std::string> optimal_comb = calc_optimal_comb(cur_opers_info);
		swipe_and_select(optimal_comb);
	}

	return true;
}

std::optional<std::unordered_map<std::string, OperInfrastInfo>> asst::InfrastProductionTask::swipe_and_detect()
{
	if (!swipe_to_the_left()) {
		return std::nullopt;
	}

	std::unordered_map<std::string, std::string> feature_cond = InfrastConfiger::get_instance().m_oper_name_feat;
	std::unordered_set<std::string> feature_whatever = InfrastConfiger::get_instance().m_oper_name_feat_whatever;

	std::vector<std::string> end_flag_vec = InfrastConfiger::get_instance().m_infrast_end_flag[m_facility];

	std::unordered_map<std::string, OperInfrastInfo> cur_opers_info;
	// 因为有些干员在宿舍休息，或者被其他基建占用了，所以需要重新识别一遍当前可用的干员
	for (int i = 0; i != SwipeMaxTimes; ++i) {
		if (need_exit()) {
			return std::nullopt;
		}
		const cv::Mat& image = m_controller_ptr->get_image(true);
		// 异步进行滑动操作
		std::future<bool> swipe_future = std::async(
			std::launch::async, &InfrastProductionTask::swipe, this, false);

		auto cur_name_textarea = detect_operators_name(image, feature_cond, feature_whatever);
		for (const TextArea& textarea : cur_name_textarea) {
			OperInfrastInfo info;
			// 考虑map中没有这个名字的情况：包括一开始识别漏了、抽到了新干员但没更新等，也有可能是本次识别错了
			// TODO，这里可以抛个回调出去，提示这种case
			if (m_all_opers_info.find(textarea.text) == m_all_opers_info.cend()) {
				info.name = textarea.text;
				info.elite = 0;
				info.level = 0;
			}
			else {
				info = m_all_opers_info[textarea.text];
			}
			cur_opers_info.emplace(textarea.text, std::move(info));
		}

		json::value opers_json;
		std::vector<json::value> opers_json_vec;
		for (const auto& [name, info] : cur_opers_info) {
			json::value info_json;
			info_json["name"] = info.name;
			info_json["elite"] = info.elite;
			//info_json["level"] = info.level;
			opers_json_vec.emplace_back(std::move(info_json));
		}
		opers_json["all"] = json::array(opers_json_vec);
		m_callback(AsstMsg::OpersDetected, opers_json, m_callback_arg);

		bool break_flag = false;
		// 如果找到了end_flag_vec中的名字，说明已经识别到有当前基建技能的最后一个干员了，就不用接着识别了
		auto find_iter = std::find_first_of(
			cur_opers_info.cbegin(), cur_opers_info.cend(),
			end_flag_vec.cbegin(), end_flag_vec.cend(),
			[](const auto& lhs, const auto& rhs) ->bool {
				return lhs.first == rhs;
			});
		if (find_iter != cur_opers_info.cend()) {
			break_flag = true;
		}
		// 阻塞等待异步的滑动结束
		if (!swipe_future.get()) {
			return std::nullopt;
		}
		if (break_flag) {
			break;
		}
	}
	return cur_opers_info;
}

std::list<std::string> asst::InfrastProductionTask::calc_optimal_comb(
	const std::unordered_map<std::string, OperInfrastInfo>& cur_opers_info) const
{
	// 配置文件中的干员组合，和抓出来的干员名比对，如果组合中的干员都有，那就用这个组合
	// 注意配置中的干员组合需要是有序的
	// Todo 时间复杂度起飞了，需要优化下

	OperInfrastComb three_optimal_comb;		// 三人组合的最优解
	OperInfrastComb single_optimal_comb;	// 由三个单人组成的组合 中的最优解
	for (const OperInfrastComb& comb : InfrastConfiger::get_instance().m_infrast_combs[m_facility]) {
		if (comb.comb.size() == 3) {
			int count = 0;
			for (const OperInfrastInfo& info : comb.comb) {
				// 找到了干员名，而且当前精英化等级需要大于等于配置文件中要求的精英化等级
				auto find_iter = cur_opers_info.find(info.name);
				if (find_iter != cur_opers_info.cend()
					&& find_iter->second.elite >= info.elite) {
					++count;
				}
				else {
					break;
				}
			}
			if (count == 3) {
				three_optimal_comb = comb;
				break;
			}
		}
		else if (comb.comb.size() == 1) {
			const auto& cur_info = comb.comb.at(0);
			auto find_iter = cur_opers_info.find(cur_info.name);
			if (find_iter != cur_opers_info.cend()
				&& find_iter->second.elite >= cur_info.elite) {
				single_optimal_comb.comb.emplace_back(cur_info);
				single_optimal_comb.efficiency += comb.efficiency;
				if (single_optimal_comb.comb.size() >= 3) {
					break;
				}
			}
		}
		else {
			// 配置文件中没有两人的，不可能走到这里，TODO，报错。
			// 以后游戏更新了有双人的再说，思路是一样的，找出双人组合中干员都有的+单人组合中有的。
			// 再建一个OperInfrastComb，比一下效率
		}
	}
	OperInfrastComb optimal_comb = three_optimal_comb.efficiency >= single_optimal_comb.efficiency
		? three_optimal_comb : single_optimal_comb;

	std::list<std::string> optimal_opers_name;
	for (const auto& info : optimal_comb.comb) {
		optimal_opers_name.emplace_back(info.name);
	}

	json::value opers_json;
	opers_json["comb"] = json::array(optimal_opers_name);
	m_callback(AsstMsg::InfrastComb, opers_json, m_callback_arg);

	return optimal_opers_name;
}

bool asst::InfrastProductionTask::swipe_and_select(std::list<std::string>& name_comb, int swipe_max_times)
{
	std::unordered_map<std::string, std::string> feature_cond = InfrastConfiger::get_instance().m_oper_name_feat;
	std::unordered_set<std::string> feature_whatever = InfrastConfiger::get_instance().m_oper_name_feat_whatever;
	// 一边滑动一边点击最优解中的干员
	for (int i = 0; i != swipe_max_times; ++i) {
		if (need_exit()) {
			return false;
		}
		const cv::Mat& image = m_controller_ptr->get_image(true);
		auto cur_name_textarea = detect_operators_name(image, feature_cond, feature_whatever);

		for (TextArea& text_area : cur_name_textarea) {
			// 点过了就不会再点了，直接从最优解vector里面删了
			auto iter = std::find(name_comb.begin(), name_comb.end(), text_area.text);
			if (iter != name_comb.end()) {
				m_controller_ptr->click_without_scale(text_area.rect);
				sleep(200);
				name_comb.erase(iter);
			}
		}
		if (name_comb.empty()) {
			break;
		}
		// 因为滑动和点击是矛盾的，这里没法异步做
		if (!swipe(true)) {
			return false;
		}
	}
	// 点击“确定”按钮，确定完要联网加载的，比较慢，多sleep一会
	bool ret = click_confirm_button();
	return ret && sleep(2000);
}