#include "SideStoryReopenTask.h"

#include "Config/TaskData.h"
#include "Task/Fight/MedicineCounterTaskPlugin.h"
#include "Task/Fight/StageQueueMissionCompletedTaskPlugin.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"

void asst::SideStoryReopenTask::set_sidestory_name(std::string sidestory_name)
{
    std::string upper_prefix = std::move(sidestory_name);
    ranges::transform(upper_prefix, upper_prefix.begin(), [](char ch) -> char {
        return static_cast<char>(::toupper(ch));
    });
    m_sidestory_name = upper_prefix;
}

void asst::SideStoryReopenTask::set_medicine(int medicine)
{
    m_medicine = medicine;
}

void asst::SideStoryReopenTask::set_expiring_medicine(int expiring_medicine)
{
    m_expiring_medicine = expiring_medicine;
}

void asst::SideStoryReopenTask::set_stone(int stone)
{
    m_stone = stone;
}

bool asst::SideStoryReopenTask::set_server(std::string server)
{
    m_server = std::move(server);
    return true;
}

bool asst::SideStoryReopenTask::set_enable_penguin(bool enable)
{
    m_enable_penguin = enable;
    return true;
}

bool asst::SideStoryReopenTask::set_penguin_id(std::string id)
{
    m_penguin_id = std::move(id);
    return true;
}

bool asst::SideStoryReopenTask::set_enable_yituliu(bool enable)
{
    // 暂时没用上，其他地方加了这里也加一个
    m_enable_yituliu = enable;
    return true;
}

bool asst::SideStoryReopenTask::_run()
{
    LogTraceFunction;

    if (need_exit()) {
        return false;
    }
    clear();

    if (m_sidestory_name.length() != 2) {
        Log.error(__FUNCTION__, "SideStory_name.len != 2");
        return false;
    }

    std::string m_sidestory_reopen_task = m_sidestory_name + "ChapterTo" + m_sidestory_name;
    if (!Task.get(m_sidestory_reopen_task)) {
        Log.error(__FUNCTION__, m_sidestory_reopen_task, "task not exists");

        json::value task_not_exists = basic_info_with_what("SideStoryReopenTaskNotExists");
        callback(AsstMsg::SubTaskExtraInfo, task_not_exists);
        return false;
    }
    Task.get("SideStoryReopen")->next = { m_sidestory_name + "ChapterTo" + m_sidestory_name };

    if (!at_normal_page() && !navigate_to_normal_page()) {
        Log.error(__FUNCTION__, "cound not navigate to normal page");
        return false;
    }

    // 选择关卡并依次配置Task信息
    for (int stage_index = 1; stage_index < 10; stage_index++) {
        if (need_exit()) {
            return false;
        }

        // 如果无法选中则结束任务，如果不是第一关则认为已完成
        if (!select_stage(stage_index)) {
            if (stage_index == 1) {
                json::value battle_info = basic_info_with_what("StageQueueStageNotFound");
                battle_info["details"]["stage_code"] = m_sidestory_name + "-" + (char)(stage_index + '0');

                callback(AsstMsg::SubTaskExtraInfo, battle_info);
                return false;
            }
            else {
                // 已打至index-1关
                return true;
            }
        }
        // 检查 / 激活代理，代不了就下一关
        if (!activate_prts()) {
            json::value battle_info = basic_info_with_what("StageQueueUnableToAgent");
            battle_info["details"]["stage_code"] = m_sidestory_name + "-" + (char)(stage_index + '0');
            callback(AsstMsg::SubTaskExtraInfo, battle_info);
            continue;
        }

        auto result = fight(false, false);
        if (!result) {
            if (m_sanity_not_enough) {
                if (m_medicine > m_cur_medicine) {
                    m_cur_medicine++;
                    result = fight(true, false);
                }
                else if (m_stone > m_cur_stone) {
                    m_cur_stone++;
                    result = fight(false, true);
                }
                else {
                    // 理智真不够了
                    return true;
                }
            }
        }
        if (!result) {
            // 遇到掉线、意外情况，结束任务
            return false;
        }
    }
    return true;
}

bool asst::SideStoryReopenTask::at_normal_page()
{
    std::vector<std::string> stage_name;
    for (int stage_index = 1; stage_index < 10; stage_index++) {
        stage_name.emplace_back(m_sidestory_name + "-" + std::to_string(stage_index));
    }
    Task.get<OcrTaskInfo>(m_sidestory_name + "@ClickStageName")->text = stage_name;
    Task.get<OcrTaskInfo>(m_sidestory_name + "@ClickedCorrectStage")->text = std::move(stage_name);
    Task.get<OcrTaskInfo>(m_sidestory_name + "@ClickedCorrectStageOrSwipe")->next = { m_sidestory_name +
                                                                                      "@ClickedCorrectStage" };

    return ProcessTask(*this, { m_sidestory_name + "@ClickStageName" }).set_retry_times(0).run();
}

/// <summary>
/// 从首页导航至普通关页面
/// </summary>
bool asst::SideStoryReopenTask::navigate_to_normal_page()
{
    LogTraceFunction;

    return ProcessTask(*this, { "StageBegin" }).set_times_limit("GoLastBattle", 0).run() &&
           ProcessTask(*this, { "SideStoryReopen" }).run();
}

/// <summary>
/// 点选关卡
/// </summary>
/// <param name="stage_index">第几关，从1开始</param>
bool asst::SideStoryReopenTask::select_stage(int stage_index)
{
    LogTraceFunction;

    std::string m_stage_code = m_sidestory_name + "-" + std::to_string(stage_index);

    Task.get<OcrTaskInfo>(m_stage_code + "@ClickStageName")->text = { m_stage_code };
    Task.get<OcrTaskInfo>(m_stage_code + "@ClickedCorrectStage")->text = { m_stage_code };
    return ProcessTask(*this, { m_stage_code + "@StageNavigationBegin" }).run();
}

/// <summary>
/// 检查 / 激活代理
/// </summary>
bool asst::SideStoryReopenTask::activate_prts()
{
    LogTraceFunction;
    return ProcessTask(*this, { "StageQueue@CheckPrts" }).run();
}

bool asst::SideStoryReopenTask::fight(bool use_medicine, bool use_stone)
{
    LogTraceFunction;
    m_sanity_not_enough = false;

    auto fight_task = ProcessTask(*this, { "StageQueue@StartButton1" });

    // 配置药+源石
    fight_task.set_times_limit("StageQueue@StoneConfirm", use_stone ? 1 : 0)
        .set_times_limit("StageQueue@StartButton1", 1)
        .set_times_limit("StageQueue@StartButton2", 1);

    auto medicine_plugin = fight_task.register_plugin<MedicineCounterTaskPlugin>();
    medicine_plugin->set_count(use_medicine ? 1 : 0);
    medicine_plugin->set_use_expiring(m_expiring_medicine);

    auto plugin = fight_task.register_plugin<StageQueueMissionCompletedTaskPlugin>();
    plugin->set_drop_stats(std::move(m_drop_stats));
    plugin->set_enable_penguin(m_enable_penguin);
    plugin->set_enable_yituliu(m_enable_yituliu);
    plugin->set_server(m_server);
    plugin->set_penguin_id(m_penguin_id);
    auto result = fight_task.run();
    m_drop_stats = plugin->get_drop_stats();
    if (!result) {
        return false;
    }
    if (fight_task.get_last_task_name() == "StageQueue@CloseStonePage") {
        m_sanity_not_enough = true;
    }
    else {
        return true;
    }
    return false;
}

void asst::SideStoryReopenTask::clear()
{
    m_cur_medicine = 0;
    m_cur_stone = 0;
}
