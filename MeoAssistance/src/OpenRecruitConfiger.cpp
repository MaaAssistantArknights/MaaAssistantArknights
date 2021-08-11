#include "OpenRecruitConfiger.h"

#include <fstream>
#include <sstream>
#include <algorithm>

#include <json.h>
#include "Logger.hpp"

using namespace asst;

bool OpenRecruitConfiger::load(const std::string& filename)
{
	DebugTraceFunction;
	DebugTrace("OpenRecruitConfiger::load | ", filename);

	OpenRecruitConfiger temp;
	if (temp._load(filename)) {
		// 按干员等级排个序
		std::sort(temp.m_all_opers.begin(), temp.m_all_opers.end(), [](
			const auto& lhs,
			const auto& rhs)
			-> bool {
				return lhs.level > rhs.level;
			});
		*this = std::move(temp);
		return true;
	}
	else {
		return false;
	}
}

bool asst::OpenRecruitConfiger::_load(const std::string& filename)
{
	std::ifstream ifs(filename, std::ios::in);
	if (!ifs.is_open()) {
		return false;
	}
	std::stringstream iss;
	iss << ifs.rdbuf();
	ifs.close();
	std::string content(iss.str());

	auto&& ret = json::parser::parse(content);
	if (!ret) {
		DebugTrace("parse error", content);
		return false;
	}

	json::value root = ret.value();
	try {
		for (json::value& oper : root.as_array()) {
			OperInfo oper_temp;
			oper_temp.name = oper["name"].as_string();
			oper_temp.type = oper["type"].as_string();
			m_all_types.emplace(oper_temp.type);
			// 职业类型也作为tag之一，加上"干员"两个字
			std::string type_as_tag = oper_temp.type + GbkToUtf8("干员");
			oper_temp.tags.emplace(type_as_tag);
			m_all_tags.emplace(std::move(type_as_tag));

			oper_temp.level = oper["level"].as_integer();
			oper_temp.sex = oper.get("sex", "unknown");
			for (const json::value& tag_value : oper["tags"].as_array()) {
				std::string tag = tag_value.as_string();
				oper_temp.tags.emplace(tag);
				m_all_tags.emplace(std::move(tag));
			}
			oper_temp.hidden = oper.get("hidden", false);
			oper_temp.name_en = oper.get("name-en", "unknown");
			m_all_opers.emplace_back(std::move(oper_temp));
		}
	}
	catch (json::exception& e) {
		DebugTraceError("Load config json error!", e.what());
		return false;
	}

	//#ifdef LOG_TRACE
	//	for (auto&& tag : m_all_tags) {
	//		std::cout << Utf8ToGbk(tag) << std::endl;
	//	}
	//#endif

	DebugTrace("Load config succeed");

	return true;
}