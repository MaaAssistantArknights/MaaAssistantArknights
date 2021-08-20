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

	std::vector<std::vector<std::string>> all_oper_combs;				// 所有的干员组合
	std::unordered_map<std::string, std::string> feature_cond_default;	// 特征检测关键字，如果OCR识别到了key的内容但是却没有value的内容，则进行特征检测进一步确认
	std::unordered_set<std::string> feature_whatever_default;			// 无论如何都进行特征检测的

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
	std::unordered_map<std::string, std::string> feature_cond = feature_cond_default;
	std::unordered_set<std::string> feature_whatever = feature_whatever_default;

	auto detect_foo = [&](const cv::Mat& image) -> std::vector<TextArea> {
		std::vector<TextArea> all_text_area = ocr_detect(image);
		/* 过滤出所有制造站中的干员名 */
		std::vector<TextArea> cur_name_textarea = text_search(
			all_text_area,
			InfrastConfiger::get_instance().m_all_opers_name,
			Configer::get_instance().m_infrast_ocr_replace);

		// 用特征检测再筛选一遍OCR识别漏了的
		for (const TextArea& textarea : all_text_area) {
			for (auto iter = feature_cond.begin(); iter != feature_cond.end(); ++iter) {
				auto& [key, value] = *iter;
				// 识别到了key，但是没识别到value，这种情况就需要进行特征检测进一步确认了
				if (textarea.text.find(key) != std::string::npos
					&& textarea.text.find(value) == std::string::npos) {
					// 把key所在的矩形放大一点送去做特征检测，不需要把整张图片都送去检测
					Rect magnified_area = textarea.rect.center_zoom(2.0);
					magnified_area.x = (std::max)(0, magnified_area.x);
					magnified_area.y = (std::max)(0, magnified_area.y);
					if (magnified_area.x + magnified_area.width >= image.cols) {
						magnified_area.width = image.cols - magnified_area.x - 1;
					}
					if (magnified_area.y + magnified_area.height >= image.rows) {
						magnified_area.height = image.rows - magnified_area.y - 1;
					}
					cv::Rect cv_rect(magnified_area.x, magnified_area.y, magnified_area.width, magnified_area.height);
					// key是关键字而已，真正要识别的是value
					auto&& ret = m_identify_ptr->feature_match(image(cv_rect), value);
					if (ret) {
						cur_name_textarea.emplace_back(value, textarea.rect);
						iter = feature_cond.erase(iter);
						--iter;
					}
				}
			}
		}
		for (auto iter = feature_whatever.begin(); iter != feature_whatever.end(); ++iter) {
			auto&& ret = m_identify_ptr->feature_match(image, *iter);
			if (ret) {
				cur_name_textarea.emplace_back(std::move(ret.value()));
				iter = feature_whatever.erase(iter);
				--iter;
			}
		}

		return cur_name_textarea;
	};

	std::unordered_set<OperInfrastInfo> detected_opers;
	// 一边识别一边滑动，把所有制造站干员名字抓出来
	while (true) {

		const cv::Mat& image = get_format_image(true);
		// 异步进行滑动操作
		std::future<bool> swipe_future = std::async(std::launch::async, swipe_foo);

		auto cur_name_textarea = detect_foo(image);

		int oper_numer = detected_opers.size();
		for (const TextArea& textarea : cur_name_textarea)
		{
			cv::Rect elite_rect;
			// 因为有的名字长有的名字短，但是右对齐的，所以跟着右边走
			elite_rect.x = textarea.rect.x + textarea.rect.width - 250;
			elite_rect.y = textarea.rect.y - 200;
			if (elite_rect.x < 0 || elite_rect.y < 0) {
				continue;
			}
			elite_rect.width = 100;
			elite_rect.height = 150;
			cv::Mat elite_mat = image(elite_rect);

			// for debug
			static cv::Mat elite1 = cv::imread(GetResourceDir() + "operators\\Elite1.png");
			static cv::Mat elite2 = cv::imread(GetResourceDir() + "operators\\Elite2.png");
			auto&& [score1, point1] = m_identify_ptr->match_template(elite_mat, elite1);
			auto&& [score2, point2] = m_identify_ptr->match_template(elite_mat, elite2);
			std::cout << "elite1:" << score1 << ", elite2:" << score2 << std::endl;

			OperInfrastInfo info;
			info.name = textarea.text;
			if (score1 > score2 && score1 > 0.7) {
				info.elite = 1;
			}
			else if (score2 > score1 && score2 > 0.7) {
				info.elite = 2;
			}
			else {
				info.elite = 0;
			}
			detected_opers.emplace(std::move(info));
		}

		json::value opers_json;
		std::vector<json::value> opers_json_vec;
		for (const OperInfrastInfo& info : detected_opers) {
			json::value info_json;
			info_json["name"] = Utf8ToGbk(info.name);
			info_json["elite"] = info.elite;
			//info_json["level"] = info.level;
			opers_json_vec.emplace_back(std::move(info_json));
		}
		opers_json["all"] = json::array(opers_json_vec);
		m_callback(AsstMsg::InfrastOpers, opers_json, m_callback_arg);

		// 阻塞等待滑动结束
		if (!swipe_future.get()) {
			return false;
		}
		// 说明本次识别一个新的都没识别到，应该是滑动到最后了，直接结束循环
		if (oper_numer == detected_opers.size()) {
			break;
		}
	}

	//// 配置文件中的干员组合，和抓出来的干员名比对，如果组合中的干员都有，那就用这个组合
	//// Todo 时间复杂度起飞了，需要优化下
	//std::vector<std::string> optimal_comb;
	//for (auto&& name_vec : all_oper_combs) {
	//	int count = 0;
	//	for (std::string& name : name_vec) {
	//		if (detected_names.find(name) != detected_names.cend()) {
	//			++count;
	//		}
	//		else {
	//			break;
	//		}
	//	}
	//	if (count == name_vec.size()) {
	//		optimal_comb = name_vec;
	//		break;
	//	}
	//}

	//std::vector<std::string> optimal_comb_gbk;	// 给回调json用的，gbk的
	//for (const std::string& name : optimal_comb)
	//{
	//	optimal_comb_gbk.emplace_back(Utf8ToGbk(name));
	//}

	//opers_json["comb"] = json::array(optimal_comb_gbk);
	//m_callback(AsstMsg::InfrastComb, opers_json, m_callback_arg);

	//// 重置特征检测的条件，后面不用了，这次直接move
	//feature_cond = std::move(feature_cond_default);
	//feature_whatever = std::move(feature_whatever_default);

	//// 一边滑动一边点击最优解中的干员
	//for (int i = 0; i != m_swipe_max_times; ++i) {
	//	const cv::Mat& image = get_format_image(true);
	//	auto cur_name_textarea = detect_foo(image);

	//	for (TextArea& text_area : cur_name_textarea) {
	//		// 点过了就不会再点了，直接从最优解vector里面删了
	//		auto iter = std::find(optimal_comb.begin(), optimal_comb.end(), text_area.text);
	//		if (iter != optimal_comb.end()) {
	//			m_control_ptr->click(text_area.rect);
	//			optimal_comb.erase(iter);
	//		}
	//	}
	//	if (optimal_comb.empty()) {
	//		break;
	//	}
	//	// 因为滑动和点击是矛盾的，这里没法异步做
	//	if (!swipe_foo(true)) {
	//		return false;
	//	}
	//}
	return true;
}
