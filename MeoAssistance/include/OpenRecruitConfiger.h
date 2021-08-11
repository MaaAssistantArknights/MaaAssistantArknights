#pragma once

#include <string>
#include <vector>
#include <unordered_set>

#include "AsstDef.h"

namespace asst {
	class OpenRecruitConfiger
	{
	public:
		~OpenRecruitConfiger() = default;

		static OpenRecruitConfiger& get_instance() {
			static OpenRecruitConfiger unique_instance;
			return unique_instance;
		}

		bool load(const std::string& filename);

		OpenRecruitConfiger& operator=(const OpenRecruitConfiger& rhs) = default;
		OpenRecruitConfiger& operator=(OpenRecruitConfiger&& rhs) noexcept = default;

		std::unordered_set<std::string> m_all_tags;
		std::unordered_set<std::string> m_all_types;

		std::vector<OperInfo> m_all_opers;

		constexpr static int CorrectNumberOfTags = 5;

	private:
		OpenRecruitConfiger() = default;
		OpenRecruitConfiger(const OpenRecruitConfiger& rhs) = default;
		OpenRecruitConfiger(OpenRecruitConfiger&& rhs) noexcept = default;

		bool _load(const std::string& filename);
	};
}