#pragma once

#include "Platform.hpp"
#include "SingletonHolder.hpp"

namespace asst
{
    enum class WorkingDirType
    {
        Res,
        User,
    };

    template <WorkingDirType type>
    class WorkingDir : public SingletonHolder<WorkingDir<type>>
    {
    public:
        bool empty() const noexcept { return dir_.empty(); }
        const std::filesystem::path& get() const noexcept { return dir_; }
        bool set(std::filesystem::path dir)
        {
            if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
                return false;
            }
            dir_ = std::move(dir);
            return true;
        }

    private:
        std::filesystem::path dir_;
    };

    static auto& ResDir = WorkingDir<WorkingDirType::Res>::get_instance();
    static auto& UserDir = WorkingDir<WorkingDirType::User>::get_instance();
} // namespace asst
