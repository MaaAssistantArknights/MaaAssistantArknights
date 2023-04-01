#pragma once

#include "Common/AsstTypes.h"
#include "InstHelper.h"
#include "Utils/NoWarningCVMat.h"
#include "Utils/Platform.hpp"

// #ifndef  ASST_DEBUG
// #define ASST_DEBUG
// #endif // ! ASST_DEBUG

namespace asst
{
    class TaskData;
    class Status;
    class Assistant;

    class AbstractImageAnalyzer : protected InstHelper
    {
    public:
        AbstractImageAnalyzer() = default;
        AbstractImageAnalyzer(const cv::Mat& image);
#ifdef ASST_DEBUG
        AbstractImageAnalyzer(const cv::Mat& image, cv::Mat& draw);
#endif
        AbstractImageAnalyzer(const cv::Mat& image, Assistant* inst);
        AbstractImageAnalyzer(const AbstractImageAnalyzer&) = delete;
        AbstractImageAnalyzer(AbstractImageAnalyzer&&) = delete;
        virtual ~AbstractImageAnalyzer() = default;

        virtual void set_image(const cv::Mat& image);
#ifdef ASST_DEBUG
        virtual void set_image(const cv::Mat& image, cv::Mat& draw);
#endif
        virtual void set_roi(const Rect& roi) noexcept;

        virtual bool analyze() = 0;
        virtual void set_log_tracing(bool enable);

        AbstractImageAnalyzer& operator=(const AbstractImageAnalyzer&) = delete;
        AbstractImageAnalyzer& operator=(AbstractImageAnalyzer&&) = delete;

        bool save_img(const std::filesystem::path& relative_dir = utils::path("debug"));

#ifdef ASST_DEBUG
        cv::Mat get_draw() const;
#endif

    protected:
        using InstHelper::status;

    protected:
        static Rect correct_rect(const Rect& rect, const cv::Mat& image) noexcept;

        cv::Mat m_image;
        Rect m_roi;
        bool m_log_tracing = true;

#ifdef ASST_DEBUG
        cv::Mat m_image_draw;
#endif

    private:
        using InstHelper::ctrler;
        using InstHelper::need_exit;
    };
} // namespace asst
