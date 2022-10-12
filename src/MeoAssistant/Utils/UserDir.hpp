#pragma once

#include "Utils/AsstUtils.hpp"
#include "Utils/SingletonHolder.hpp"

namespace asst
{
    class UserDir : public SingletonHolder<UserDir>
    {
    public:
        bool empty() const noexcept { return user_dir_.empty(); }
        const std::filesystem::path& get() const noexcept { return user_dir_; }
        bool set(const char* dir)
        {
            auto temp = utils::path(dir);
            if (!std::filesystem::exists(temp) || !std::filesystem::is_directory(temp)) {
                return false;
            }
            user_dir_ = temp;
            return true;
        }

    private:
        std::filesystem::path user_dir_;
    };
} // namespace asst
