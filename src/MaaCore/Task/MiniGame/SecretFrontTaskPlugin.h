#pragma once

#include "Task/AbstractTaskPlugin.h"

#include "Common/AsstTypes.h"

#include <array>
#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace cv
{
class Mat;
}

namespace asst
{
class SecretFrontTaskPlugin final : public AbstractTaskPlugin
{
public:
    using AbstractTaskPlugin::AbstractTaskPlugin;
    virtual ~SecretFrontTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

    enum class Ending
    {
        Unknown,
        A,
        B,
        C,
        D,
        E
    };

    struct EndingInfo
    {
        std::string_view hook;
        std::array<int, 3> target;
        Ending ending;
    };

protected:
    virtual bool _run() override;

private:
    struct CardData
    {
        int materiel = 0;
        int intelligence = 0;
        int medicine = 0;
        double percentage = 0.0; // 0~1

        // 计算预期收益，考虑概率衰减
        std::array<double, 3> expected_gain() const
        {
            return { materiel * percentage * percentage, intelligence * percentage * percentage, medicine * percentage * percentage };
        }
    };

    enum class Mode
    {
        Actions,    // 行动列表选卡（有数值）
        Event,      // 事件卡（无数值，仅基于文本选择）
        SelectTeam, // 选择分队
    };

    static Point card_pos_base(int total, int idx);

    std::optional<int> ocr_int(const cv::Mat& image, const Rect& roi) const;

    std::optional<std::array<int, 3>> read_properties(const cv::Mat& image) const;

    std::optional<CardData> read_card(const cv::Mat& image, int total, int idx) const;

    int estimate_total_cards(const cv::Mat& image) const;

    // 事件（无数值）时，使用文字 OCR 估计卡牌数量并读取选项名（如 1A/1B/2A）
    int estimate_total_cards_event(const cv::Mat& image) const;
    // 读取卡牌阶段名；如果 bin_threshold >= 0 则会在 OCR 上使用 set_bin_threshold(bin_threshold, 255)
    std::string read_stage_name(const cv::Mat& image, int total, int idx, int bin_threshold = -1) const;

    // 根据当前属性和路线，确定当前应该瞄准哪个阶段的目标
    std::array<int, 3> get_current_stage_target(const std::array<int, 3>& properties) const;

    struct BestChoice
    {
        int page = 0;       // 0, 1, 2
        int total = 0;      // card count on the chosen page
        int idx = 0;        // index of the chosen card on that page
        double score = 0.0; // score for debugging / logging
    };

    // 从当前页和随后的两页中选择全局最优卡片（会翻页读取），并把 UI 翻回到选择页
    BestChoice
        choose_best_card(const std::array<int, 3>& properties, int current_total, const std::vector<CardData>& cards);

    void click_choose_card(int total, int idx) const;

    // 根据当前结局类型返回路线（1A/2A/3A/...），用于事件卡选择
    std::vector<std::string_view> current_route() const;
    bool handle_event_page(const cv::Mat& image) const;

    mutable std::array<int, 3> m_target { 370, 385, 620 };
    mutable Ending m_ending { Ending::E };
    mutable Mode m_mode { Mode::Actions };
};
}
