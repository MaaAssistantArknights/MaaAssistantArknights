#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "AsstDef.h"

namespace asst {
	class InfrastConfiger
	{
	public:
		~InfrastConfiger() = default;

		static InfrastConfiger& get_instance() {
			static InfrastConfiger unique_instance;
			return unique_instance;
		}

		bool load(const std::string& filename);

		std::vector<std::vector<std::string>> m_mfg_combs;	// 制造站组合
		std::unordered_set<std::string> m_mfg_opers;		// 制造站所有干员
		std::string m_mfg_end;								// 识别到这个词，就认为干员遍历结束，一般用排序里最后的那个干员（？）

	private:
		InfrastConfiger() = default;
		InfrastConfiger(const InfrastConfiger& rhs) = default;
		InfrastConfiger(InfrastConfiger&& rhs) noexcept = default;

		InfrastConfiger& operator=(const InfrastConfiger& rhs) = default;
		InfrastConfiger& operator=(InfrastConfiger && rhs) noexcept = default;

		bool _load(const std::string& filename);
	};
}
