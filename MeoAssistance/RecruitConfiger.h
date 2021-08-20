#pragma once

#include <string>
#include <vector>
#include <unordered_set>

#include "AsstDef.h"

namespace asst {
	class RecruitConfiger
	{
	public:
		~RecruitConfiger() = default;

		static RecruitConfiger& get_instance() {
			static RecruitConfiger unique_instance;
			return unique_instance;
		}

		bool load(const std::string& filename);

		std::unordered_set<std::string> m_all_tags;
		std::unordered_set<std::string> m_all_types;

		std::vector<OperRecruitInfo> m_all_opers;

		constexpr static int CorrectNumberOfTags = 5;

	private:
		RecruitConfiger() = default;
		RecruitConfiger(const RecruitConfiger& rhs) = default;
		RecruitConfiger(RecruitConfiger&& rhs) noexcept = default;

		RecruitConfiger& operator=(const RecruitConfiger& rhs) = default;
		RecruitConfiger& operator=(RecruitConfiger&& rhs) noexcept = default;

		bool _load(const std::string& filename);
	};
}