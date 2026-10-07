#pragma once
#include "Vision/VisionHelper.h"

#include <optional>

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

    // 手动设置要识别的物品字典，不设置则默认使用 get_ordered_material_item_id
    void set_item_ids(std::vector<std::string> ids) noexcept { m_item_ids = std::move(ids); }

    // 基础物品识别只查指定物品，跳过其他合法物品。
    void set_is_basic(bool is_basic) noexcept { m_is_basic = is_basic; }

    const auto& get_result() const noexcept { return m_result; }

    bool is_quantity_recognition_complete() const noexcept { return m_quantity_recognition_complete; }

    bool has_reached_last_item() const noexcept { return m_reached_last_item; }

    const std::optional<Rect>& get_unrecognized_item_rect() const noexcept { return m_unrecognized_item_rect; }

    static void clear_cached_templates()
    {
        m_cached_templs.clear();
        m_template_mean_colors.clear();
    }

private:
    void resize();
    void prepare_cached_templates();
    const std::vector<std::string>& get_ordered_item_ids() const;
    static double color_diff(const cv::Scalar& a, const cv::Scalar& b);
    static std::vector<std::string> filter_candidates_by_color(
        const cv::Mat& roi,
        const std::unordered_map<std::string, cv::Scalar>& template_mean_colors,
        double max_diff = 400.0);
    static cv::Rect get_center_rect(const cv::Mat& img, int width = 80, int height = 40);
    bool analyze_base_rect();
    bool analyze_all_items();

    size_t
        match_item(const Rect& roi, /* out */ ItemInfo& item_info, size_t begin_index = 0ULL, bool with_enlarge = true);
    std::optional<int> match_quantity(const ItemInfo& item);
    Rect resize_rect_to_raw_size(const Rect& rect);

    template <typename F>
    static cv::Mat image_from_function(const cv::Size& size, const F& func);

    size_t m_match_begin_pos = 0ULL;
    std::vector<std::string> m_item_ids; // 为空时使用 get_ordered_material_item_id
    bool m_is_basic = false;             // 为 true 时匹配不到不中断
    bool m_quantity_recognition_complete = true;
    bool m_reached_last_item = false;
    std::optional<Rect> m_unrecognized_item_rect;
    Rect m_resized_rect;
    cv::Mat m_image_resized;
#ifdef ASST_DEBUG
    cv::Mat m_image_draw_resized;
#endif
    std::vector<Rect> m_all_items_roi;
    std::unordered_map<std::string, ItemInfo> m_result;

    inline static std::unordered_map<std::string, cv::Mat> m_cached_templs;
    inline static std::unordered_map<std::string, cv::Scalar> m_template_mean_colors;
};
}
