#pragma once

#include "AbstractResource.h"

#include <unordered_map>
#include <unordered_set>

#include <opencv2/opencv.hpp>

namespace asst
{
    class TemplResource : public AbstractResource
    {
    public:

        virtual ~TemplResource() = default;

        void append_load_required(std::unordered_set<std::string> required) noexcept;
        virtual bool load(const std::string& dir) override;

        bool exist_templ(const std::string& key) const noexcept;
        const cv::Mat& get_templ(const std::string& key) const noexcept;
        const std::pair<cv::Mat, cv::Rect>& get_hist(const std::string& key) const noexcept;

        void emplace_templ(std::string key, cv::Mat templ);
        void emplace_hist(std::string key, cv::Mat hist, cv::Rect roi);
        void clear_hists() noexcept;

    private:
        std::unordered_set<std::string> m_templs_filename;
        std::unordered_map<std::string, cv::Mat> m_templs;
        std::unordered_map<std::string, std::pair<cv::Mat, cv::Rect>> m_hists;
    };
}
