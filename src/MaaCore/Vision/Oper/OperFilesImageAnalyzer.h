#pragma once

#include <optional>

#include "Vision/VisionHelper.h"

namespace asst
{
// 档案页（干员信息页）干员培养状态识别，roi 与模板由 AutoRaise@CurrentXxx 任务提供。
class OperFilesImageAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~OperFilesImageAnalyzer() override = default;

    // 识别技能槽 skill（1-3）的当前专精等级（0-3），识别失败返回 std::nullopt。
    std::optional<int> mastery_level(int skill);

    // 识别当前干员的精英化阶段（0-2），识别失败返回 std::nullopt。
    std::optional<int> elite_level();
};
} // namespace asst
