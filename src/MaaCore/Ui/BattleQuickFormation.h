#pragma once

#include "Common/AsstMsg.h"
#include "Common/AsstTypes.h"
#include "InstHelper.h"
#include "MaaUtils/NoWarningCVMat.hpp"

namespace asst
{
class BattleQuickFormation : private InstHelper
{
public:
    struct SkillResult
    {
        Rect rect;
        int level = -1;
    };

public:
    BattleQuickFormation(const AsstCallback& callback, Assistant* inst, std::string_view task_chain);
    virtual ~BattleQuickFormation() = default;

private:
    // 用于创建 ProcessTask
    AsstCallback m_callback = nullptr;
    std::string_view m_task_chain;

public:
    // 检查干员等级, 返回 <elite, level>, 未识别到时返回 <-1, -1>
    std::pair<int, int> analyze_oper_level(const cv::Mat& image, asst::Rect flag);
    // 查找并选择指定技能
    std::optional<SkillResult> find_oper_skill(int skill, bool skip_level = false);

private:
    // 查找指定技能, return 技能区域及技能等级, reverse 为反向查找3技能; skip_check 跳过技能等级检查,
    // 返回的rect略大于技能icon区域
    std::optional<std::pair<asst::Rect, int>>
        find_skill(const cv::Mat& image, int skill, bool reverse, bool skip_level = false);
};
} // namespace asst
