#include "InfrastConfiger.h"

#include <fstream>
#include <sstream>

#include <json.h>
#include "Logger.hpp"

using namespace asst;

bool InfrastConfiger::load(const std::string& filename)
{
	DebugTraceFunction;
	DebugTrace("Configer::load | ", filename);

	InfrastConfiger temp;
	if (temp._load(filename)) {
		*this = std::move(temp);
		return true;
	}
	else {
		return false;
	}
}

bool InfrastConfiger::_load(const std::string& filename)
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
		// 制造站
		for (json::value& comb_info : root["Manufacturing"]["combs"].as_array())
		{
			std::vector<std::string> comb_vec;
			for (json::value& oper_name : comb_info["comb"].as_array())
			{
				std::string oper_name_str = oper_name.as_string();
				m_mfg_opers.emplace(oper_name_str);
				comb_vec.emplace_back(std::move(oper_name_str));
			}
			m_mfg_combs.emplace_back(std::move(comb_vec));
		}
		// 贸易站 TODO……
	}
	catch (json::exception& e) {
		DebugTraceError("Load config json error!", e.what());
		return false;
	}
	DebugTrace("Load config succeed");

	return true;
}