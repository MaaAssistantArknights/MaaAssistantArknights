#pragma once

#include "Vision/AbstractImageAnalyzer.h"

namespace asst
{
    struct OperBoxInfo
    {
        std::string name;
        std::string id;
        int level = 0; //等级
        int elite = 0; //精英度
        bool own = false;
    };
    class OperBoxImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~OperBoxImageAnalyzer() override = default;
        virtual bool analyze() override;
        const auto& get_result() const noexcept { return m_current_page_opers; }

    private:
        bool analyzer_opers();

        std::vector<OperBoxInfo> m_current_page_opers;
#ifdef ASST_DEBUG
        cv::Mat m_image_draw_oper;
#endif  
    };
}
