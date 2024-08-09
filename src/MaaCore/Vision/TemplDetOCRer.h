#pragma once
#include "Config/MatcherConfig.h"
#include "Config/OCRerConfig.h"
#include "VisionHelper.h"

namespace asst
{
    class TemplDetOCRer : public VisionHelper, public OCRerConfig, public MatcherConfig
    {
    public:
        struct Result : public TextRect
        {
            Rect flag_rect;
            double flag_score = .0;
        };

        using ResultsVec = std::vector<Result>;
        using ResultsVecOpt = std::optional<ResultsVec>;

    public:
        using VisionHelper::VisionHelper;
        virtual ~TemplDetOCRer() override = default;

        void set_task_info(const std::string& templ_task_name, const std::string& ocr_task_name);
        void set_flag_rect_move(Rect flag_rect_move);
        void set_ocr_use_raw(bool use_raw) { m_use_raw = use_raw; }

        ResultsVecOpt analyze() const;
        // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
        const auto& get_result() const noexcept { return m_result; }

    protected:
        // from Config
        virtual void _set_roi(const Rect& roi) override { std::ignore = roi; }

    protected:
        using MatcherConfig::set_task_info;
        using OCRerConfig::set_task_info;
        using OCRerConfig::set_without_det;

    private:
        Rect m_flag_rect_move;
        bool m_use_raw = true;

    private:
        // FIXME: 老接口太难重构了，先弄个这玩意兼容下，后续慢慢全删掉
        mutable ResultsVec m_result;
    };
}
