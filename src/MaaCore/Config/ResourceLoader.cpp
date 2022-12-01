#include "ResourceLoader.h"

#include <filesystem>

#include "GeneralConfig.h"
#include "Miscellaneous/BattleDataConfig.h"
#include "Miscellaneous/CopilotConfig.h"
#include "Miscellaneous/InfrastConfig.h"
#include "Miscellaneous/ItemConfig.h"
#include "Miscellaneous/OcrPack.h"
#include "Miscellaneous/RecruitConfig.h"
#include "Miscellaneous/StageDropsConfig.h"
#include "Miscellaneous/TilePack.h"
#include "Roguelike/RoguelikeCopilotConfig.h"
#include "Roguelike/RoguelikeRecruitConfig.h"
#include "Roguelike/RoguelikeShoppingConfig.h"
#include "TaskData.h"
#include "TemplResource.h"
#include "Utils/Logger.hpp"

bool asst::ResourceLoader::load(const std::filesystem::path& path)
{
#define LoadResourceAndCheckRet(Config, Filename)                         \
    {                                                                     \
        LogTraceScope(std::string("LoadResourceAndCheckRet ") + #Config); \
        auto full_path = path / Filename;                                 \
        bool ret = load_resource<Config>(full_path);                      \
        if (!ret) {                                                       \
            Log.error(#Config, " load failed, path:", full_path);         \
            return false;                                                 \
        }                                                                 \
    }

#define LoadResourceWithTemplAndCheckRet(Config, Filename, TemplDir)                             \
    {                                                                                            \
        LogTraceScope(std::string("LoadResourceWithTemplAndCheckRet ") + #Config);               \
        auto full_path = path / Filename;                                                        \
        auto full_templ_dir = path / TemplDir;                                                   \
        bool ret = load_resource_with_templ<Config>(full_path, full_templ_dir);                  \
        if (!ret) {                                                                              \
            Log.error(#Config, "load failed, path:", full_path, ", templ dir:", full_templ_dir); \
            return false;                                                                        \
        }                                                                                        \
    }

    LogTraceFunction;
    using namespace asst::utils::path_literals;

    /* load resource with json files*/
    LoadResourceAndCheckRet(GeneralConfig, "config.json"_p);
    LoadResourceAndCheckRet(RecruitConfig, "recruitment.json"_p);
    LoadResourceAndCheckRet(StageDropsConfig, "stages.json"_p);
    LoadResourceAndCheckRet(RoguelikeCopilotConfig, "roguelike_copilot.json"_p);
    LoadResourceAndCheckRet(RoguelikeRecruitConfig, "roguelike_recruit.json"_p);
    LoadResourceAndCheckRet(RoguelikeShoppingConfig, "roguelike_shopping.json"_p);
    LoadResourceAndCheckRet(BattleDataConfig, "battle_data.json"_p);

    /* load resource with json and template files*/
    LoadResourceWithTemplAndCheckRet(TaskData, "tasks.json"_p, "template"_p);
    LoadResourceWithTemplAndCheckRet(InfrastConfig, "infrast.json"_p, "template"_p / "infrast"_p);
    LoadResourceWithTemplAndCheckRet(ItemConfig, "item_index.json"_p, "template"_p / "items"_p);

    /* load 3rd parties resource */
    LoadResourceAndCheckRet(TilePack, "Arknights-Tile-Pos"_p / "levels.json"_p);
    LoadResourceAndCheckRet(WordOcr, "PaddleOCR"_p);
    LoadResourceAndCheckRet(CharOcr, "PaddleCharOCR"_p);

    m_loaded = true;

#undef LoadTemplByConfigAndCheckRet
#undef LoadResourceAndCheckRet

    return true;
}
