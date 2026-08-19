#include "CopilotTask.h"

#include "Arknights-Tile-Pos/TileCalc2.hpp"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/CopilotConfig.h"
#include "Config/TaskData.h"
#include "Task/Fight/MedicineCounterTaskPlugin.h"
#include "Task/Miscellaneous/BattleFormationTask.h"
#include "Task/Miscellaneous/BattleProcessTask.h"
#include "Task/Miscellaneous/MultiCopilotTaskPlugin.h"
#include "Task/ProcessTask.h"
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
        for (const auto& [id, filename, nav_name, is_raid] : configs) {
            MultiCopilotTaskPlugin::MultiCopilotConfig config_cvt;
            auto copilot_opt = parse_copilot_filename(filename);
            if (!copilot_opt) {
                return false;
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
    if (operbox_data_path.empty()) {
        m_formation_task_ptr->set_operbox_data(std::nullopt);
    }
    else {
        std::vector<OperBoxInfo> operbox_data;
        operbox_data = parse_operbox_data(operbox_data_path);
        if (operbox_data.empty()) {
            LogError << __FUNCTION__ << "| OperBox data is empty, cannot perform precheck";
            json::value info = basic_info_with_what("OperboxDataParseFailed");
            callback(AsstMsg::SubTaskError, info);
            return false;
        }
        std::sort(operbox_data.begin(), operbox_data.end(), OperBoxInfo::SortCmp {});
        m_formation_task_ptr->set_operbox_data(std::move(operbox_data));
    }

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
