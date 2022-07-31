#pragma once

#include "AbstractResource.h"

#include <unordered_map>
#include <unordered_set>

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable: 5054 )
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#endif
#include <opencv2/opencv.hpp>
#ifdef _MSC_VER
#pragma warning( pop )
#else
#pragma GCC diagnostic pop
#endif

namespace asst
{
    class TemplResource : public AbstractResource
    {
    public:

        virtual ~TemplResource() override = default;

        void set_load_required(std::unordered_set<std::string> required) noexcept;
        virtual bool load(const std::string& dir) override;

        bool exist_templ(const std::string& key) const noexcept;
        const cv::Mat get_templ(const std::string& key) const noexcept;

        void emplace_templ(std::string key, cv::Mat templ);

    private:
        std::unordered_set<std::string> m_templs_filename;
        std::unordered_map<std::string, cv::Mat> m_templs;

        bool m_loaded = false;
    };
}
