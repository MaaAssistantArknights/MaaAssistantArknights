#pragma once

#include "Vision/VisionHelper.h"

namespace asst
{
class SupportListAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~SupportListAnalyzer() noexcept override = default;

    struct SupportUnit // 备选助战干员
    {
        Rect rect;
        std::string name;              // 干员名称/代号
        int elite = 0;                 // 精英化阶段
        int level = 0;                 // 等级
        bool from_friend = false;      // 是否为好友助战
        // SomeType modules            // 模组
        // ———————— 以下字段仅在集成战略中有效 ————————
        int hope = 0;                  // 希望消耗
        int elite_after_promotion = 0; // 进阶后精英化阶段，仅在集成战略中有效，
        int level_after_promotion = 0; // 进阶后等级，仅在集成战略中有效，
    };

    bool analyze();
    [[nodiscard]] std::vector<SupportUnit> get_result() const { return m_result; };

private:
    std::vector<SupportUnit> m_result;
};
}
