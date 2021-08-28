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

		void set_facility(std::string facility)
		{
			m_facility = std::move(facility);
		}
		void set_all_opers_info(std::unordered_set<OperInfrastInfo> infos) {
			m_all_opers_info = std::move(infos);
		}
	protected:
		constexpr static int SwipeMaxTimes = 17;

		//std::vector<std::string> calc_optimal_comb();

		std::string m_facility;	// 设施名（制造站、贸易站、控制中枢……）
		std::unordered_set<OperInfrastInfo> m_all_opers_info;
	};
}