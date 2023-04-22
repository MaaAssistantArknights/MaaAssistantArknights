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
        bool analyzer_opers();
        bool analyzer_opers_box();
        bool analyzer_box();
        // 按位置排序
        // 1 - 2 - 3
        // 4 - 5 - 6
        void sort_result_horizontal_m(std::vector<asst::MatchRect> m_rect_box);
        void sort_result_horizontal_t(std::vector<asst::TextRect> t_rect_box);
        int level_num(std::string level);
#ifdef ASST_DEBUG
        cv::Mat m_image_draw_oper;
#endif
        void oper_box_clear();
        void sort_all();
        std::vector<asst::OperBoxInfo> m_current_page_opers;
        std::unordered_map<std::string, asst::OperBoxInfo> m_result;

        bool get_all_info();
        //精英度与lv_flag是否相邻
        bool is_near(asst::Rect rect1, asst::Rect rect2);
        //void 
        void draft_img();

        //干员名
        std::vector<asst::TextRect> m_oper_names;
        //等级数
        std::vector<asst::TextRect> m_oper_levels;
        //lv_flag
        std::vector<asst::MatchRect> m_lv_flags;
        //精英一位置
        std::vector<asst::MatchRect> m_elite_ones;

        // 精英二位置
        std::vector<asst::MatchRect> m_elite_twos;
    };
}
