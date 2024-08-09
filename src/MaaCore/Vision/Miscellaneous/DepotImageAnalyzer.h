#pragma once
#include "Vision/VisionHelper.h"

namespace asst
{
struct ItemInfo
{
    std::string item_id;
    std::string item_name;
    int quantity = 0;
    Rect rect;
};

class DepotImageAnalyzer final : public VisionHelper
{
public:
    static constexpr size_t NPos = ~0ULL;

public:
    using VisionHelper::VisionHelper;
    virtual ~DepotImageAnalyzer() override = default;

    bool analyze();

    void set_match_begin_pos(size_t pos) noexcept;
    size_t get_match_begin_pos() const noexcept;

    const auto& get_result() const noexcept { return m_result; }

private:
    void resize();
    bool analyze_base_rect();
    bool analyze_all_items();

    bool check_roi_empty(const Rect& roi);
    size_t
        match_item(const Rect& roi, /* out */ ItemInfo& item_info, size_t begin_index = 0ULL, bool with_enlarge = true);
    int match_quantity(const ItemInfo& item);
    Rect resize_rect_to_raw_size(const Rect& rect);

    template <typename F>
    static cv::Mat image_from_function(const cv::Size& size, const F& func);

    size_t m_match_begin_pos = 0ULL;
    Rect m_resized_rect;
    cv::Mat m_image_resized;
#ifdef ASST_DEBUG
    cv::Mat m_image_draw_resized;
#endif
    std::vector<Rect> m_all_items_roi;
    std::unordered_map<std::string, ItemInfo> m_result;
};
}
