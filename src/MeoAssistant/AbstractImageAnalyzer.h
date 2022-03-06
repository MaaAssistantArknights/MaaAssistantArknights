#pragma once

#include <opencv2/opencv.hpp>

#include "AsstTypes.h"

namespace asst
{
    class TaskData;
    class AbstractImageAnalyzer
    {
    public:
        AbstractImageAnalyzer() = default;
        AbstractImageAnalyzer(const cv::Mat image);
        AbstractImageAnalyzer(const cv::Mat image, std::shared_ptr<TaskData> task_data);
        AbstractImageAnalyzer(const cv::Mat image, const Rect& roi);
        AbstractImageAnalyzer(const AbstractImageAnalyzer&) = delete;
        AbstractImageAnalyzer(AbstractImageAnalyzer&&) = delete;
        virtual ~AbstractImageAnalyzer() = default;

        virtual void set_image(const cv::Mat image, const Rect& roi = Rect());
        virtual void set_roi(const Rect& roi) noexcept;
        void set_task_data(std::shared_ptr<TaskData> task_data);

        virtual bool analyze() = 0;

        AbstractImageAnalyzer& operator=(const AbstractImageAnalyzer&) = delete;
        AbstractImageAnalyzer& operator=(AbstractImageAnalyzer&&) = delete;

    protected:
        virtual void _set_image(const cv::Mat image);
        static Rect empty_rect_to_full(const Rect& rect, const cv::Mat image) noexcept;

        cv::Mat m_image;
        Rect m_roi;
        std::shared_ptr<TaskData> m_task_data = nullptr;

#ifdef ASST_DEBUG
        cv::Mat m_image_draw;
#endif
    };
}
