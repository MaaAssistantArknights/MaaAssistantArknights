#pragma once

#include "Common/AsstBattleDef.h"
#include "Vision/VisionHelper.h"

namespace asst
{
class SupportListAnalyzer final : public VisionHelper
{
public:
    using VisionHelper::VisionHelper;
    virtual ~SupportListAnalyzer() noexcept override = default;

    using Friendship = battle::Friendship;
    using SupportUnit = battle::SupportUnit;

    /// <summary>
    /// 识别 <c>m_image</c> 中显示的助战列表页。
    /// </summary>
    /// <param name="role">当前助战列表所选择的职业，仅用于对干员名进行消歧义以区分不同升变形态下的阿米娅。</param>
    /// <returns>
    /// 若完整识别到至少一名助战干员，则返回 <c>true</c>，反之则返回 <c>false</c>。
    /// </returns>
    /// <remarks>
    /// 助战列表共有 9 个栏位，一页即一屏，屏幕上最多只能同时完整显示 8 名助战干员。
    /// 对每位助战干员，将依次识别其名称、精英阶段、等级与潜能；若遇到识别失败的项目，则跳过当前助战干员。
    /// 此外，还会判断与助战干员提供者之间的好友关系。
    /// </remarks>
    bool analyze(battle::Role role);

    [[nodiscard]] const std::vector<SupportUnit>& get_result() const { return m_result; };

private:
    /// <summary>
    /// 获取模版名 <c>s</c> 的后缀数字。
    /// </summary>
    /// <param name="s">模版名。</param>
    /// <param name="delimiter">模版名后缀数字前的分隔符。</param>
    /// <returns>
    /// 若成功识别到后缀数字，则返回识别结果，反之则返回 <c>std::nullopt</c>。
    /// </returns>
    /// <example>
    /// <code>
    /// get_suffix_num("SupportListAnalyzer-Elite-2.png", '-') == 2
    /// </code>
    /// </example>
    [[nodiscard]] static std::optional<int> get_suffix_num(const std::string& s, char delimiter = '-');

    std::vector<SupportUnit> m_result;
};
}
