#include "OperBoxDataConfig.h"

#include <algorithm>
#include <functional>
#include <ranges>
#include <unordered_map>
#include <unordered_set>

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Task/AbstractTask.h"
#include "Utils/BipartiteMatch.hpp"
#include "Utils/Logger.hpp"

void asst::OperBoxDataConfig::clear()
{
    m_data.clear();
}

bool asst::OperBoxDataConfig::parse(const json::value& data)
{
    LogTraceFunction;

    clear();

    auto own_opers = data.get("own_opers", json::array());
    for (const auto& item : own_opers) {
        OperBoxInfo info;
        info.id = item.get("id", std::string());
        if (BattleData.find_oper_by_id(info.id) == nullptr) {
            LogError << __FUNCTION__ << "| OperBox data contains invalid oper id:" << info.id;
            clear();
            return false;
        }
        info.name = item.get("name", std::string());
        info.elite = item.get("elite", 0);
        info.level = item.get("level", 0);
        info.potential = item.get("potential", 0);
        info.rarity = item.get("rarity", 0);
        info.own = item.get("own", false);
        info.main_skill_level = item.get("mainSkillLevel", 0);

        for (const auto& skill : item.get("skills", json::array())) {
            OperBoxInfo::Skill skill_info;
            skill_info.id = skill.get("id", std::string());
            skill_info.level = skill.get("level", 0);
            info.skills.emplace_back(std::move(skill_info));
        }

        static const std::unordered_map<std::string, battle::OperModule> EquipTypeMap {
            { "A", battle::OperModule::Alpha },   { "B", battle::OperModule::Beta },  { "X", battle::OperModule::Chi },
            { "Y", battle::OperModule::Upsilon }, { "D", battle::OperModule::Delta },
        };
        for (const auto& equip : item.get("equips", json::array())) {
            OperBoxInfo::Equip equip_info;
            equip_info.id = equip.get("id", std::string());
            std::string type = equip.get("type", std::string());
            if (auto iter = EquipTypeMap.find(type); iter != EquipTypeMap.end()) {
                equip_info.type = iter->second;
            }
            equip_info.level = equip.get("level", 0);
            info.equips.emplace_back(std::move(equip_info));
        }

        m_data.emplace_back(std::move(info));
    }

    std::sort(m_data.begin(), m_data.end(), OperBoxInfo::SortCmp {});
    return !m_data.empty();
}

bool asst::OperBoxDataConfig::can_match(const battle::copilot::OperUsageGroup& group, const OperBoxInfo& info) const
{
    if (!info.own || info.id.empty()) {
        return false;
    }
    auto it = std::ranges::find_if(group.opers, [&](const battle::OperUsage& op) {
        // !!! 要用干员的 id 而不是 name，干员识别的 name 可能不是中文
        if (BattleData.get_first_id(op.role, op.name) != info.id) {
            return false;
        }
        if (m_ignore_requirements) {
            if (op.requirements.elite <= 0 && op.requirements.level <= 0) {
                return true;
            }
            return info.elite >= op.requirements.elite;
        }
        if (!(info.elite > op.requirements.elite ||
              (info.elite == op.requirements.elite && info.level >= op.requirements.level))) {
            return false;
        }
        if (op.skill > 0) {
            if (info.skills.size() < op.skill) {
                return false;
            }
            int info_skill_level = info.main_skill_level + info.skills[op.skill - 1].level;
            if (info_skill_level < op.requirements.skill_level) {
                return false;
            }
        }
        if (op.requirements.module > 0) {
            auto module_it = std::ranges::find_if(info.equips, [&](const OperBoxInfo::Equip& equip) {
                return equip.type == static_cast<battle::OperModule>(op.requirements.module);
            });
            if (module_it == info.equips.end() || module_it->level == 0) {
                return false;
            }
        }
        return true;
    });
    return it != group.opers.end();
}

std::optional<asst::battle::copilot::OperUsageGroups>
    asst::OperBoxDataConfig::precheck(const battle::copilot::OperUsageGroups& formation, bool use_support_unit)
{
    LogTraceFunction;
    using asst::battle::copilot::OperUsageGroup;
    using asst::battle::copilot::OperUsageGroups;

    const std::vector<OperBoxInfo>& operbox_data = m_data;
    OperUsageGroups groups = formation;
    if (groups.empty()) {
        return groups;
    }

    auto matcher = std::bind_front(&OperBoxDataConfig::can_match, this);

    // 使用二分图最大权匹配算法，尝试将干员组与可用干员进行匹配
    auto result = algorithm::bipartite::bipartite_max_match<OperUsageGroup, OperBoxInfo>(groups, operbox_data, matcher);

    LogInfo << __FUNCTION__ << "| matched" << result.matched.size() << "groups, unmatched"
            << result.unmatched_left.size() << "groups";

    // 匹配的干员组
    std::unordered_map<std::string, std::string> assigned;
    {
        json::array matched_groups;
        for (const auto& [left, right] : result.matched) {
            assigned[groups[left].name] = operbox_data[right].id;
            std::string oper_name = BattleData.find_oper_by_id(operbox_data[right].id)->name;
            auto req_it = std::ranges::find_if(groups[left].opers, [&](const battle::OperUsage& op) {
                return BattleData.get_first_id(op.role, op.name) == operbox_data[right].id;
            });
            LogInfo << __FUNCTION__ << "| Matched group:" << groups[left].name << "with oper:" << oper_name
                    << ". Usage elite:" << req_it->requirements.elite << ", level:" << req_it->requirements.level
                    << ", skill:" << req_it->skill << ". Operbox elite:" << operbox_data[right].elite
                    << ", level:" << operbox_data[right].level;
            matched_groups.emplace_back(
                std::unordered_map<std::string, std::string> { { "group_name", groups[left].name },
                                                               { "oper_name", oper_name } });
        }
        if (!matched_groups.empty()) {
            json::value info = m_task_ptr->basic_info_with_what("BattleFormationOperboxMatched");
            info["details"]["matched_groups"] = std::move(matched_groups);
            m_task_ptr->callback(AsstMsg::SubTaskExtraInfo, info);
        }
    }

    // 没有未匹配的干员组
    if (result.unmatched_left.empty()) {
        for (const auto& [left, right] : result.matched) {
            auto req_it = std::ranges::find_if(groups[left].opers, [&](const battle::OperUsage& op) {
                return BattleData.get_first_id(op.role, op.name) == operbox_data[right].id;
            });
            groups[left].opers = { *req_it }; // 只保留匹配的干员
        }
        return groups;
    }

    // 只有一个未匹配的干员组
    if (result.unmatched_left.size() == 1) {
        std::string unmatched_group_name = groups[result.unmatched_left[0]].name;
        if (!use_support_unit) {
            json::value info = m_task_ptr->basic_info_with_what("BattleFormationOperbox1Unmatched");
            info["details"]["group_name"] = unmatched_group_name;
            m_task_ptr->callback(AsstMsg::SubTaskExtraInfo, info);
            return std::nullopt;
        }

        // 枚举作业中所有干员，尝试借助战
        // 不能改图结构，因为可能你有一个精1的干员，但作业1个组要求精1的干员，另1个要求精2的同名干员，借助战的干员可能是精2的，网络流做不了
        // 不知道这么写效率够不够，应该是常数很小的O(n^4)，可能跟O(n^3)的差不多
        std::unordered_set<std::string> candidate_ids;
        for (const auto& group : groups) {
            for (const auto& op : group.opers) {
                auto id = BattleData.get_first_id(op.role, op.name);
                if (id.has_value()) {
                    candidate_ids.insert(*id);
                }
            }
        }

        auto try_borrow = [&](const std::string& borrow_id) -> bool {
            auto cur_data = operbox_data;
            std::erase_if(cur_data, [&](const OperBoxInfo& o) { return o.id == borrow_id; });
            OperBoxInfo fake_oper {};
            fake_oper.id = borrow_id;
            auto oper_ptr = BattleData.find_oper_by_id(borrow_id);
            fake_oper.name = oper_ptr->name;
            fake_oper.rarity = oper_ptr->rarity;
            fake_oper.elite = (fake_oper.rarity >= 3) + (fake_oper.rarity >= 4); // magic: 满练
            fake_oper.level = 30 + (fake_oper.elite * 25) + (fake_oper.rarity > 3) * 10 * (fake_oper.rarity - 5);
            fake_oper.potential = 6;
            fake_oper.own = true;
            auto insert_pos = std::ranges::lower_bound(cur_data, fake_oper, OperBoxInfo::SortCmp {}) - cur_data.begin();
            cur_data.insert(cur_data.begin() + insert_pos, std::move(fake_oper));

            auto retry =
                algorithm::bipartite::bipartite_max_match<OperUsageGroup, OperBoxInfo>(groups, cur_data, matcher);

            if (!retry.unmatched_left.empty()) {
                return false;
            }
            std::unordered_map<std::string, std::string> new_assigned;
            for (const auto& [left, right] : retry.matched) {
                if (cur_data[right].id == borrow_id) {
                    LogInfo << __FUNCTION__ << "| borrow" << BattleData.find_oper_by_id(borrow_id)->name << "for"
                            << groups[left].name;
                    unmatched_group_name = groups[left].name;
                    for (auto& oper : groups[left].opers) {
                        oper.status = battle::OperStatus::Unavailable;
                    }
                }
                else {
                    new_assigned[groups[left].name] = cur_data[right].id;
                    auto req_it = std::ranges::find_if(groups[left].opers, [&](const battle::OperUsage& op) {
                        return BattleData.get_first_id(op.role, op.name) == cur_data[right].id;
                    });
                    groups[left].opers = { *req_it }; // 只保留匹配的干员
                }
            }
            if (assigned != new_assigned) {
                LogInfo << __FUNCTION__ << "| assigned groups changed after borrow, update:";
                json::array assigned_groups;
                for (const auto& group : groups) {
                    if (new_assigned.find(group.name) == new_assigned.end()) {
                        continue;
                    }
                    const auto& oper_id = new_assigned[group.name];
                    auto oper_it =
                        std::ranges::find_if(operbox_data, [&](const OperBoxInfo& op) { return op.id == oper_id; });
                    auto req_it = std::ranges::find_if(group.opers, [&](const battle::OperUsage& op) {
                        return BattleData.get_first_id(op.role, op.name) == oper_id;
                    });
                    LogInfo << __FUNCTION__ << "| Matched group:" << group.name << "with oper:" << oper_it->name
                            << ". Usage elite:" << req_it->requirements.elite
                            << ", level:" << req_it->requirements.level << ", skill:" << req_it->skill
                            << ". Operbox elite:" << oper_it->elite << ", level:" << oper_it->level;
                    assigned_groups.emplace_back(
                        std::unordered_map<std::string, std::string> { { "group_name", group.name },
                                                                       { "oper_name", oper_it->name } });
                }
                json::value info = m_task_ptr->basic_info_with_what("BattleFormationOperboxMatched");
                info["details"]["matched_groups"] = std::move(assigned_groups);
                m_task_ptr->callback(AsstMsg::SubTaskExtraInfo, info);
            }
            json::value info = m_task_ptr->basic_info_with_what("BattleFormationOperbox1Unmatched");
            info["details"]["group_name"] = unmatched_group_name;
            info["details"]["may_borrow_oper"] = BattleData.find_oper_by_id(borrow_id)->name;
            m_task_ptr->callback(AsstMsg::SubTaskExtraInfo, info);
            return true;
        };

        auto& unmatched_group = groups[result.unmatched_left[0]];
        for (const auto& op : unmatched_group.opers) {
            if (m_task_ptr->need_exit()) {
                break;
            }
            auto borrow_id = BattleData.get_first_id(op.role, op.name);
            if (borrow_id == std::nullopt || !candidate_ids.erase(*borrow_id)) {
                continue;
            }
            if (try_borrow(*borrow_id)) {
                return groups;
            }
        }
        for (const auto& borrow_id : candidate_ids) {
            if (m_task_ptr->need_exit()) {
                break;
            }
            if (try_borrow(borrow_id)) {
                return groups;
            }
        }
        json::value info = m_task_ptr->basic_info_with_what("BattleFormationOperbox1Unmatched");
        info["details"]["group_name"] = unmatched_group_name;
        m_task_ptr->callback(AsstMsg::SubTaskExtraInfo, info);
        return std::nullopt;
    }

    // 多个未匹配的干员组
    {
        json::array unmatched_groups;
        LogInfo << __FUNCTION__ << "|" << result.unmatched_left.size() << "slots unmatched, aborting formation";
        for (size_t idx : result.unmatched_left) {
            LogInfo << __FUNCTION__ << "| Unmatched slot:" << groups[idx].name;
            unmatched_groups.emplace_back(groups[idx].name);
        }
        json::value info = m_task_ptr->basic_info_with_what("OperboxMultipleUnmatched");
        info["details"]["unmatched_groups"] = std::move(unmatched_groups);
        m_task_ptr->callback(AsstMsg::SubTaskError, info);
    }
    return std::nullopt;
}
