#include "AvatarCacheManager.h"

#include "../TaskData.h"
#include "BattleDataConfig.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"

bool asst::AvatarCacheManager::load(const std::filesystem::path& path)
{
    LogTraceFunction;

    if (path == m_save_path) {
        Log.info("already loaded", path);
        return true;
    }
    Log.info("load", path);

    m_waiting_to_load.clear();
    m_save_path = path;

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

        m_waiting_to_load.insert_or_assign(name, filepath);
    }

    return true;
}

const asst::AvatarCacheManager::AvatarsMap& asst::AvatarCacheManager::get_avatars(battle::Role role)
{
    if (!m_waiting_to_load.empty()) {
        const auto& [_1, _2, w, h] = Task.get("BattleOperAvatar")->rect_move;
        for (auto& [name, filepath] : m_waiting_to_load) {
            Log.info("load", name, filepath);
            auto avatar = asst::imread(filepath);
            if (avatar.empty()) {
                Log.error("load failed", filepath);
                continue;
            }
            if (avatar.cols != w || avatar.rows != h) {
                Log.error("size mismatch", filepath, avatar.cols, avatar.rows);
                continue;
            }
            m_avatars[role].insert_or_assign(std::move(name), std::move(avatar));
        }
        m_waiting_to_load.clear();
    }

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
    Log.info(path);

    asst::imwrite(path, avatar);
}
