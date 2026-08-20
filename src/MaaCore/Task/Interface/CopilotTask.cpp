#include "CopilotTask.h"

#include "Arknights-Tile-Pos/TileCalc2.hpp"

#include "Common/AsstBattleDef.h"
#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/CopilotConfig.h"
#include "Config/TaskData.h"
#include "Task/Fight/MedicineCounterTaskPlugin.h"
#include "Task/Miscellaneous/BattleFormationTask.h"
#include "Task/Miscellaneous/BattleProcessTask.h"
#include "Task/Miscellaneous/MultiCopilotTaskPlugin.h"
#include "Task/ProcessTask.h"
#include "Utils/BipartiteMatch.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"

asst::CopilotTask::CopilotTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_multi_copilot_plugin_ptr(std::make_shared<MultiCopilotTaskPlugin>(callback, inst, TaskType)),
    m_formation_task_ptr(std::make_shared<BattleFormationTask>(callback, inst, TaskType)),
    m_battle_task_ptr(std::make_shared<BattleProcessTask>(callback, inst, TaskType)),
    m_stop_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType))
{
    LogTraceFunction;

    m_multi_copilot_plugin_ptr->set_retry_times(0);
    m_multi_copilot_plugin_ptr->set_formation_task_ptr(m_formation_task_ptr);
    m_multi_copilot_plugin_ptr->set_battle_task_ptr(m_battle_task_ptr);
    m_subtasks.emplace_back(m_multi_copilot_plugin_ptr);

    auto start_1_tp = std::make_shared<ProcessTask>(callback, inst, TaskType);
    start_1_tp->set_tasks({ "BattleStartPre" }).set_retry_times(3).set_ignore_error(true);
    m_subtasks.emplace_back(start_1_tp);

    m_medicine_task_ptr = std::make_shared<ProcessTask>(callback, inst, TaskType);
    m_medicine_task_ptr->set_tasks({ "BattleStartPre@UseMedicine", "BattleStartPre@BattleQuickFormation" })
        .set_ignore_error(true);
    m_medicine_task_ptr->register_plugin<MedicineCounterTaskPlugin>()->set_count(999999);
    m_subtasks.emplace_back(m_medicine_task_ptr);

    m_subtasks.emplace_back(m_formation_task_ptr)->set_retry_times(0);

    auto start_2_tp = std::make_shared<ProcessTask>(callback, inst, TaskType);
    start_2_tp->set_tasks({ "BattleStartAll" }).set_retry_times(3).set_ignore_error(false);
    m_subtasks.emplace_back(start_2_tp);

    // 跳过“以下干员出战后将被禁用，是否继续？”对话框
    auto start_3_tp = std::make_shared<ProcessTask>(callback, inst, TaskType);
    start_3_tp->set_tasks({ "SkipForbiddenOperConfirm", "Stop" }).set_ignore_error(false);
    m_subtasks.emplace_back(start_3_tp);

    m_subtasks.emplace_back(m_battle_task_ptr)->set_retry_times(0);

    m_stop_task_ptr->set_enable(false);
    m_subtasks.emplace_back(m_stop_task_ptr);
}

bool asst::CopilotTask::set_params(const json::value& params)
{
    LogTraceFunction;

    using SupportUnitUsage = BattleFormationTask::SupportUnitUsage;

    if (m_has_subtasks_duplicate) {
        Log.error(__FUNCTION__, "CopilotTask set_params failed, already set params");
        return false;
    }

    bool use_sanity_potion = params.get("use_sanity_potion", false);                 // 是否使用理智药
    bool with_formation = params.get("formation", false);                            // 是否使用自动编队
    int formation_index = params.get("formation_index", 0);                          // 选择第几个编队，0为不选择
    bool add_trust = params.get("add_trust", false);                                 // 是否自动补信赖
    bool ignore_requirements = params.get("ignore_requirements", false);             // 跳过未满足的干员属性要求
    bool add_user_additional = params.contains("user_additional");                   // 是否自动补用户自定义干员
    auto support_unit_usage = static_cast<SupportUnitUsage>(
        params.get("support_unit_usage", static_cast<int>(SupportUnitUsage::None))); // 助战干员使用模式
    std::string support_unit_name = params.get("support_unit_name", std::string());
    std::string operbox_data_path = params.get("operbox_data_path", std::string());  // 干员辅助编队数据路径, 为空则禁用
    std::optional<std::vector<OperBoxInfo>> operbox_data = std::nullopt;

    if (!operbox_data_path.empty()) {
        operbox_data = parse_operbox_data(operbox_data_path);
        if (operbox_data->empty()) {
            LogError << __FUNCTION__ << "| OperBox data is empty, cannot perform precheck";
            json::value info = basic_info_with_what("OperboxDataParseFailed");
            callback(AsstMsg::SubTaskError, info);
            return false;
        }
        std::sort(operbox_data->begin(), operbox_data->end(), OperBoxInfo::SortCmp {});
    }

    auto filename_opt = params.find<std::string>("filename");
    auto multi_tasks_opt = params.find<json::array>("copilot_list"); // 多任务列表
    if (!filename_opt && !multi_tasks_opt) {
        Log.error("CopilotTask set_params failed, stage_name or filename not found");
        return false;
    }

    if (filename_opt) {
        m_multi_copilot_plugin_ptr->set_enable(false);
        m_battle_task_ptr->set_wait_until_end(false);
        auto copilot_opt = parse_copilot_filename(*filename_opt);
        if (!copilot_opt) {
            return false;
        }
        if (operbox_data.has_value()) {
            const auto& assignment_opt = operbox_precheck(
                *operbox_data,
                Copilot.get_data().groups,
                support_unit_usage != SupportUnitUsage::None);
            if (!assignment_opt) {
                return false;
            }
            m_formation_task_ptr->set_assigned_groups(std::move(assignment_opt));
        }
        else {
            m_formation_task_ptr->set_assigned_groups(std::nullopt);
        }
        m_stage_name = Copilot.get_stage_name();
        if (!m_battle_task_ptr->set_stage_name(m_stage_name)) {
            Log.error("Not support stage");
            return false;
        }
    }
    else if (multi_tasks_opt) {
        m_multi_copilot_plugin_ptr->set_enable(true); // 启用多任务插件, 自动覆盖Copilot中的配置
        m_battle_task_ptr->set_wait_until_end(true);
        auto configs = static_cast<std::vector<MultiCopilotConfig>>(*multi_tasks_opt);
        std::vector<MultiCopilotTaskPlugin::MultiCopilotConfig> configs_cvt;
        std::unordered_map<std::string, std::shared_ptr<battle::copilot::OperUsageGroups>> operbox_assignments;
        for (const auto& [id, filename, nav_name, is_raid] : configs) {
            MultiCopilotTaskPlugin::MultiCopilotConfig config_cvt;
            auto copilot_opt = parse_copilot_filename(filename);
            if (!copilot_opt) {
                return false;
            }
            config_cvt.assigned_groups = nullptr;
            if (operbox_data.has_value()) {
                if (operbox_assignments.find(filename) != operbox_assignments.end()) {
                    config_cvt.assigned_groups = operbox_assignments[filename];
                }
                else {
                    const auto& assignment_opt = operbox_precheck(
                        *operbox_data,
                        Copilot.get_data().groups,
                        support_unit_usage != SupportUnitUsage::None);
                    if (!assignment_opt) {
                        return false;
                    }
                    config_cvt.assigned_groups = std::make_shared<OperUsageGroups>(std::move(assignment_opt.value()));
                    operbox_assignments[filename] = config_cvt.assigned_groups;
                }
            }
            const auto& stage_name = Copilot.get_stage_name();
            const auto& map_data = Tile.find(stage_name);
            if (!map_data || !json::open(map_data->second)) {
                return false;
            }
            if (!nav_name) {
                config_cvt.nav_name = map_data->first.code;
            }
            else {
                LogInfo << __FUNCTION__ << " | navigation name override: " << *nav_name;
                config_cvt.nav_name = *nav_name;
            }
            config_cvt.copilot_file = *copilot_opt;
            config_cvt.is_raid = is_raid;
            config_cvt.id = id; // ID 从0开始
            configs_cvt.emplace_back(std::move(config_cvt));
        }

        size_t count = configs_cvt.size();
        // 追加任务
        m_subtasks.reserve(m_subtasks.size() * count);
        // 保存原始大小
        size_t original_size = m_subtasks.size();
        for (size_t i = 1; i < count; ++i) {
            m_subtasks.insert(m_subtasks.end(), m_subtasks.begin(), m_subtasks.begin() + original_size);
        }
        m_multi_copilot_plugin_ptr->set_multi_copilot_config(std::move(configs_cvt));
        m_has_subtasks_duplicate = true;

        for (const auto& obj : *multi_tasks_opt) {
            if (obj.contains("is_paradox")) {
                Log.error("================  !DEPRECATED!  ================");
                LogError << "`is_paradox` has been deprecated since v6.1.2;";
                LogError << "Please use 'ParadoxCopilotTask' for paradox copilot;";
                Log.error("================  !DEPRECATED!  ================");
                return false;
            }
        }
    }

    m_medicine_task_ptr->set_enable(use_sanity_potion);

    m_formation_task_ptr->set_enable(with_formation);
    m_formation_task_ptr->set_select_formation(formation_index);
    m_formation_task_ptr->set_add_trust(add_trust);
    m_formation_task_ptr->set_ignore_requirements(ignore_requirements);
    m_formation_task_ptr->set_support_unit_usage(support_unit_usage);
    m_formation_task_ptr->set_specific_support_unit(support_unit_name);

    if (auto opt = params.find<json::array>("user_additional"); with_formation && add_user_additional && opt) {
        std::vector<std::pair<std::string, int>> user_additional;
        for (const auto& op : *opt) {
            std::string name = op.get("name", std::string());
            if (name.empty()) {
                continue;
            }
            if (BattleData.is_name_invalid(name)) {
                Log.error(__FUNCTION__, "| User additional oper", name, "is invalid");
                json::value info = basic_info_with_what("UserAdditionalOperInvalid");
                info["details"]["name"] = name;
                callback(AsstMsg::SubTaskError, info);
                return false;
            }
            user_additional.emplace_back(std::pair<std::string, int> { std::move(name), op.get("skill", 0) });
        }
        m_formation_task_ptr->set_user_additional(std::move(user_additional));
    }

    m_battle_task_ptr->set_formation_task_ptr(m_formation_task_ptr->get_opers_in_formation());

    size_t loop_times = params.get("loop_times", 1);
    if (m_multi_copilot_plugin_ptr->get_enable()) {
        // 如果没三星就中止
        // 悖论模拟不需要强制三星，因为练度等关系有概率不过，反正不消耗理智，走单独的退出逻辑
        // EDIT: UI 上取消勾选需要按顺序，非三星通关会导致取消的内容错误
        /* if (m_paradox_task_ptr->get_enable()) {
             m_stop_task_ptr->set_tasks({ "ClickCornerUntilReturnButton" });
         }
         else {
             m_stop_task_ptr->set_tasks({ "Copilot@WaitUntilEndOfAction" });
         }*/
        m_stop_task_ptr->set_tasks({ "Copilot@WaitUntilEndOfAction" }); // 带三星检查
        m_stop_task_ptr->set_enable(true);
    }
    else if (loop_times > 1) {
        m_stop_task_ptr->set_tasks({ "ClickCornerUntilStartButton" });
        m_stop_task_ptr->set_enable(true);

        // 追加
        m_subtasks.reserve(m_subtasks.size() * loop_times);
        size_t original_size = m_subtasks.size();
        for (size_t i = 1; i < loop_times; ++i) {
            m_subtasks.insert(m_subtasks.end(), m_subtasks.begin(), m_subtasks.begin() + original_size);
        }
        m_has_subtasks_duplicate = true;
    }
    return true;
}

std::optional<std::filesystem::path> asst::CopilotTask::parse_copilot_filename(const std::string& name)
{
    auto path = utils::path(name);
    if (!Copilot.load(path)) {
        Log.error("CopilotConfig parse failed");
        return std::nullopt;
    }
    return path;
}

std::vector<asst::OperBoxInfo> asst::CopilotTask::parse_operbox_data(const std::string& path)
{
    LogTraceFunction;

    std::vector<OperBoxInfo> result;
    auto json_opt = json::open(utils::path(path), true, true);
    if (!json_opt) {
        LogError << __FUNCTION__ << "| Failed to open OperBox data file:" << path;
        return result;
    }

    auto& json_obj = json_opt.value();
    auto own_opers = json_obj.get("own_opers", json::array());

    for (auto& item : own_opers) {
        OperBoxInfo info;
        info.id = item.get("id", std::string());
        if (BattleData.find_oper_by_id(info.id) == nullptr) {
            LogError << __FUNCTION__ << "| OperBox data contains invalid oper id:" << info.id;
            result.clear();
            break;
        }
        info.name = item.get("name", std::string());
        info.elite = item.get("elite", 0);
        info.level = item.get("level", 0);
        info.potential = item.get("potential", 0);
        info.rarity = item.get("rarity", 0);
        info.own = item.get("own", false);
        result.emplace_back(std::move(info));
    }

    return result;
}

using asst::battle::copilot::OperUsageGroups;

std::optional<OperUsageGroups> asst::CopilotTask::operbox_precheck(
    const std::vector<OperBoxInfo>& operbox_data,
    const OperUsageGroups& formation,
    bool use_support_unit)
{
    LogTraceFunction;
    using asst::battle::copilot::OperUsageGroup;
    OperUsageGroups groups = formation;
    if (groups.empty()) {
        return groups;
    }

    auto can_match = [](const OperUsageGroup& group, const OperBoxInfo& info) { // 目前只考虑忽略干员练度情况
        if (!info.own || info.id.empty()) {
            return false;
        }
        auto it = std::ranges::find_if(group.opers, [&](const battle::OperUsage& op) {
            // !!! 要用干员的 id 而不是 name，干员识别的 name 可能不是中文
            if (BattleData.get_id(op.role, op.name) != info.id) {
                return false;
            }
            if (op.requirements.elite <= 0 && op.requirements.level <= 0) {
                return true;
            }
            return info.elite >= op.requirements.elite;
        });
        return it != group.opers.end();
    };

    // 使用二分图最大权匹配算法，尝试将干员组与可用干员进行匹配
    auto result =
        algorithm::bipartite::bipartite_max_match<OperUsageGroup, OperBoxInfo>(groups, operbox_data, can_match);

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
                return BattleData.get_id(op.role, op.name) == operbox_data[right].id;
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
            json::value info = basic_info_with_what("BattleFormationOperboxMatched");
            info["details"]["matched_groups"] = std::move(matched_groups);
            callback(AsstMsg::SubTaskExtraInfo, info);
        }
    }

    // 没有未匹配的干员组
    if (result.unmatched_left.empty()) {
        for (const auto& [left, right] : result.matched) {
            auto req_it = std::ranges::find_if(groups[left].opers, [&](const battle::OperUsage& op) {
                return BattleData.get_id(op.role, op.name) == operbox_data[right].id;
            });
            groups[left].opers = { *req_it }; // 只保留匹配的干员
        }
        return groups;
    }

    // 只有一个未匹配的干员组
    if (result.unmatched_left.size() == 1) {
        std::string unmatched_group_name = groups[result.unmatched_left[0]].name;
        if (!use_support_unit) {
            json::value info = basic_info_with_what("BattleFormationOperbox1Unmatched");
            info["details"]["group_name"] = unmatched_group_name;
            callback(AsstMsg::SubTaskExtraInfo, info);
            return std::nullopt;
        }

        // 枚举作业中所有干员，尝试借助战
        // 不能改图结构，因为可能你有一个精1的干员，但作业1个组要求精1的干员，另1个要求精2的同名干员，借助战的干员可能是精2的，网络流做不了
        // 不知道这么写效率够不够，应该是常数很小的O(n^4)，可能跟O(n^3)的差不多
        std::unordered_set<std::string> candidate_ids;
        for (const auto& group : groups) {
            for (const auto& op : group.opers) {
                auto id = BattleData.get_id(op.role, op.name);
                if (!id.empty()) {
                    candidate_ids.insert(id);
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
                algorithm::bipartite::bipartite_max_match<OperUsageGroup, OperBoxInfo>(groups, cur_data, can_match);

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
                        return BattleData.get_id(op.role, op.name) == cur_data[right].id;
                    });
                    groups[left].opers = { *req_it }; // 只保留匹配的干员
                }
            }
            if (assigned != new_assigned) {
                LogInfo << __FUNCTION__ << "| assigned groups changed after borrow, update:";
                json::value info = basic_info_with_what("BattleFormationOperboxMatched");
                json::array assigned_groups;
                for (const auto& group : groups) {
                    if (new_assigned.find(group.name) == new_assigned.end()) {
                        continue;
                    }
                    const auto& oper_id = new_assigned[group.name];
                    auto oper_it =
                        std::ranges::find_if(operbox_data, [&](const OperBoxInfo& op) { return op.id == oper_id; });
                    auto req_it = std::ranges::find_if(group.opers, [&](const battle::OperUsage& op) {
                        return BattleData.get_id(op.role, op.name) == oper_id;
                    });
                    LogInfo << __FUNCTION__ << "| Matched group:" << group.name << "with oper:" << oper_it->name
                            << ". Usage elite:" << req_it->requirements.elite
                            << ", level:" << req_it->requirements.level << ", skill:" << req_it->skill
                            << ". Operbox elite:" << oper_it->elite << ", level:" << oper_it->level;
                    assigned_groups.emplace_back(
                        std::unordered_map<std::string, std::string> { { "group_name", group.name },
                                                                       { "oper_name", oper_it->name } });
                }
                info["details"]["matched_groups"] = std::move(assigned_groups);
                callback(AsstMsg::SubTaskExtraInfo, info);
            }
            json::value info = basic_info_with_what("BattleFormationOperbox1Unmatched");
            info["details"]["group_name"] = unmatched_group_name;
            info["details"]["may_borrow_oper"] = BattleData.find_oper_by_id(borrow_id)->name;
            callback(AsstMsg::SubTaskExtraInfo, info);
            return true;
        };

        auto& unmatched_group = groups[result.unmatched_left[0]];
        for (const auto& op : unmatched_group.opers) {
            if (need_exit()) {
                break;
            }
            auto borrow_id = BattleData.get_id(op.role, op.name);
            if (borrow_id.empty() || !candidate_ids.erase(borrow_id)) {
                continue;
            }
            if (try_borrow(borrow_id)) {
                return groups;
            }
        }
        for (const auto& borrow_id : candidate_ids) {
            if (need_exit()) {
                break;
            }
            if (try_borrow(borrow_id)) {
                return groups;
            }
        }
        json::value info = basic_info_with_what("BattleFormationOperbox1Unmatched");
        info["details"]["group_name"] = unmatched_group_name;
        callback(AsstMsg::SubTaskExtraInfo, info);
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
        json::value info = basic_info_with_what("OperboxMultipleUnmatched");
        info["details"]["unmatched_groups"] = std::move(unmatched_groups);
        callback(AsstMsg::SubTaskError, info);
    }
    return std::nullopt;
}
