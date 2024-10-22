#pragma once
#include "Config/Miscellaneous/StageDropsConfig.h"
#include "Vision/VisionHelper.h"

#include <optional>

namespace asst
{
class StageDropsImageAnalyzer final : public VisionHelper
{
    static constexpr const char* LMD_ID = "4001";

public:
    using VisionHelper::VisionHelper;
    virtual ~StageDropsImageAnalyzer() override = default;

    bool analyze();

    StageKey get_stage_key() const;
    int get_stars() const noexcept;
    int get_times() const noexcept;

    // <drop_type, <item_id, quantity>>
    const auto& get_drops() const noexcept { return m_drops; }

    bool analyze_baseline();

    std::vector<std::pair<Rect, StageDropType>> get_baseline() const noexcept { return m_baseline; }

    // merge a new image with a different material position into m_image
    // might resize m_image.
    // return offset in pixels
    std::optional<int> merge_image(const cv::Mat& new_img);

protected:
    bool analyze_stage_code();
    bool analyze_times();
    bool analyze_stars();
    bool analyze_difficulty();
    bool analyze_drops();
    // 落叶殇火 活动（异格夜刀）, act24side, 2023-03
    bool analyze_drops_for_CF();
    // 第十二章主线 活动，前两次打有三倍掉落，过滤，2023-04
    bool analyze_drops_for_12();

    int match_quantity(const Rect& roi, const std::string& item, bool use_word_model = false);
    std::optional<TextRect> match_quantity_string(const Rect& roi, bool use_word_model = false);
    std::optional<TextRect>
        match_quantity_string(const Rect& roi, const std::string& item, bool use_word_model = false);
    static int quantity_string_to_int(const std::string& str);

    StageDropType match_droptype(const Rect& roi);
    std::string match_item(const Rect& roi, StageDropType type, int index, int size);

    std::string m_stage_code;
    int m_times = -1; // -2 means recognition failed, -1 means not found
    StageDifficulty m_difficulty = StageDifficulty::Normal;
    int m_stars = 0;
    std::vector<std::pair<Rect, StageDropType>> m_baseline;
    // <drop_type, <item_id, quantity>>
    std::vector<StageDropInfo> m_drops;
};
}
