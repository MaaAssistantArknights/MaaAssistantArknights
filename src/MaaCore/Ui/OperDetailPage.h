#pragma once

#include <optional>

#include "Common/AsstMsg.h"
#include "Common/AsstTypes.h"
#include "InstHelper.h"
#include "MaaUtils/NoWarningCV.hpp"

namespace asst
{
class OperDetailPage : private InstHelper
{
public:
    struct SkillResult
    {
        Rect rect;
        int level = -1;
    };

public:
    OperDetailPage(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);
    virtual ~OperDetailPage() = default;

private:
    // 用于创建 ProcessTask
    AsstCallback m_callback = nullptr;
    std::string_view m_task_chain;

public:
    std::optional<int> analyze_elite(const cv::Mat& image);
    std::optional<int> analyze_level(const cv::Mat& image);
    std::optional<int> analyze_skill_level(const cv::Mat& image);
};
} // namespace asst
