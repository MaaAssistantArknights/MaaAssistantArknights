#pragma once

#include "InfrastAbstractTask.h"

#include "AsstInfrastDef.h"

namespace asst
{
	// 生产类设施的任务，适用于制造站/贸易站
	class InfrastProductionTask : public InfrastAbstractTask
	{
	public:
		using InfrastAbstractTask::InfrastAbstractTask;
		virtual ~InfrastProductionTask() override = default;

		InfrastProductionTask& set_uses_of_drone(std::string uses_of_drones) noexcept;
		std::string get_uses_of_drone() const noexcept;

	protected:
		bool shift_facility_list();
		bool facility_list_detect();
		bool opers_detect_with_swipe();
		size_t opers_detect(); // 返回当前页面的干员数
		bool optimal_calc();
		bool opers_choose();
		bool use_drone();
		void set_product(std::string product_name) noexcept;

		infrast::SkillsComb efficient_regex_calc(
			std::unordered_set<infrast::Skill> skills) const;

		std::string m_product;
		std::string m_uses_of_drones;
		int m_cur_num_of_locked_opers = 0;
		std::vector<infrast::Oper> m_all_available_opers;
		std::vector<infrast::SkillsComb> m_optimal_combs;
		std::vector<Rect> m_facility_list_tabs;
		size_t max_num_of_opers_per_page = 0;
	};
}
