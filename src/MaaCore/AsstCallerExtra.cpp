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

        const auto& level_key = level_metadata->first;
        thread_local auto stage_id = level_key.stageId;
        thread_local auto code = level_key.code;
        thread_local auto level_id = level_key.levelId;
        thread_local auto name = level_key.name;

        return { stage_id.c_str(), code.c_str(), level_id.c_str(), name.c_str() };
    }
}

#endif // ASST_WITH_EXTRA_CALLERS
