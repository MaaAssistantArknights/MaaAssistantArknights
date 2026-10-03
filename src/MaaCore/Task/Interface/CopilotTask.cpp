#include "CopilotTask.h"

#include <algorithm>
#include <boost/regex.hpp>
#include <limits>

#include "Arknights-Tile-Pos/TileCalc2.hpp"

#include "Config/GeneralConfig.h"
#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/CopilotConfig.h"
#include "Controller/Controller.h"
#include "Task/Fight/MedicineCounterTaskPlugin.h"
#include "Task/Miscellaneous/BattleFormationTask.h"
#include "Task/Miscellaneous/BattleProcessTask.h"
#include "Task/Miscellaneous/MultiCopilotTaskPlugin.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"
#include "Vision/OCRer.h"

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
    m_medicine_task_ptr->register_plugin<MedicineCounterTaskPlugin>()->set_count(999'999);
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
    m_subtasks_per_run = m_subtasks.size();
}

bool asst::CopilotTask::run()
{
    if (!m_auto_restart) {
        return InterfaceTask::run();
    }

    if (run_with_auto_restart()) {
        return true;
    }

    save_img(utils::path("debug") / utils::path("interface"));
    return false;
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

    constexpr int DefaultAutoRestartTimes = 3; // 未传入参数时的默认重开次数
    constexpr int MinAutoRestartTimes = 1;     // 每个作业允许重开的次数下限
    constexpr int MaxAutoRestartTimes = 999;   // 与界面控件保持一致的重开次数上限

    const int normalized_auto_restart_times = std::clamp(
        params.get("auto_restart_times", DefaultAutoRestartTimes),
        MinAutoRestartTimes,
        MaxAutoRestartTimes);                                                  // 将外部参数限制在界面允许的范围内

    m_auto_restart = params.get("auto_restart", false);                        // 是否启用自动重开
    m_auto_restart_times = static_cast<size_t>(normalized_auto_restart_times); // 每个作业最大重开次数

    auto filename_opt = params.find<std::string>("filename");
    auto multi_tasks_opt = params.find<json::array>("copilot_list"); // 多任务列表
    if (!filename_opt && !multi_tasks_opt) {
        LogError << __FUNCTION__ << "CopilotTask set_params failed, stage_name or filename not found";
        return false;
    }

    if (filename_opt) {
        m_multi_copilot_plugin_ptr->set_enable(false);
        m_battle_task_ptr->set_wait_until_end(m_auto_restart);
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

        const size_t count = configs_cvt.size();
        if (count == 0) {
            LogError << __FUNCTION__ << "Copilot list is empty";
            return false;
        }
        m_run_count = count;
        // 追加任务
        const auto original_subtasks = m_subtasks;
        m_subtasks.reserve(original_subtasks.size() * count);
        for (size_t i = 1; i < count; ++i) {
            m_subtasks.insert(m_subtasks.end(), original_subtasks.begin(), original_subtasks.end());
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
            if (BattleData.is_name_invalid(battle::Role::Unknown, name)) {
                LogError << __FUNCTION__ << "| User additional oper" << name << "is invalid";
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
    m_battle_task_ptr->set_abort_on_leak(m_auto_restart);

    const size_t loop_times = std::max<size_t>(params.get("loop_times", 1), 1);
    m_stop_task_ptr->set_enable(false); // 清除上一次 set_params 留下的结算任务启用状态
    if (m_auto_restart) {
        m_stop_task_ptr->set_tasks({ "Copilot@WaitUntilEndOfAction-AutoRestart" });
        m_stop_task_ptr->set_enable(true);
    }
    else if (m_multi_copilot_plugin_ptr->get_enable()) {
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
    }

    if (!m_multi_copilot_plugin_ptr->get_enable()) {
        m_run_count = loop_times;
        if (loop_times > 1) {
            const auto original_subtasks = m_subtasks;
            m_subtasks.reserve(original_subtasks.size() * loop_times);
            for (size_t i = 1; i < loop_times; ++i) {
                m_subtasks.insert(m_subtasks.end(), original_subtasks.begin(), original_subtasks.end());
            }
            m_has_subtasks_duplicate = true;
        }
    }

    if (m_auto_restart) {
        // ProcessTask 会在多次尝试间复用并保留执行计数；下方状态机已经按作业限制重开次数，
        // 因此流程节点不能再使用一个跨作业累计的失败上限。
        m_stop_task_ptr->set_times_limit("Copilot@FightMissionFailed-AutoRestart", std::numeric_limits<int>::max());
        m_stop_task_ptr->set_times_limit("Copilot@WaitUntilEndOfAction-AutoRestart", std::numeric_limits<int>::max());
    }
    return true;
}

bool asst::CopilotTask::run_with_auto_restart()
{
    if (!m_enable) {
        LogInfo << __FUNCTION__ << "Task disabled, pass" << basic_info().to_string();
        return true;
    }
    m_running = true;
    notify_auto_restart(AutoRestartState::Enabled, 0);

    // 单作业没有用于导航的关卡名。首次开打前从已展开的详情面板读取当前显示编号，
    // 之后只用它在当前地图画面重新点开关卡，不进入多作业的跨页导航流程。
    if (!m_multi_copilot_plugin_ptr->get_enable()) {
        if (!cache_single_stage_display_name() || !cache_single_stage_mode(ctrler()->get_image())) {
            return false;
        }
    }

    for (size_t run_index = 0; run_index < m_run_count; ++run_index) {
        size_t restart_times = 0;
        while (!need_exit()) {
            const auto result = run_stage_attempt(run_index);
            if (result == StageAttemptResult::Success) {
                if (restart_times > 0) {
                    notify_auto_restart(AutoRestartState::Recovered, run_index, restart_times);
                }
                break;
            }
            if (result == StageAttemptResult::Error) {
                return false;
            }
            if (restart_times >= m_auto_restart_times) {
                notify_auto_restart(AutoRestartState::LimitReached, run_index, restart_times, result);
                return false;
            }

            ++restart_times;
            notify_auto_restart(AutoRestartState::Restarting, run_index, restart_times, result);

            // 无论是自然失败还是漏怪后主动放弃，游戏都会先进入失败结算页；
            // 必须返回关卡页后才能开始下一次尝试。
            if (!ProcessTask(*this, { "Copilot@ClickCornerUntilStartButton" }).set_retry_times(20).run()) {
                return false;
            }

            if (m_multi_copilot_plugin_ptr->get_enable()) {
                // 多作业每次加载配置时会先推进游标；重试前回退一次，确保仍加载并导航到当前作业。
                if (!m_multi_copilot_plugin_ptr->retry_current_config()) {
                    LogError << __FUNCTION__ << "Failed to rewind multi-copilot config for auto restart";
                    return false;
                }
            }
            else {
                if (!reopen_single_stage() || !restore_single_stage_mode()) {
                    return false;
                }
            }
        }
        if (need_exit()) {
            return false;
        }
    }
    return true;
}

bool asst::CopilotTask::cache_single_stage_mode(const cv::Mat& image)
{
    const auto mode = detect_single_stage_mode(image);
    if (!mode) {
        // 地图级难度在失败后仍处于原环境；没有详情页切换控件时无需主动切换。
        LogInfo << __FUNCTION__ << "No local stage mode switch detected; keep the inherited map mode";
        m_single_stage_mode.clear();
        return true;
    }

    m_single_stage_mode = mode->text;
    LogInfo << __FUNCTION__ << "Cached single copilot stage mode" << m_single_stage_mode;
    return true;
}

std::optional<asst::CopilotTask::SingleStageModeSwitch>
    asst::CopilotTask::detect_single_stage_mode(const cv::Mat& image) const
{
    OCRer ocr(image);
    ocr.set_task_info("Copilot@SingleStageModeOCR");
    const auto results = ocr.analyze();
    if (!results) {
        return std::nullopt;
    }

    const auto best = std::ranges::max_element(*results, {}, &OcrPack::Result::score);
    if (best == results->end() || best->text.empty() || best->score < 0.5) {
        return std::nullopt;
    }
    return SingleStageModeSwitch { .text = best->text, .rect = best->rect };
}

bool asst::CopilotTask::restore_single_stage_mode()
{
    if (m_single_stage_mode.empty()) {
        return true;
    }

    constexpr int MaxModeSwitches = 4;
    for (int attempt = 0; attempt <= MaxModeSwitches && !need_exit(); ++attempt) {
        const auto current = detect_single_stage_mode(ctrler()->get_image());
        if (current && current->text == m_single_stage_mode) {
            LogInfo << __FUNCTION__ << "Restored single copilot stage mode" << m_single_stage_mode;
            return true;
        }
        if (!current || attempt == MaxModeSwitches) {
            break;
        }

        LogInfo << __FUNCTION__ << "Rotate single copilot stage mode" << current->text << "->" << m_single_stage_mode;
        ctrler()->click(current->rect);
        sleep(Config.get_options().task_delay);
    }

    LogError << __FUNCTION__ << "Failed to restore single copilot stage mode" << m_single_stage_mode;
    return false;
}

bool asst::CopilotTask::cache_single_stage_display_name()
{
    const auto image = ctrler()->get_image();
    OCRer stage_ocr(image);
    stage_ocr.set_task_info("Copilot@SingleStageCodeOCR");
    const auto results = stage_ocr.analyze();
    if (!results) {
        LogError << __FUNCTION__ << "Failed to recognize stage code candidates";
        return false;
    }

    constexpr int P3StageListRight = 640;
    constexpr int DetailPanelLeft = 800;
    constexpr int DetailPanelBottom = 160;

    // P3 的列表页有独立的 SELECTED 标签。先由标签定位左侧选中行，再要求同一编号也出现在右侧详情中。
    // 这一分支不复用传统关卡图的详情区域和节点布局。
    for (const auto& selected : *results) {
        if (!selected.text.ends_with("ELECTED") || selected.rect.x >= P3StageListRight) {
            continue;
        }

        const OcrPack::Result* best = nullptr;
        int best_distance = std::numeric_limits<int>::max();
        for (const auto& candidate : *results) {
            const int vertical_distance = selected.rect.y - (candidate.rect.y + candidate.rect.height);
            const int horizontal_distance =
                std::abs((selected.rect.x + selected.rect.width / 2) - (candidate.rect.x + candidate.rect.width / 2));
            if (!is_stage_code_candidate(candidate.text) || candidate.rect.x >= P3StageListRight ||
                vertical_distance < 0 || vertical_distance > 60 || horizontal_distance > 120) {
                continue;
            }

            const bool also_in_detail = std::ranges::any_of(*results, [&](const OcrPack::Result& detail) {
                return stage_text_matches(detail.text, candidate.text) && detail.rect.x >= DetailPanelLeft &&
                       detail.rect.y < DetailPanelBottom;
            });
            if (also_in_detail && vertical_distance < best_distance) {
                best = &candidate;
                best_distance = vertical_distance;
            }
        }

        if (best) {
            m_single_stage_page_type = SingleStagePageType::P3StageList;
            m_single_stage_display_name = best->text;
            LogInfo << __FUNCTION__ << "Cached P3 single copilot stage" << m_single_stage_display_name;
            return true;
        }
    }

    // 传统关卡图没有 SELECTED 标签：以左侧地图节点的完整编号为准，右上详情 OCR 允许粘连前缀。
    // 突袭页的 OPERATION 图标可能与编号合并成 ERATIoNR8-8，不能要求两侧 OCR 文本完全相等。
    constexpr int StageMapRight = 800;
    for (const auto& candidate : *results) {
        if (!is_stage_code_candidate(candidate.text) || candidate.rect.x >= StageMapRight) {
            continue;
        }
        const bool also_in_detail = std::ranges::any_of(*results, [&](const OcrPack::Result& detail) {
            return stage_text_matches(detail.text, candidate.text) && detail.rect.x >= DetailPanelLeft &&
                   detail.rect.y < DetailPanelBottom;
        });
        if (also_in_detail) {
            m_single_stage_page_type = SingleStagePageType::StageMap;
            m_single_stage_display_name = candidate.text;
            LogInfo << __FUNCTION__ << "Cached stage-map single copilot stage" << m_single_stage_display_name;
            return true;
        }
    }

    LogError << __FUNCTION__ << "Failed to determine the current single copilot stage page";
    return false;
}

bool asst::CopilotTask::reopen_single_stage()
{
    constexpr int MaxAttempts = 3;
    for (int attempt = 1; attempt <= MaxAttempts && !need_exit(); ++attempt) {
        const auto image = ctrler()->get_image();
        if (confirm_single_stage(image)) {
            return true;
        }

        const auto stage_rect = find_stage_on_current_page(image, m_single_stage_display_name);
        if (!stage_rect) {
            LogWarn << __FUNCTION__ << "Stage is not visible on the current page" << m_single_stage_display_name
                    << "attempt" << attempt << "/" << MaxAttempts;
            sleep(Config.get_options().task_delay);
            continue;
        }

        ctrler()->click(*stage_rect);
        sleep(Config.get_options().task_delay);
        if (confirm_single_stage(ctrler()->get_image())) {
            LogInfo << __FUNCTION__ << "Reopened single copilot stage" << m_single_stage_display_name;
            return true;
        }
    }

    LogError << __FUNCTION__ << "Failed to reopen single copilot stage" << m_single_stage_display_name;
    return false;
}

std::optional<asst::Rect>
    asst::CopilotTask::find_stage_on_current_page(const cv::Mat& image, const std::string& stage_name) const
{
    OCRer ocr(image);
    ocr.set_task_info("Copilot@SingleStageCodeOCR");
    const auto results = ocr.analyze();
    if (!results) {
        return std::nullopt;
    }

    const int stage_area_right = m_single_stage_page_type == SingleStagePageType::P3StageList ? 640 : 800;
    for (const auto& result : *results) {
        if (is_stage_code_candidate(result.text) && stage_text_matches(result.text, stage_name) &&
            result.rect.x < stage_area_right && result.score >= 0.5) {
            return result.rect;
        }
    }
    return std::nullopt;
}

bool asst::CopilotTask::confirm_single_stage(const cv::Mat& image) const
{
    OCRer stage_ocr(image);
    stage_ocr.set_task_info("Copilot@SingleStageCodeOCR");
    const auto results = stage_ocr.analyze();
    constexpr int DetailAreaLeft = 800;
    const int stage_area_right = m_single_stage_page_type == SingleStagePageType::P3StageList ? 640 : 800;
    if (!results) {
        return false;
    }

    const bool in_detail = std::ranges::any_of(*results, [&](const OcrPack::Result& result) {
        return stage_text_matches(result.text, m_single_stage_display_name) && result.rect.x >= DetailAreaLeft &&
               result.rect.y < 160;
    });
    const bool on_stage_page = std::ranges::any_of(*results, [&](const OcrPack::Result& result) {
        return is_stage_code_candidate(result.text) && stage_text_matches(result.text, m_single_stage_display_name) &&
               result.rect.x < stage_area_right;
    });
    return in_detail && on_stage_page;
}

bool asst::CopilotTask::is_stage_code_candidate(const std::string& text)
{
    // 只约束关卡编号由多个 ASCII 段组成，不枚举主线、活动、EX、S 等具体命名规则。
    // 当前关卡最终仍由页面布局中的选中关系和左右重复文本共同确认。
    static const boost::regex StageCodeRegex(R"(^[A-Za-z0-9]+(?:[-_.][A-Za-z0-9]+)+$)");
    return boost::regex_match(text, StageCodeRegex);
}

bool asst::CopilotTask::stage_text_matches(const std::string& text, const std::string& stage_name)
{
    const auto normalize = [](const std::string& value) {
        std::string normalized;
        normalized.reserve(value.size());
        for (const unsigned char ch : value) {
            if (ch >= '0' && ch <= '9') {
                normalized.push_back(static_cast<char>(ch));
            }
            else if (ch >= 'A' && ch <= 'Z') {
                normalized.push_back(static_cast<char>(ch));
            }
            else if (ch >= 'a' && ch <= 'z') {
                normalized.push_back(static_cast<char>(ch - 'a' + 'A'));
            }
        }
        return normalized;
    };

    const std::string normalized_text = normalize(text);
    const std::string normalized_stage_name = normalize(stage_name);
    return !normalized_stage_name.empty() && normalized_text.ends_with(normalized_stage_name);
}

asst::CopilotTask::StageAttemptResult asst::CopilotTask::run_stage_attempt(size_t run_index)
{
    // m_subtasks 中的重复分组共享任务对象；每次尝试只运行当前执行轮次，避免重试时跳入下一轮。
    const size_t begin = run_index * m_subtasks_per_run;
    const size_t end = begin + m_subtasks_per_run;
    if (end > m_subtasks.size()) {
        LogError << __FUNCTION__ << "Invalid Copilot subtask range:" << begin << end << m_subtasks.size();
        return StageAttemptResult::Error;
    }

    const int task_delay = Config.get_options().task_delay;
    const int failed_times_before = m_stop_task_ptr->get_exec_times("Copilot@FightMissionFailed-AutoRestart");

    for (size_t index = begin; index < end; ++index) {
        if (need_exit()) {
            return StageAttemptResult::Error;
        }

        const auto& task_ptr = m_subtasks.at(index);
        if (!task_ptr->get_enable()) {
            continue;
        }

        LogTrace << __FUNCTION__ << "Run subtask" << index - begin + 1 << "/" << m_subtasks_per_run
                 << task_ptr->basic_info().to_string();
        task_ptr->set_task_id(m_task_id);

        const bool succeeded = task_ptr->run();
        // 即使流程图之后正常结束，也可能已经命中过失败节点，因此必须独立检查计数，不能只依赖返回值。
        if (task_ptr == m_stop_task_ptr &&
            m_stop_task_ptr->get_exec_times("Copilot@FightMissionFailed-AutoRestart") > failed_times_before) {
            return StageAttemptResult::RetryAfterFailure;
        }
        if (!succeeded) {
            if (task_ptr == m_battle_task_ptr && m_battle_task_ptr->has_leaked()) {
                return StageAttemptResult::RetryAfterLeak;
            }
            if (!task_ptr->get_ignore_error()) {
                return StageAttemptResult::Error;
            }
        }

        if (index + 1 != end) {
            sleep(task_delay);
        }
    }
    return StageAttemptResult::Success;
}

void asst::CopilotTask::notify_auto_restart(
    AutoRestartState state,
    size_t run_index,
    size_t restart_times,
    StageAttemptResult reason)
{
    const std::string state_name = [state]() {
        switch (state) {
        case AutoRestartState::Enabled:
            return "Enabled";
        case AutoRestartState::Restarting:
            return "Restarting";
        case AutoRestartState::Recovered:
            return "Recovered";
        case AutoRestartState::LimitReached:
            return "LimitReached";
        }
        return "Unknown";
    }();
    const std::string reason_name = reason == StageAttemptResult::RetryAfterLeak      ? "EnemyLeak"
                                    : reason == StageAttemptResult::RetryAfterFailure ? "BattleFailed"
                                                                                      : "None";

    // 核心状态保存在 debug/asst.log；同一状态还会通过回调由 WPF 写入 debug/gui.log，
    // 便于从底层日志和用户可见日志两条路径审查每次状态变化。
    const auto& log_level = state == AutoRestartState::LimitReached ? Logger::level::error
                            : state == AutoRestartState::Restarting ? Logger::level::warn
                                                                    : Logger::level::info;
    Log << log_level << __FUNCTION__ << "Copilot auto-restart state" << state_name << "run" << run_index + 1 << "/"
        << m_run_count << "restart" << restart_times << "/" << m_auto_restart_times << "reason" << reason_name;

    auto info = basic_info_with_what("CopilotAutoRestart");
    info["details"] = json::object {
        { "state", state_name },   { "times", restart_times },     { "max_times", m_auto_restart_times },
        { "reason", reason_name }, { "run_index", run_index + 1 }, { "run_count", m_run_count },
    };
    callback(AsstMsg::SubTaskExtraInfo, info);
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
