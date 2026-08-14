#include "AsstCallerExtra.h"

#if ASST_WITH_EXTRA_CALLERS

#include "Config/Miscellaneous/TilePack.h"

extern "C"
{
    struct AsstMapLevelKey ASSTAPI AsstGetMapLevelKey(const char* key)
    {
        if (!key) {
            return { nullptr, nullptr, nullptr, nullptr };
        }
        const auto& level_metadata = asst::Tile.find(key);
        if (!level_metadata) {
            return { nullptr, nullptr, nullptr, nullptr };
        }
        thread_local Map::LevelKey level_key;
        level_key = level_metadata->first;
        return { level_key.stageId.c_str(), level_key.code.c_str(), level_key.levelId.c_str(), level_key.name.c_str() };
    }
}

#endif // ASST_WITH_EXTRA_CALLERS
