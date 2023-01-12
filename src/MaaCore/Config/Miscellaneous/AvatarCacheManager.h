#pragma once

#pragma once
#include "Config/AbstractResource.h"

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
        void set_avatar(const std::string& name, battle::Role role, const cv::Mat& avatar, bool overlay = true);

    private:
        std::unordered_map<battle::Role, std::unordered_map<std::string, cv::Mat>> m_avatars;
        std::filesystem::path m_path;
    };
    inline static auto& AvatarCache = AvatarCacheManager::get_instance();
}
