#include "AsstCallerExtra.h"

#if ASST_WITH_EXTRA_CALLERS

#include "Config/Miscellaneous/ItemConfig.h"
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

    const char* ASSTAPI AsstGetItemName(const char* id)
    {
        static const std::string empty;
        thread_local std::string item_id, item_name;
        if (!id) {
            item_id = empty;
        }
        else {
            item_id = id;
        }
        item_name = asst::ItemData.get_item_name(item_id);
        return item_name.c_str();
    }
}

#endif // ASST_WITH_EXTRA_CALLERS
