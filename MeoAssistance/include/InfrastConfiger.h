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
		std::unordered_set<std::string> m_all_opers_name;				// 所有干员的名字
		std::unordered_map<std::string, std::string> m_oper_name_feat;	// 根据关键字需要特征检测干员名：如果OCR识别到了key的内容但是却没有value的内容，则进行特征检测进一步确认
		std::unordered_set<std::string> m_oper_name_feat_whatever;		// 无论如何都进行特征检测的干员名

		std::unordered_map<std::string, std::vector<std::vector<OperInfrastInfo>>> m_infrast_combs;		// 各个设施内的可能干员组合

	private:
		InfrastConfiger() = default;
		InfrastConfiger(const InfrastConfiger& rhs) = default;
		InfrastConfiger(InfrastConfiger&& rhs) noexcept = default;

		InfrastConfiger& operator=(const InfrastConfiger& rhs) = default;
		InfrastConfiger& operator=(InfrastConfiger && rhs) noexcept = default;

		bool _load(const std::string& filename);
	};
}
