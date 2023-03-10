#pragma once

#include <optional>

#include "Common/AsstTypes.h"
#include "InstHelper.h"
#include "Utils/NoWarningCVMat.h"
#include "Utils/Platform.hpp"

// #ifndef  ASST_DEBUG
// #define ASST_DEBUG
// #endif // ! ASST_DEBUG

namespace asst
{
    class Assistant;

    class AbstractImageAnalyzer : protected InstHelper
    {
    public:
        AbstractImageAnalyzer() = default;
        AbstractImageAnalyzer(const cv::Mat& image, Assistant* inst = nullptr);
        virtual ~AbstractImageAnalyzer() = default;

        virtual void set_image(const cv::Mat& image);
        virtual void set_roi(const Rect& roi) noexcept;
        virtual bool save_img(const std::filesystem::path& relative_dir = utils::path("debug"));

    protected:
        using InstHelper::status;

    protected:
        static Rect correct_rect(const Rect& rect, const cv::Mat& image) noexcept;

        cv::Mat m_image;
        Rect m_roi;

#ifdef ASST_DEBUG
        cv::Mat m_image_draw;
#endif

    private:
        using InstHelper::ctrler;
        using InstHelper::need_exit;
    };
} // namespace asst
