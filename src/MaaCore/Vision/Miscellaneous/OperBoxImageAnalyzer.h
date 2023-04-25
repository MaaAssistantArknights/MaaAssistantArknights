#pragma once

#include "Vision/AbstractImageAnalyzer.h"

namespace asst
{
    struct OperBoxInfo
    {
        std::string id;
        std::string name;
        int level = 0; // 等级
        int elite = 0; // 精英度
        bool own = false;
        int potential = 0; // 潜能
        Rect rect;
    };
    class OperBoxImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~OperBoxImageAnalyzer() override = default;
        virtual bool analyze() override;
        const auto& get_result() const noexcept { return m_current_page_opers; }

    private:
        int level_num(const std::string& level);
        bool analyzer_oper_box();
        // 获取lv rect
        bool lv_flag_analyzer();
        bool names_analyzer();
        bool level_analyzer();
        bool elite_analyzer();
        bool potential_analyzer();
        // 按位置排序
        // 1 - 2 - 3
        // 4 - 5 - 6
        void sort_oper_horizontal(std::vector<asst::OperBoxInfo> m_oper_boxs);
        std::vector<asst::OperBoxInfo> m_current_page_opers;
#ifdef ASST_DEBUG
        cv::Mat m_image_draw_oper;
#endif
    };
}
