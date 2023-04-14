#pragma once
#include "AbstractImageAnalyzer.h"

#include <variant>

namespace asst
{
    class MatchImageAnalyzer : public AbstractImageAnalyzer
    {
    public:
        struct Result
        {
            std::string to_string() const
            {
                return "{ rect: " + rect.to_string() + ", score: " + std::to_string(score) + " }";
            }

            Rect rect;
            double score = 0.0;
        };

        using ResultOpt = std::optional<Result>;

    public:
        using AbstractImageAnalyzer::AbstractImageAnalyzer;
        virtual ~MatchImageAnalyzer() override = default;

        const ResultOpt& analyze();

        void set_task_info(const std::shared_ptr<TaskInfo>& task_ptr);
        void set_task_info(const std::string& task_name);

        void set_templ(std::variant<std::string, cv::Mat> templ);
        void set_threshold(double templ_thres) noexcept;
        void set_mask_range(int lower, int upper, bool mask_with_src = false) noexcept;
        void set_mask_with_close(int with_close) noexcept;

        const std::optional<Result>& result() const noexcept;

    protected:
        void set_task_info(MatchTaskInfo task_info) noexcept;

        std::variant<std::string, cv::Mat> m_templ;
        double m_templ_thres = 0.0;
        std::pair<int, int> m_mask_range;
        bool m_mask_with_src = false;
        bool m_mask_with_close = false;

        ResultOpt m_result;
    };
} // namespace asst
