#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst
{
    class DepotImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        struct ItemInfo
        {
            std::string item_id;
            std::string item_name;
            int quantity = 0;
            Rect rect;
        };

    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~DepotImageAnalyzer() = default;

        virtual bool analyze() override;

    private:
        void resize();
        bool analyze_base_rect();
        bool analyze_all_items();

        bool check_roi_empty(const Rect& roi);
        bool match_item(const Rect& roi, /* out */ ItemInfo& item_info, std::vector<std::string> excludes = std::vector<std::string>());
        int match_quantity(const Rect& roi);

        Rect m_resized_rect;
        cv::Mat m_image_resized;
#ifdef ASST_DEBUG
        cv::Mat m_image_draw_resized;
#endif
        std::vector<Rect> m_all_items_roi;
        std::vector<std::string> m_exists_items;
        std::unordered_map<std::string, ItemInfo> m_result;
    };
}
