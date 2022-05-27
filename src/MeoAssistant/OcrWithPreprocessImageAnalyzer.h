#pragma once
#include "OcrImageAnalyzer.h"

namespace asst
{
    class OcrWithPreprocessImageAnalyzer : public OcrImageAnalyzer
    {
    public:
        using OcrImageAnalyzer::OcrImageAnalyzer;
        virtual ~OcrWithPreprocessImageAnalyzer() noexcept = default;

        virtual bool analyze() override;

        void set_threshold(int lower, int upper = 255);
        void set_split(bool split);
        void set_expansion(int expansion);

        virtual void set_task_info(OcrTaskInfo task_info) noexcept override;

    protected:

        int m_threshold_lower = 140;
        int m_threshold_upper = 255;
        bool m_split = false;
        int m_expansion = 2;

    private:
        virtual void set_use_cache(bool is_use) noexcept override { std::ignore = is_use; }
        virtual void set_region_of_appeared(Rect region) noexcept override { std::ignore = region; }
    };
}
