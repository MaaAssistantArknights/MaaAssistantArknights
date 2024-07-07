#include "AvatarCacheManager.h"

#include "../TaskData.h"
#include "BattleDataConfig.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"

bool asst::AvatarCacheManager::load(const std::filesystem::path& path)
{
    LogTraceFunction;

    if (path == m_save_path) {
        Log.info("already loaded", path.lexically_relative(UserDir.get()));
        return true;
    }
    Log.info("load", path.lexically_relative(UserDir.get()));

    m_save_path = path;

    LoadItem waiting_to_load;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const std::filesystem::path& filepath = entry.path();
        std::string name = utils::path_to_utf8_string(filepath.stem());

        auto role = BattleData.get_role(name);
        if (role == battle::Role::Unknown) {
            Log.warn("unknown oper", name);
            continue;
        }

        waiting_to_load[role].emplace(name, filepath);
    }

    m_load_future = std::async(std::launch::async, &AvatarCacheManager::_load, this, std::move(waiting_to_load));

    return true;
}

const asst::AvatarCacheManager::AvatarsMap& asst::AvatarCacheManager::get_avatars(battle::Role role)
{
    return m_avatars[role];
}

void asst::AvatarCacheManager::set_avatar(const std::string& name, battle::Role role, const cv::Mat& avatar,
                                          bool overlay)
{
    LogTraceFunction;
    Log.info(__FUNCTION__, name, ", overlay:", overlay);

    if (overlay) {
        m_avatars[role].insert_or_assign(name, avatar);
    }
    else {
        m_avatars[role].try_emplace(name, avatar);
        return;
    }

    if (BattleData.is_name_invalid(name)) {
        Log.error("invalid name", name);
        return;
    }

    auto path = m_save_path / utils::path(name + CacheExtension);
    Log.info(path.lexically_relative(UserDir.get()));

    asst::imwrite(path, avatar);
}

void asst::AvatarCacheManager::_load(LoadItem waiting_to_load)
{
    LogTraceFunction;

    if (waiting_to_load.empty()) {
        return;
    }

    std::unique_lock<std::mutex> lock(m_load_mutex);
    // const auto [_1, _2, w, h] = Task.get("BattleOperAvatar")->rect_move;
    constexpr int w = 60;
    constexpr int h = 60;

    for (const auto& [role, name_and_paths] : waiting_to_load) {
        for (const auto& [name, filepath] : name_and_paths) {

#ifdef ASST_DEBUG
            Log.trace(__FUNCTION__, name, filepath.lexically_relative(UserDir.get()));
#endif

            auto avatar = asst::imread(filepath);

            if (avatar.empty()) {
                Log.error("load failed", filepath.lexically_relative(UserDir.get()));
                continue;
            }
            if (avatar.cols != w || avatar.rows != h) {
                Log.error("size mismatch", filepath.lexically_relative(UserDir.get()), avatar.cols, avatar.rows);
                continue;
            }

            m_avatars[role].insert_or_assign(name, std::move(avatar));
        }
    }
}
