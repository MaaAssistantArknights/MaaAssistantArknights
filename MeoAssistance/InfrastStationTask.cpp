#include "InfrastStationTask.h"

#include <thread>
#include <future>
#include <algorithm>

#include <opencv2/opencv.hpp>

#include "Configer.h"
#include "InfrastConfiger.h"
#include "WinMacro.h"
#include "Identify.h"
#include "AsstAux.h"

using namespace asst;

asst::InfrastStationTask::InfrastStationTask(AsstCallback callback, void* callback_arg)
	: IdentifyOperTask(callback, callback_arg)
{
	m_cropped_height_ratio = 0.052;
	m_cropped_upper_y_ratio = 0.441;
	m_cropped_lower_y_ratio = 0.831;
}

bool asst::InfrastStationTask::run()
{
	if (m_view_ptr == nullptr
		|| m_identify_ptr == nullptr
		|| m_control_ptr == nullptr)
	{
		m_callback(AsstMsg::PtrIsNull, json::value(), m_callback_arg);
		return false;
	}
	auto&& [width, height] = m_view_ptr->getAdbDisplaySize();
	m_swipe_begin = Rect(width * 0.9, height * 0.5, 0, 0);
	m_swipe_end = Rect(width * 0.4, height * 0.5, 0, 0);


	// 配置文件中的干员组合，和抓出来的干员名比对，如果组合中的干员都有，那就用这个组合
	// Todo 时间复杂度起飞了，需要优化下
	std::vector<std::string> optimal_comb; // OperInfrastInfo是带精英化和等级信息的，基建里识别不到，也用不到，这里只保留干员名
	for (const auto& name_vec : InfrastConfiger::get_instance().m_infrast_combs[m_facility]) {
		int count = 0;
		std::vector<std::string> temp_comb;
		for (const OperInfrastInfo& info : name_vec) {
			if (m_all_opers_info.find(info) != m_all_opers_info.cend()) {
				++count;
				temp_comb.emplace_back(info.name);
			}
			else {
				break;
			}
		}
		if (count == temp_comb.size()) {
			optimal_comb = temp_comb;
			break;
		}
	}

	std::vector<std::string> optimal_comb_gbk;	// 给回调json用的，gbk的
	for (const std::string& name : optimal_comb)
	{
		optimal_comb_gbk.emplace_back(Utf8ToGbk(name));
	}

	json::value opers_json;
	opers_json["comb"] = json::array(optimal_comb_gbk);
	m_callback(AsstMsg::InfrastComb, opers_json, m_callback_arg);

	std::unordered_map<std::string, std::string> feature_cond = InfrastConfiger::get_instance().m_oper_name_feat;
	std::unordered_set<std::string> feature_whatever = InfrastConfiger::get_instance().m_oper_name_feat_whatever;

	// 一边滑动一边点击最优解中的干员
	for (int i = 0; i != SwipeMaxTimes; ++i) {
		const cv::Mat& image = get_format_image(true);
		auto cur_name_textarea = detect_opers(image, feature_cond, feature_whatever);

		for (TextArea& text_area : cur_name_textarea) {
			// 点过了就不会再点了，直接从最优解vector里面删了
			auto iter = std::find(optimal_comb.begin(), optimal_comb.end(), text_area.text);
			if (iter != optimal_comb.end()) {
				m_control_ptr->click(text_area.rect);
				optimal_comb.erase(iter);
			}
		}
		if (optimal_comb.empty()) {
			break;
		}
		// 因为滑动和点击是矛盾的，这里没法异步做
		if (!swipe()) {
			return false;
		}
	}
	return true;
}
