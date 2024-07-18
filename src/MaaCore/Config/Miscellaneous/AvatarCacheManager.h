#pragma once

#pragma once
#include "Config/AbstractResource.h"

#include <future>
#include <unordered_map>

#include "Utils/NoWarningCVMat.h"

#include "Common/AsstBattleDef.h"
#include "Common/AsstTypes.h"

namespace asst
{
    class AvatarCacheManager final : public SingletonHolder<AvatarCacheManager>, public AbstractResource
    {
    public:
        using AvatarsMap = std::unordered_map<std::string, cv::Mat>;
        inline static const std::string CacheExtension = ".png";

    public:
        virtual ~AvatarCacheManager() override = default;

        virtual bool load(const std::filesystem::path& path) override;

        const AvatarsMap& get_avatars(battle::Role role);
        void remove_avatars(battle::Role role);
        void set_avatar(const std::string& name, battle::Role role, const cv::Mat& avatar, bool overlay = true);

    private:
        using LoadItem = std::unordered_map<battle::Role, std::unordered_map<std::string, std::filesystem::path>>;
        void _load(LoadItem waiting_to_load);

        std::filesystem::path m_save_path;
        std::future<void> m_load_future;
        std::mutex m_load_mutex;

        std::unordered_map<battle::Role, std::unordered_map<std::string, cv::Mat>> m_avatars;
    };
    inline static auto& AvatarCache = AvatarCacheManager::get_instance();
}
