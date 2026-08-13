#pragma once
#include "Task/InterfaceTask.h"

#include <memory>
#include <meojson/json.hpp>

namespace asst
{
class MultiCopilotTaskPlugin;
class BattleProcessTask;
class BattleFormationTask;
class ProcessTask;
class ParadoxRecognitionTask;

// 抄作业任务
class CopilotTask final : public InterfaceTask
{
public:
    struct MultiCopilotConfig
    {
        int id = -1;
        std::string filename;                         // 文件名
        std::optional<std::string> nav_name_override; // 关卡名
        bool is_raid = false;                         // 是否是突袭

        MEO_JSONIZATION(MEO_OPT id, filename, MEO_OPT nav_name_override, MEO_OPT is_raid);
    };

public:
    inline static constexpr std::string_view TaskType = "Copilot";

    CopilotTask(const AsstCallback& callback, Assistant* inst);
    virtual ~CopilotTask() override = default;

    virtual bool set_params(const json::value& params) override;

    std::string get_stage_name() const { return m_stage_name; }

private:
    std::optional<std::filesystem::path> parse_copilot_filename(const std::string& name);

    std::shared_ptr<MultiCopilotTaskPlugin> m_multi_copilot_plugin_ptr = nullptr;
    std::shared_ptr<ProcessTask> m_medicine_task_ptr = nullptr;
    std::shared_ptr<BattleFormationTask> m_formation_task_ptr = nullptr;
    std::shared_ptr<BattleProcessTask> m_battle_task_ptr = nullptr;
    std::shared_ptr<ProcessTask> m_stop_task_ptr = nullptr;
    std::string m_stage_name;
    bool m_has_subtasks_duplicate = false;
};
}
