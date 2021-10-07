#pragma once

#include <opencv2/opencv.hpp>

#include "AsstDef.h"

namespace asst {
    class AbstractImageAnalyzer
    {
    public:
        AbstractImageAnalyzer() = default;
        AbstractImageAnalyzer(const cv::Mat& image, const Rect& roi = Rect());
        AbstractImageAnalyzer(const AbstractImageAnalyzer&) = delete;
        AbstractImageAnalyzer(AbstractImageAnalyzer&&) = delete;
        virtual ~AbstractImageAnalyzer() = default;

        virtual void set_image(const cv::Mat& image, const Rect& roi = Rect());
        virtual void set_roi(const Rect& roi) noexcept;
        virtual bool analyze() = 0;

        AbstractImageAnalyzer& operator=(const AbstractImageAnalyzer&) = delete;
        AbstractImageAnalyzer& operator=(AbstractImageAnalyzer&&) = delete;

    protected:
        static Rect empty_rect_to_full(const Rect& rect, const cv::Mat& image) noexcept;

        cv::Mat m_image;
        Rect m_roi;
    };
}