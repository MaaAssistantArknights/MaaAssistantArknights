#pragma once

#include "AbstractResource.h"

#include <atomic>
#include <unordered_map>
#include <unordered_set>

#include "MaaUtils/NoWarningCVMat.hpp"
#include "MaaUtils/SingletonHolder.hpp"

namespace asst
{
class TemplResource final : public MAA_NS::SingletonHolder<TemplResource>, public AbstractResource
{
public:
    virtual ~TemplResource() override = default;

    void set_load_required(std::unordered_set<std::string> required) noexcept;
    virtual bool load(const std::filesystem::path& path) override;

    const cv::Mat& get_templ(const std::string& name);

    uint64_t revision() const noexcept { return m_revision.load(std::memory_order_acquire); }

private:
    std::unordered_set<std::string> m_load_required;
    std::unordered_map<std::string, cv::Mat> m_templs;
    std::unordered_map<std::string, std::filesystem::path> m_templ_paths;
    std::atomic<uint64_t> m_revision { 0 };
};
}
