#pragma once

#include "AbstractConfiger.h"

#include <string>
#include <vector>
#include <unordered_set>

#include "AsstDef.h"

namespace asst {
	class RecruitConfiger : public AbstractConfiger
	{
	public:
		virtual ~RecruitConfiger() = default;

		static RecruitConfiger& get_instance() {
			static RecruitConfiger unique_instance;
			return unique_instance;
		}

		std::unordered_set<std::string> m_all_tags;
		std::unordered_set<std::string> m_all_types;

		std::vector<OperRecruitInfo> m_all_opers;

		constexpr static int CorrectNumberOfTags = 5;

	protected:
		RecruitConfiger() = default;
		RecruitConfiger(const RecruitConfiger& rhs) = default;
		RecruitConfiger(RecruitConfiger&& rhs) noexcept = default;

		RecruitConfiger& operator=(const RecruitConfiger& rhs) = default;
		RecruitConfiger& operator=(RecruitConfiger&& rhs) noexcept = default;

		virtual bool parse(json::value&& json) override;
	};
}