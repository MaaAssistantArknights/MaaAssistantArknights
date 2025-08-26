#pragma once
#include "Task/AbstractTask.h"

#include <variant>

namespace asst
{
class BattleProcessTask;

class MultiCopilotTaskPlugin : public AbstractTask
{
public:
    struct MultiCopilotConfig
    {
        std::variant<int, std::filesystem::path> copilot_file; // 文件名
        std::string nav_name;                                  // 关卡名
        bool is_raid = false;                                  // 是否需要导航
    };

public:
    using AbstractTask::AbstractTask;
    virtual ~MultiCopilotTaskPlugin() override = default;

    void set_multi_copilot_config(std::vector<MultiCopilotConfig> config) { m_copilot_configs = std::move(config); }

    void set_battle_task_ptr(const std::shared_ptr<BattleProcessTask>& ptr) { m_battle_task_ptr = ptr; }

private:
    virtual bool _run() override;

    std::vector<MultiCopilotConfig> m_copilot_configs;
    int m_index_current = 0; // 当前执行的索引
    std::shared_ptr<BattleProcessTask> m_battle_task_ptr = nullptr;
};
}
