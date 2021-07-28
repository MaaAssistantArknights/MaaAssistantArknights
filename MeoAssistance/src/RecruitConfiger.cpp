#include "RecruitConfiger.h"

#include <fstream>
#include <sstream>

#include "json.h"
#include "Logger.hpp"

using namespace asst;

bool RecruitConfiger::load(const std::string& filename)
{
	DebugTraceFunction;
	DebugTrace("RecruitConfiger::load | ", filename);

	std::ifstream ifs(filename, std::ios::in);
	if (!ifs.is_open()) {
		return false;
	}
	std::stringstream iss;
	iss << ifs.rdbuf();
	ifs.close();
	std::string content(iss.str());

	auto ret = json::parser::parse(content);
	if (!ret) {
		DebugTrace("parse error", content);
		return false;
	}

	auto root = std::move(ret).value();

	RecruitConfiger temp;
	try {
		auto opers_array = root.as_array();
		for (auto&& oper : opers_array) {
			OperInfo oper_temp;
			oper_temp.name = oper["name"].as_string();
			oper_temp.type = oper["type"].as_string();
			temp.m_all_types.emplace(oper_temp.type);
			// 职业类型也作为tag之一，加上"干员"两个字
			std::string type_as_tag = oper_temp.type + GbkToUtf8("干员");
			oper_temp.tags.emplace(type_as_tag);
			temp.m_all_tags.emplace(type_as_tag);

			oper_temp.level = oper["level"].as_integer();
			oper_temp.sex = oper["sex"].as_string();
			auto tags_arr = oper["tags"].as_array();
			for (const auto& tag_value : tags_arr) {
				std::string tag = tag_value.as_string();
				oper_temp.tags.emplace(tag);
				temp.m_all_tags.emplace(tag);
			}
			oper_temp.hidden = oper["hidden"].as_boolean();
			oper_temp.name_en = oper["name-en"].as_string();
			temp.m_opers.emplace_back(std::move(oper_temp));
		}
	}
	catch (json::exception& e) {
		DebugTraceError("Load config json error!", e.what());
		return false;
	}

	*this = std::move(temp);

#ifdef LOG_TRACE
	for (auto&& tag : m_all_tags) {
		std::cout << Utf8ToGbk(tag) << std::endl;
	}
#endif

	DebugTrace("Load config succeed");

	return true;
}
