#pragma once
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"

namespace asst
{
class RoguelikeEncounterOptionAnalyzer final : public VisionHelper
{
public:
    struct Option
    {
        bool enabled;
        cv::Mat templ;
        std::string text;
    };

    using Result = std::vector<Option>;
    using ResultOpt = std::optional<Result>;

    using VisionHelper::VisionHelper;
    virtual ~RoguelikeEncounterOptionAnalyzer() override = default;

    /// <summary>
    /// 识别 <c>m_image</c> 中所有的事件选项。
    /// </summary>
    /// <returns>
    /// 若识别成功，则返回 <c>true</c>，若识别失败或遇到不支持的 <c>m_theme</c> 则返回 <c>false</c>。
    /// </returns>
    bool analyze();

    /// <summary>
    /// 将 <c>new_image</c> 从下方拼接到 <c>m_image</c>。
    /// </summary>
    /// <param name="new_img">要拼接到 <c>m_image</c> 的图像。</param>
    /// <returns>
    /// 拼接后 <c>m_image</c> 向下伸长的长度；若无法拼接图像或遇到不支持的 <c>m_theme</c> 则返回 <c>std::nullopt</c>。
    /// </returns>
    /// <remarks>
    /// 要求 <c>m_image</c> 与 <c>new_image</c> 宽度相同，分别展示事件选项列表的上、下部分视图，
    /// 两视图有重叠部分且重叠部分中至少有一个完整的事件选项。
    /// </remarks>
    std::optional<int> merge_image(const cv::Mat& new_img);

    /// <summary>
    /// 将 <c>m_theme</c> 设置为 <c>theme</c>，并输出相关日志。
    /// </summary>
    /// <param name="theme">要设置到 <c>m_theme</c> 的值。</param>
    /// <remarks>
    /// 若遇到不支持的 <c>theme</c> 则保持原 <c>m_theme</c>。
    /// </remarks>
    void set_theme(const std::string& theme);

    [[nodiscard]] const Result& get_result() const { return m_result; };

    /// <summary>
    /// 在 <c>image</c> 中匹配 <c>option_templ</c> 所对应的事件选项。
    /// </summary>
    /// <param name="theme">集成战略主题；目前只适配了界园主题。</param>
    /// <param name="image">目标图像。</param>
    /// <param name="option_templ">要匹配的事件选项的模版。</param>
    /// <returns>
    /// 匹配结果；若匹配失败或遇到不支持的 <c>theme</c> 则返回 <c>std::nullopt</c>。
    /// </returns>
    /// <remarks>
    /// 为了方便调用，此函数不依赖 <c>m_theme</c>，而是需要将 <c>theme</c> 作为参数传入。
    /// </remarks>
    [[nodiscard]] static Matcher::ResultOpt
        match_option(const std::string& theme, const cv::Mat& image, const cv::Mat& option_templ);

    using VisionHelper::save_img;

private:
    /// <summary>
    /// 识别 <c>image</c> 中的所有事件选项。
    /// </summary>
    /// <param name="image">要识别的图像。</param>
    /// <returns>
    /// 识别结果，按照 y 坐标顺序排列；若没有识别到任何事件选项，则返回 <c>std::nullopt</c>。
    /// </returns>
    [[nodiscard]] MultiMatcher::ResultsVecOpt analyze_options(const cv::Mat& image) const;

    /// <summary>
    /// 获取 <c>image</c> 中最下方事件选项的 y 坐标。
    /// </summary>
    /// <param name="image">要识别的图像。</param>
    /// <returns>
    /// <c>image</c> 中最下方事件选项的 y 坐标；若没有识别到任何事件选项，则返回 <c>UNDEFINED</c>。
    /// </returns>
    [[nodiscard]] int get_last_option_y(const cv::Mat& image) const;

    /// <summary>
    /// 将 <c>m_last_option_y</c> 设置为 <c>last_option_y</c>，并输出相关日志。
    /// </summary>
    /// <param name="last_option_y">要设置到 <c>m_last_option_y</c> 的值。</param>
    void set_last_option_y(int last_option_y);

    /// <summary>
    /// 对 <c>image</c> 进行 Otsu 全局阈值将二值化，以便于后续 OCR。
    /// </summary>
    /// <param name="image">要处理的图像。</param>
    /// <returns>
    /// 处理后的图像。
    /// </returns>
    /// <remarks>
    /// 用于对不可选事件选项的 templ 进行预处理以识别上面模糊的文字。
    /// </remarks>
    [[nodiscard]] static cv::Mat binarize_for_ocr(const cv::Mat& image);

    /// <summary>
    /// 将 <c>image</c> 保存于 debug/roguelike/encounter 目录下，并在输出 "Save {description} to {文件路径}" 的日志。
    /// </summary>
    /// <param name="image">要保存的图像。</param>
    /// <param name="description">填入日志信息中的描述图像的字符串。</param>
    /// <returns>
    /// 若文件写入成功，则返回 <c>true</c>，否则返回 <c>false</c>。
    /// </returns>
    static bool save_img(const cv::Mat& image, std::string_view description = "image");

    /// <summary>
    /// 集成战略主题；目前只适配了界园主题。
    /// </summary>
    std::string m_theme;

    /// <summary>
    /// y 坐标的无效哨兵值。
    /// </summary>
    /// <remarks>
    /// 主要用于 <c>m_last_option_y</c>。
    /// </remarks>
    static constexpr int UNDEFINED = -1;

    /// <summary>
    /// <c>m_image</c> 中最下方事件选项的 y 坐标。
    /// </summary>
    int m_last_option_y = UNDEFINED;

    /// <summary>
    /// <c>analyze</c> 后的事件选项识别结果。
    /// </summary>
    Result m_result;
};
} // namespace asst
