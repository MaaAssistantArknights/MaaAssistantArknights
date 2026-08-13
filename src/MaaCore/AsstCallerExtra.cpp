#include "AsstCallerExtra.h"

#if ASST_WITH_EXTRA_CALLERS

#include "Config/Miscellaneous/TilePack.h"

struct AsstMapLevelKey ASSTAPI AsstGetMapLevelKey(const char* key)
{
    const auto& level_metadata = asst::Tile.find(key);
    if (!level_metadata) {
        return { nullptr, nullptr, nullptr, nullptr };
    }
    const auto& level_key = level_metadata->first;
    return { .stage_id = level_key.stageId.c_str(),
             .code = level_key.code.c_str(),
             .level_id = level_key.levelId.c_str(),
             .name = level_key.name.c_str() };
}

#endif // ASST_WITH_EXTRA_CALLERS
