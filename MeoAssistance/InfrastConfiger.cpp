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
				comb_vec.emplace_back(std::move(oper_name_str));
			}
			m_mfg_combs.emplace_back(std::move(comb_vec));
		}
		m_mfg_end = root["Manufacturing"]["end"].as_string();
		for (json::value& oper : root["Manufacturing"]["all"].as_array())
		{
			m_mfg_opers.emplace(oper["name"].as_string());
		}
		for (json::value& pair : root["Manufacturing"]["featureKey"].as_array()) 
		{
			m_mfg_feat.emplace(pair[0].as_string(), pair[1].as_string());
		}
		for (json::value& name : root["Manufacturing"]["featureWhatever"].as_array())
		{
			m_mfg_feat_whatever.emplace(name.as_string());
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