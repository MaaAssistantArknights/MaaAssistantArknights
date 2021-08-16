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

		std::vector<std::vector<std::string>> m_mfg_combs;			// 制造站组合
		std::unordered_set<std::string> m_mfg_opers;				// 制造站所有干员
		std::unordered_map<std::string, std::string> m_mfg_feat;	// 特征检测关键字，如果OCR识别到了key的内容但是却没有value的内容，则进行特征检测进一步确认
		std::unordered_set<std::string> m_mfg_feat_whatever;		// 无论如何都进行特征检测的干员名
		std::string m_mfg_end;										// 识别到这个词，就认为干员遍历结束，一般用排序里最后的那个干员（？）

	private:
		InfrastConfiger() = default;
		InfrastConfiger(const InfrastConfiger& rhs) = default;
		InfrastConfiger(InfrastConfiger&& rhs) noexcept = default;

		InfrastConfiger& operator=(const InfrastConfiger& rhs) = default;
		InfrastConfiger& operator=(InfrastConfiger && rhs) noexcept = default;

		bool _load(const std::string& filename);
	};
}
