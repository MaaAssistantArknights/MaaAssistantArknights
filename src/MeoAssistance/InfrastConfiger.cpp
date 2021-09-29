#include "InfrastConfiger.h"

#include <fstream>
#include <sstream>

#include <json.h>

#include "Logger.hpp"

using namespace asst;

bool InfrastConfiger::parse(const json::value& json)
{
    // 通用的干员信息
    for (const json::value& name : json.at("allNames").as_array()) {
        m_all_opers_name.emplace(name.as_string());
    }
    for (const json::value& pair : json.at("featureKey").as_array()) {
        m_oper_name_feat.emplace(pair[0].as_string(), pair[1].as_string());
    }
    for (const json::value& name : json.at("featureWhatever").as_array()) {
        m_oper_name_feat_whatever.emplace(name.as_string());
    }

    // 每个基建设施中的干员组合信息
    for (const json::value& facility : json.at("infrast").as_array()) {
        std::string key = facility.at("facility").as_string();

        std::vector<OperInfrastComb> combs_vec;
        for (const json::value& comb_json : facility.at("combs").as_array()) {
            std::vector<OperInfrastInfo> opers;
            for (const json::value& oper : comb_json.at("opers").as_array()) {
                OperInfrastInfo info;
                info.name = oper.at("name").as_string();
                info.elite = oper.get("elite", 0);
                info.level = oper.get("level", 0);
                opers.emplace_back(std::move(info));
            }
            OperInfrastComb comb;
            comb.comb = std::move(opers);
            comb.efficiency = comb_json.at("efficiency").as_integer();

            combs_vec.emplace_back(std::move(comb));
        }
        m_infrast_combs.emplace(key, std::move(combs_vec));

        std::vector<std::string> end_name_vec;
        for (const json::value& name : facility.at("endFlag").as_array()) {
            end_name_vec.emplace_back(name.as_string());
        }
        m_infrast_end_flag.emplace(key, std::move(end_name_vec));
    }

    return true;
}