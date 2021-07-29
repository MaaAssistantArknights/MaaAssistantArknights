#pragma once

#include <string>
#include <vector>
#include <unordered_set>

#include "AsstDef.h"

namespace asst {
	class RecruitConfiger
	{
	public:

		RecruitConfiger() = default;
		~RecruitConfiger() = default;

		RecruitConfiger(const RecruitConfiger& rhs) = default;
		RecruitConfiger(RecruitConfiger&& rhs) noexcept = default;

		bool load(const std::string& filename);


		RecruitConfiger& operator=(const RecruitConfiger& rhs) = default;
		RecruitConfiger& operator=(RecruitConfiger&& rhs) noexcept = default;

		std::unordered_set<std::string> m_all_tags;
		std::unordered_set<std::string> m_all_types;

		std::vector<OperInfo> m_opers;

	private:
		bool _load(const std::string& filename);
	};

}