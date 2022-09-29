#pragma once

#include "AbstractResource.h"
#include "SingletonHolder.hpp"

#include <unordered_map>
#include <unordered_set>

#include "NoWarningCVMat.h"

namespace asst
{
    class TemplResource final : public SingletonHolder<TemplResource>, public AbstractResource
    {
    public:
        virtual ~TemplResource() override = default;

        void set_load_required(std::unordered_set<std::string> required) noexcept;
        virtual bool load(const std::filesystem::path& path) override;

        bool exist_templ(const std::string& key) const noexcept;
        const cv::Mat get_templ(const std::string& key) const noexcept;

        void insert_or_assign_templ(const std::string& key, cv::Mat&& templ);

    private:
        std::unordered_set<std::string> m_templs_filename;
        std::unordered_map<std::string, cv::Mat> m_templs;

        bool m_loaded = false;
    };
}
