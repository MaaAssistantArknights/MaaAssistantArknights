#pragma once

#include "IdentifyOperTask.h"

namespace asst {
	// 基建进驻任务
	class InfrastStationTask : public IdentifyOperTask
	{
	public:
		InfrastStationTask(AsstCallback callback, void* callback_arg);
		virtual ~InfrastStationTask() = default;

		virtual bool run() override;

		void set_all_opers_info(std::unordered_map<std::string, OperInfrastInfo> infos) {
			m_all_opers_info = std::move(infos);
		}
	protected:
		constexpr static int SwipeMaxTimes = 17;

		// 一边滑动一边识别
		virtual std::optional<std::unordered_map<std::string, OperInfrastInfo>> swipe_and_detect() override;
		// 计算最优解干员组合
		std::list<std::string> calc_optimal_comb(const std::unordered_map<std::string, OperInfrastInfo>& cur_opers_info) const;
		// 一边滑动一边识别并点击干员名
		bool swipe_and_select(std::list<std::string>& name_comb, int swipe_max_times = SwipeMaxTimes);
		// 快速滑动到最左边
		bool swipe_to_the_left();

		std::string m_facility;	// 设施名（制造站、贸易站、控制中枢……）
		std::unordered_map<std::string, OperInfrastInfo> m_all_opers_info;
	};
}