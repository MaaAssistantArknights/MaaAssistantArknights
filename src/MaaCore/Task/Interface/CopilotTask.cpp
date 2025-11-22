#include "CopilotTask.h"

#include <regex>

#include "Config/Miscellaneous/CopilotConfig.h"
#include "Config/TaskData.h"
#include "Task/Fight/MedicineCounterTaskPlugin.h"
#include "Task/Miscellaneous/BattleFormationTask.h"
#include "Task/Miscellaneous/BattleProcessTask.h"
#include "Task/Miscellaneous/MultiCopilotTaskPlugin.h"
#include "Task/Miscellaneous/ParadoxRecognitionTask.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"

#include "Arknights-Tile-Pos/TileCalc2.hpp"

asst::CopilotTask::CopilotTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType),
    m_multi_copilot_plugin_ptr(std::make_shared<MultiCopilotTaskPlugin>(callback, inst, TaskType)),
    m_paradox_task_ptr(std::make_shared<ParadoxRecognitionTask>(callback, inst, TaskType)),
    m_formation_task_ptr(std::make_shared<BattleFormationTask>(callback, inst, TaskType)),
    m_battle_task_ptr(std::make_shared<BattleProcessTask>(callback, inst, TaskType)),
    m_stop_task_ptr(std::make_shared<ProcessTask>(callback, inst, TaskType))
{
    LogTraceFunction;

    m_multi_copilot_plugin_ptr->set_retry_times(0);
    m_multi_copilot_plugin_ptr->set_battle_task_ptr(m_battle_task_ptr);
    m_multi_copilot_plugin_ptr->set_paradox_task_ptr(m_paradox_task_ptr);
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
    m_subtasks.emplace_back(m_paradox_task_ptr);

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

    bool use_sanity_potion = params.get("use_sanity_potion", false); // 是否吃理智药
    bool with_formation = params.get("formation", false);            // 是否使用自动编队
    int formation_index = params.get("formation_index", 0);          // 选择第几个编队，0为不选择
    if (!params.contains("formation_index") && params.contains("select_formation")) {
        Log.warn("================  DEPRECATED  ================");
        Log.warn("`select_formation` has been deprecated since v5.23.3; Please use 'formation_index'");
        Log.warn("================  DEPRECATED  ================");
        formation_index = params.get("select_formation", 0);
    }
    bool add_trust = params.get("add_trust", false);                                 // 是否自动补信赖
    bool ignore_requirements = params.get("ignore_requirements", false);             // 跳过未满足的干员属性要求
    bool add_user_additional = params.contains("user_additional");                   // 是否自动补用户自定义干员
    auto support_unit_usage = static_cast<SupportUnitUsage>(
        params.get("support_unit_usage", static_cast<int>(SupportUnitUsage::None))); // 助战干员使用模式
    std::string support_unit_name = params.get("support_unit_name", std::string());

    // 是否在当前页面左右滑动寻找关卡，启用战斗列表则为true
    auto nav_opt1 = params.find<bool>("need_navigate");
    auto nav_opt2 = params.find<bool>("navigate_name");
    if (nav_opt1 || nav_opt2) {
        Log.warn("================  DEPRECATED  ================");
        LogWarn << "`need_navigate`, `navigate_name` and `is_raid` has been deprecated since v5.23.3;";
        Log.warn("================  DEPRECATED  ================");
        if (nav_opt1.value_or(false) || nav_opt2.value_or(false)) // 不启用导航时仅警告
        {
            return false;
        }
    }

    auto filename_opt = params.find<std::string>("filename");
    auto multi_tasks_opt = params.find<json::value>("copilot_list"); // 多任务列表
    if (!filename_opt && !multi_tasks_opt) {
        Log.error("CopilotTask set_params failed, stage_name or filename not found");
        return false;
    }

    if (filename_opt) {
        m_multi_copilot_plugin_ptr->set_enable(false);
        m_battle_task_ptr->set_wait_until_end(false);
        auto copilot_opt = parse_copilot_filename(*filename_opt);
        m_stage_name = Copilot.get_stage_name();
        // 单个悖论走正常流程，不用导航
        m_paradox_task_ptr->set_enable(false);
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

        for (const auto& [filename, stage_name, is_raid, is_paradox] : configs) {
            MultiCopilotTaskPlugin::MultiCopilotConfig config_cvt;
            auto copilot_opt = parse_copilot_filename(filename);
            if (!copilot_opt) {
                return false;
            }
            m_stage_name = Copilot.get_stage_name();
            if (auto result = Tile.find(m_stage_name); !result || !json::open(result->second)) {
                return false;
            }
            config_cvt.copilot_file = *copilot_opt;
            config_cvt.nav_name = stage_name;
            config_cvt.is_raid = is_raid;
            config_cvt.is_paradox = is_paradox;
            m_paradox_task_ptr->set_enable(is_paradox);
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
    }

    m_medicine_task_ptr->set_enable(use_sanity_potion);

    m_formation_task_ptr->set_enable(with_formation);
    m_formation_task_ptr->set_select_formation(formation_index);
    m_formation_task_ptr->set_add_trust(add_trust);
    m_formation_task_ptr->set_ignore_requirements(ignore_requirements);
    m_formation_task_ptr->set_support_unit_usage(support_unit_usage);
    m_formation_task_ptr->set_specific_support_unit(support_unit_name);

    if (auto opt = params.find<json::array>("user_additional"); add_user_additional && opt) {
        std::vector<std::pair<std::string, int>> user_additional;
        for (const auto& op : *opt) {
            std::string name = op.get("name", std::string());
            if (name.empty()) {
                continue;
            }
            user_additional.emplace_back(std::pair<std::string, int> { std::move(name), op.get("skill", 1) });
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
             m_stop_task_ptr->set_tasks({ "Copilot@ClickCornerUntilEndOfAction" });
         }*/
        m_stop_task_ptr->set_tasks({ "Copilot@ClickCornerUntilEndOfAction" });
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
