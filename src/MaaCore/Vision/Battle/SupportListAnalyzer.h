#pragma once

#include "Common/AsstBattleDef.h"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"
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

    /// <summary>
    /// 将 <c>new_image</c> 从右方拼接到 <c>m_image</c>。
    /// </summary>
    /// <param name="new_img">要拼接到 <c>m_image</c> 的图像。</param>
    /// <returns>
    /// 拼接后 <c>m_image</c> 向右伸长的长度；若无法拼接图像则返回 <c>std::nullopt</c>。
    /// </returns>
    /// <remarks>
    /// 要求 <c>m_image</c> 与 <c>new_image</c> 高度相同，分别展示助战列表的左、右部分视图，
    /// 两视图有重叠部分且重叠部分中至少有一个完整的助战栏位。
    /// </remarks>
    std::optional<int> merge_image(const cv::Mat& new_img);

    [[nodiscard]] const std::vector<SupportUnit>& get_result() const { return m_result; };

    /// <summary>
    /// 在 <c>image</c> 中匹配 <c>support_unit_templ</c> 所对应的助战干员。
    /// </summary>
    /// <param name="image">目标图像。</param>
    /// <param name="support_unit_templ">要匹配的助战干员的模版。</param>
    /// <returns>
    /// 匹配结果；若匹配失败则返回 <c>std::nullopt</c>。
    /// </returns>
    [[nodiscard]] static Matcher::ResultOpt match_support_unit(const cv::Mat& image, const cv::Mat& support_unit_templ);

    using VisionHelper::save_img;

private:
    /// <summary>
    /// 识别 <c>image</c> 中的所有助战栏位。
    /// </summary>
    /// <param name="image">要识别的图像。</param>
    /// <returns>
    /// 识别结果，按照 x 坐标顺序排列；若没有识别到任何助战栏位，则返回 <c>std::nullopt</c>。
    /// </returns>
    [[nodiscard]] static MultiMatcher::ResultsVecOpt analyze_support_units(const cv::Mat& image);

    /// <summary>
    /// 获取 <c>image</c> 中最右方助战栏位的 x 坐标。
    /// </summary>
    /// <param name="image">要识别的图像。</param>
    /// <returns>
    /// <c>image</c> 中最右方助战栏位的 x 坐标；若没有识别到任何助战栏位，则返回 <c>UNDEFINED</c>。
    /// </returns>
    [[nodiscard]] static int get_last_support_unit_x(const cv::Mat& image);

    /// <summary>
    /// 将 <c>m_last_support_unit_x</c> 设置为 <c>last_support_unit_x</c>，并输出相关日志。
    /// </summary>
    /// <param name="last_support_unit_x">要设置到 <c>m_last_support_unit_x</c> 的值。</param>
    void set_last_support_unit_x(int last_support_unit_x);

    /// <summary>
    /// 获取模版名 <c>s</c> 的后缀数字。
    /// </summary>
    /// <param name="s">模版名。</param>
    /// <param name="delimiter">模版名后缀数字前的分隔符。</param>
    /// <returns>
    /// 若成功识别到后缀数字，则返回识别结果，否则返回 <c>std::nullopt</c>。
    /// </returns>
    /// <example>
    /// <code>
    /// get_suffix_num("SupportListAnalyzer-Elite-2.png", '-') == 2
    /// </code>
    /// </example>
    [[nodiscard]] static std::optional<int> get_suffix_num(const std::string& s, char delimiter = '-');

    /// <summary>
    /// 将 <c>image</c> 保存于 debug/supportListAnalyzer 目录下，并在输出 "Save {description} to {文件路径}" 的日志。
    /// </summary>
    /// <param name="image">要保存的图像。</param>
    /// <param name="description">填入日志信息中的描述图像的字符串。</param>
    /// <returns>
    /// 若文件写入成功，则返回 <c>true</c>，反之则返回 <c>false</c>。
    /// </returns>
    static bool save_img(const cv::Mat& image, std::string_view description = "image");

    inline static std::map<std::filesystem::path, size_t> m_save_file_cnt;

    /// <summary>
    /// x 坐标的无效哨兵值。
    /// </summary>
    /// <remarks>
    /// 主要用于 <c>m_last_slot_x</c>。
    /// </remarks>
    static constexpr int UNDEFINED = -1;

    /// <summary>
    /// <c>m_image</c> 中最右方助战栏位的 x 坐标。
    /// </summary>
    int m_last_support_unit_x = UNDEFINED;

    std::vector<SupportUnit> m_result;
};
}
