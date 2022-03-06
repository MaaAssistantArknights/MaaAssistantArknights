#pragma once
#include "AbstractImageAnalyzer.h"

namespace asst
{
    class RoguelikeFormationImageAnalyzer final : public AbstractImageAnalyzer
    {
    public:
        struct Oper
        {
            Rect rect;
            bool selected = false;
            // TODO
        };
    public:
        RoguelikeFormationImageAnalyzer(const cv::Mat image, std::shared_ptr<TaskData> task_data);
        virtual ~RoguelikeFormationImageAnalyzer() = default;

        virtual bool analyze() override;

        const std::vector<Oper>& get_result() const noexcept;
    protected:
        // 该分析器不支持外部设置ROI
        virtual void set_roi(const Rect& roi) noexcept override
        {
            AbstractImageAnalyzer::set_roi(roi);
        }
        virtual void set_image(const cv::Mat image, const Rect& roi)
        {
            AbstractImageAnalyzer::set_image(image, roi);
        }

        bool selected_analyze(const Rect& roi);

        std::vector<Oper> m_result;
    };
}
