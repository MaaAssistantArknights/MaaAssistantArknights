#include "InfrastConfig.h"

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

bool asst::InfrastConfig::parse(const json::value& json)
{
    LogTraceFunction;

    for (const json::value& facility : json.at("roomType").as_array()) {
        std::string facility_name = facility.as_string();
        const json::value& facility_json = json.at(facility_name);

        /* 解析 products 字段 */
        std::vector<std::string> products;
        if (auto products_opt = facility_json.find<json::array>("products")) {
            for (const json::value& product_json : products_opt.value()) {
                products.emplace_back(product_json.as_string());
            }
        }

        /* 解析skills字段 */
        std::unordered_map<std::string, infrast::Skill> facility_skills;
        for (const auto& [id, skill_json] : facility_json.at("skills").as_object()) {
            infrast::Skill skill;
            std::string templ_name = skill_json.at("template").as_string();
            skill.templ_name = templ_name;
            if (!templ_name.starts_with("Bskill_dorm_")) {
                // 宿舍技能跳过，没有图片
                m_templ_required.emplace(skill.templ_name);
            }
            skill.id = id;
            for (const json::value& skill_names : skill_json.at("name").as_array()) {
                skill.names.emplace_back(skill_names.as_string());
            }
            skill.desc = skill_json.at("desc").at(0).as_string();

            /* 解析efficient的数字及正则值 */
            if (auto opt = skill_json.find("efficient")) {
                const json::value& efficient = *opt;
                const static std::string reg_suffix = "_reg";

                if (std::string all_reg_key = "all" + reg_suffix; efficient.contains(all_reg_key)) {
                    std::string all_reg_value = efficient.at(all_reg_key).as_string();
                    for (const std::string& pd : products) {
                        skill.efficient_regex.emplace(pd, all_reg_value);
                        skill.efficient.emplace(pd, 0);
                    }
                }
                else if (efficient.contains("all")) {
                    double all_value = efficient.at("all").as_double();
                    for (const std::string& pd : products) {
                        skill.efficient.emplace(pd, all_value);
                    }
                }
                else {
                    for (const std::string& pd : products) {
                        if (std::string pd_reg_key = pd + reg_suffix; efficient.contains(pd_reg_key)) {
                            skill.efficient_regex.emplace(pd, efficient.at(pd_reg_key).as_string());
                            skill.efficient.emplace(pd, 0);
                        }
                        else if (efficient.contains(pd)) {
                            skill.efficient.emplace(pd, efficient.at(pd).as_double());
                        }
                        else {
                            skill.efficient.emplace(pd, 0);
                        }
                    }
                }
            }
            else {
                for (const std::string& pd : products) {
                    skill.efficient.emplace(pd, 0);
                }
            }
            skill.max_num = skill_json.get("maxNum", INT_MAX);

            facility_skills.emplace(id, std::move(skill));
        }
        m_skills.emplace(facility_name, std::move(facility_skills));

        /* 解析skillsGroup字段 */
        std::vector<infrast::SkillsGroup> group_vec;
        if (auto skills_group_opt = facility_json.find<json::array>("skillsGroup")) {
            for (const json::value& group_json : skills_group_opt.value()) {
                infrast::SkillsGroup group;
                group.desc = group_json.get("desc", std::string());
                if (group_json.contains("conditions")) {
                    for (const auto& [cond, value] : group_json.at("conditions").as_object()) {
                        group.conditions.emplace(cond, value.as_integer());
                    }
                }
                for (const json::value& necessary_json : group_json.at("necessary").as_array()) {
                    infrast::SkillsComb comb;
                    comb.desc = necessary_json.get("desc", std::string());
                    for (const json::value& skill_json : necessary_json.at("skills").as_array()) {
                        const auto& skill = m_skills.at(facility_name).at(skill_json.as_string());
                        comb.skills.emplace(skill);
                    }
                    /* 解析efficient的数字及正则值 */
                    if (necessary_json.contains("efficient")) {
                        const static std::string reg_suffix = "_reg";
                        const json::value& efficient = necessary_json.at("efficient");

                        if (std::string all_reg_key = "all" + reg_suffix; efficient.contains(all_reg_key)) {
                            std::string all_reg_value = efficient.at(all_reg_key).as_string();
                            for (const std::string& pd : products) {
                                comb.efficient_regex.emplace(pd, all_reg_value);
                                comb.efficient.emplace(pd, 0);
                            }
                        }
                        else if (efficient.contains("all")) {
                            double all_value = efficient.at("all").as_double();
                            for (const std::string& pd : products) {
                                comb.efficient.emplace(pd, all_value);
                            }
                        }
                        else {
                            for (const std::string& pd : products) {
                                if (std::string pd_reg_key = pd + reg_suffix; efficient.contains(pd_reg_key)) {
                                    comb.efficient_regex.emplace(pd, efficient.at(pd_reg_key).as_string());
                                    comb.efficient.emplace(pd, 0);
                                }
                                else if (efficient.contains(pd)) {
                                    comb.efficient.emplace(pd, efficient.at(pd).as_double());
                                }
                                else {
                                    comb.efficient.emplace(pd, 0);
                                }
                            }
                        }
                    }
                    else {
                        for (const std::string& pd : products) {
                            comb.efficient.emplace(pd, 0);
                        }
                    }
                    if (auto filter_opt = necessary_json.find<json::array>("filter")) {
                        for (const auto& filter : filter_opt.value()) {
                            comb.name_filter.emplace_back(filter.as_string());
                        }
                    }
                    group.necessary.emplace_back(std::move(comb));
                }
                for (const json::value& opt_json : group_json.at("optional").as_array()) {
                    infrast::SkillsComb comb;
                    comb.desc = opt_json.get("desc", std::string());
                    for (const json::value& skill_json : opt_json.at("skills").as_array()) {
                        const auto& skill = m_skills.at(facility_name).at(skill_json.as_string());
                        comb.skills.emplace(skill);
                    }
                    /* 解析efficient的数字及正则值 */
                    if (opt_json.contains("efficient")) {
                        const static std::string reg_suffix = "_reg";
                        const json::value& efficient = opt_json.at("efficient");

                        if (std::string all_reg_key = "all" + reg_suffix; efficient.contains(all_reg_key)) {
                            std::string all_reg_value = efficient.at(all_reg_key).as_string();
                            for (const std::string& pd : products) {
                                comb.efficient_regex.emplace(pd, all_reg_value);
                                comb.efficient.emplace(pd, 0);
                            }
                        }
                        else if (efficient.contains("all")) {
                            double all_value = efficient.at("all").as_double();
                            for (const std::string& pd : products) {
                                comb.efficient.emplace(pd, all_value);
                            }
                        }
                        else {
                            for (const std::string& pd : products) {
                                if (std::string pd_reg_key = pd + reg_suffix; efficient.contains(pd_reg_key)) {
                                    comb.efficient_regex.emplace(pd, efficient.at(pd_reg_key).as_string());
                                    comb.efficient.emplace(pd, 0);
                                }
                                else if (efficient.contains(pd)) {
                                    comb.efficient.emplace(pd, efficient.at(pd).as_double());
                                }
                                else {
                                    comb.efficient.emplace(pd, 0);
                                }
                            }
                        }
                    }
                    else {
                        for (const std::string& pd : products) {
                            comb.efficient.emplace(pd, 0);
                        }
                    }

                    if (auto filter_opt = opt_json.find<json::array>("filter")) {
                        for (const auto& filter : filter_opt.value()) {
                            comb.name_filter.emplace_back(filter.as_string());
                        }
                    }
                    group.optional.emplace_back(std::move(comb));
                }
                group.allow_external = group_json.get("allowExternal", false);
                group_vec.emplace_back(std::move(group));
            }
        }
        m_skills_groups.emplace(facility_name, std::move(group_vec));

        infrast::Facility fac_info;
        fac_info.id = facility_name;
        fac_info.products = std::move(products);
        fac_info.max_num_of_opers = facility_json.get("maxNumOfOpers", 1);

        m_facilities_info.emplace(facility_name, std::move(fac_info));
    }
    return true;
}
