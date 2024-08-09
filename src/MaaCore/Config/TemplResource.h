#pragma once

#include "AbstractResource.h"

#include <unordered_map>
#include <unordered_set>

#include "Utils/NoWarningCVMat.h"
#include "Utils/SingletonHolder.hpp"

namespace asst
{
class TemplResource final : public SingletonHolder<TemplResource>, public AbstractResource
{
public:
    virtual ~TemplResource() override = default;

    void set_load_required(std::unordered_set<std::string> required) noexcept;
    virtual bool load(const std::filesystem::path& path) override;

    const cv::Mat& get_templ(const std::string& name);

private:
    std::unordered_set<std::string> m_load_required;
    std::unordered_map<std::string, cv::Mat> m_templs;
    std::unordered_map<std::string, std::filesystem::path> m_templ_paths;
};
}
