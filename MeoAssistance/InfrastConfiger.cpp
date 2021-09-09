#include "InfrastConfiger.h"

#include <fstream>
#include <sstream>

#include <json.h>

#include "Logger.hpp"

using namespace asst;

bool InfrastConfiger::parse(json::value&& json)
{
	json::value root = std::move(json);
	// 通用的干员信息
	for (json::value& name : root["allNames"].as_array())
	{
		m_all_opers_name.emplace(name.as_string());
	}
	for (json::value& pair : root["featureKey"].as_array())
	{
		m_oper_name_feat.emplace(pair[0].as_string(), pair[1].as_string());
	}
	for (json::value& name : root["featureWhatever"].as_array())
	{
		m_oper_name_feat_whatever.emplace(name.as_string());
	}

	// 每个基建设施中的干员组合信息
	for (json::value& facility : root["infrast"].as_array())
	{
		std::string key = facility["facility"].as_string();

		std::vector<OperInfrastComb> combs_vec;
		for (json::value& comb_json : facility["combs"].as_array())
		{
			std::vector<OperInfrastInfo> opers;
			for (json::value& oper : comb_json["opers"].as_array()) {
				OperInfrastInfo info;
				info.name = oper["name"].as_string();
				info.elite = oper.get("elite", 0);
				info.level = oper.get("level", 0);
				opers.emplace_back(std::move(info));
			}
			OperInfrastComb comb;
			comb.comb = std::move(opers);
			comb.efficiency = comb_json["efficiency"].as_integer();

			combs_vec.emplace_back(std::move(comb));
		}
		m_infrast_combs.emplace(key, std::move(combs_vec));

		std::vector<std::string> end_name_vec;
		for (json::value& name : facility["endFlag"].as_array()) {
			end_name_vec.emplace_back(name.as_string());
		}
		m_infrast_end_flag.emplace(key, std::move(end_name_vec));
	}

	return true;
}