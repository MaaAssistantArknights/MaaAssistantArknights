#pragma once
#include "Task/AbstractTask.h"

#include <optional>

#include "MaaUtils/NoWarningCV.hpp"
#include "Vision/OCRer.h"

namespace asst
{
class BattleProcessTask;

class MultiCopilotTaskPlugin : public AbstractTask
{
public:
    struct MultiCopilotConfig
    {
        std::filesystem::path copilot_file; // 文件名
        std::string nav_name;               // 关卡名
        int id;
        int difficulty = 0;                 // 难度; 0 Ignore, 1 Normal 普通/险地/磨难, 2 Raid 突袭, 4 Sandbox 沙盘
        std::array<int, 2> sandbox = {};    // 沙盘选项
    };

public:
    using AbstractTask::AbstractTask;
    virtual ~MultiCopilotTaskPlugin() override = default;

    void set_multi_copilot_config(std::vector<MultiCopilotConfig> config) { m_copilot_configs = std::move(config); }

    void set_battle_task_ptr(const std::shared_ptr<BattleProcessTask>& ptr) { m_battle_task_ptr = ptr; }

private:
    virtual bool _run() override;
    bool navigate_to_stage(const std::string& stage_name);
    bool enter_stage(const Rect rect, const std::string& stage_name);
    OCRer::ResultsVec find_stage(
        const cv::Mat& image,
        std::tuple<int, int, int> threshold_low,
        std::tuple<int, int, int> threshold_high);
    bool is_stage_detail_opened(const cv::Mat& image); // 检查关卡介绍是否已展开
    bool confirm_stage_name(const cv::Mat& image, const std::string& stage_name);

    std::vector<MultiCopilotConfig> m_copilot_configs;
    int m_index_current = 0; // 当前执行的索引
    std::shared_ptr<BattleProcessTask> m_battle_task_ptr = nullptr;
    int m_max_retry = 20;
};
}
