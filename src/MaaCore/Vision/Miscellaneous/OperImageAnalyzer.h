#pragma once

#include "Vision/AbstractImageAnalyzer.h"

namespace asst
{
    struct OperBoxInfo
    {
        std::string id;
        std::string name;
        int level = 0;
        bool own = false;
    };
    class OperImageAnalyzer final : public AbstractImageAnalyzer
    {
    
    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~OperImageAnalyzer() override = default;

        virtual bool analyze() override;

        const auto& get_result() const noexcept { return current_page_opers; }

    private:
        bool analyzer_opers();
#ifdef ASST_DEBUG
        cv::Mat m_image_draw_oper;
#endif

        std::vector<OperBoxInfo> current_page_opers;
    };
}
