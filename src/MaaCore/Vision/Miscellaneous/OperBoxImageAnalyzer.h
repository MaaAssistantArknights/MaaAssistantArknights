#pragma once

#include "Vision/AbstractImageAnalyzer.h"
#include "Vision/MultiMatchImageAnalyzer.h"
#include "Vision/OcrWithPreprocessImageAnalyzer.h"

namespace asst
{
    struct OperBoxInfo
    {
        std::string id;
        std::string name;
        int level = 0; //等级
        int elite = 0; //精英度
        bool own = false;
        int potential = 0; //潜能
        Rect rect;
    };
    class OperBoxImageAnalyzer : public OcrWithPreprocessImageAnalyzer
    {
    public:
        OperBoxImageAnalyzer() = default;
        OperBoxImageAnalyzer(const cv::Mat& image);
        virtual ~OperBoxImageAnalyzer() override = default;
        virtual bool analyze() override;

        const auto& get_result_box() const noexcept { return m_current_page_opers; }
    protected:
        using OcrWithPreprocessImageAnalyzer::set_task_info;
        MultiMatchImageAnalyzer m_multi_match_image_analyzer;
    private:
        bool analyzer_oper_box();
        // 按位置排序
        // 1 - 2 - 3
        // 4 - 5 - 6
        void sort_oper_horizontal(std::vector<asst::OperBoxInfo> m_oper_boxs);
        int level_num(std::string level);
        //lv_flag
        std::vector<asst::MatchRect> m_lv_flags;
        std::vector<asst::OperBoxInfo> m_current_page_opers;
#ifdef ASST_DEBUG
        cv::Mat m_image_draw_oper;
#endif
    };
}
