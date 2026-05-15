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
        bool is_raid = false;               // 是否是突袭
        int id;
        bool is_sandbox = false;            // 是否是沙盘推演
        int sandbox_option1 = 1;            // 沙盘选项1 (1 or 2)
        int sandbox_option2 = 1;            // 沙盘选项2 (1 or 2)
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
    OCRer::ResultsVec find_stage(const cv::Mat& image, int threshold_low, int threshold_high);
    bool is_stage_detail_opened(const cv::Mat& image); // 检查关卡介绍是否已展开
    bool confirm_stage_name(const cv::Mat& image, const std::string& stage_name);

    std::vector<MultiCopilotConfig> m_copilot_configs;
    int m_index_current = 0; // 当前执行的索引
    std::shared_ptr<BattleProcessTask> m_battle_task_ptr = nullptr;
    int m_max_retry = 20;
};
}
