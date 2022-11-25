#include "ResourceLoader.h"

#include <filesystem>

#include "Utils/Logger.hpp"

#include "Controller.h"
#include "Resource/BattleDataConfig.h"
#include "Resource/CopilotConfig.h"
#include "Resource/GeneralConfig.h"
#include "Resource/InfrastConfig.h"
#include "Resource/ItemConfig.h"
#include "Resource/OcrPack.h"
#include "Resource/RecruitConfig.h"
#include "Resource/RoguelikeCopilotConfig.h"
#include "Resource/RoguelikeRecruitConfig.h"
#include "Resource/RoguelikeShoppingConfig.h"
#include "Resource/StageDropsConfig.h"
#include "Resource/TemplResource.h"
#include "Resource/TilePack.h"
#include "TaskData.h"

bool asst::ResourceLoader::load(const std::filesystem::path& path)
{
#define LoadResourceAndCheckRet(Config, Filename)                         \
    {                                                                       \
        LogTraceScope(std::string("LoadResourceAndCheckRet ") + #Config); \
        auto full_path = path / Filename;                                   \
        bool ret = load_resource<Config>(full_path);                      \
        if (!ret) {                                                         \
            Log.error(#Config, " load failed, path:", full_path);         \
            return false;                                                   \
        }                                                                   \
    }

#define LoadResourceWithTemplAndCheckRet(Config, Filename, TemplDir)                             \
    {                                                                                              \
        LogTraceScope(std::string("LoadResourceWithTemplAndCheckRet ") + #Config);               \
        auto full_path = path / Filename;                                                          \
        auto full_templ_dir = path / TemplDir;                                                     \
        bool ret = load_resource_with_templ<Config>(full_path, full_templ_dir);                  \
        if (!ret) {                                                                                \
            Log.error(#Config, "load failed, path:", full_path, ", templ dir:", full_templ_dir); \
            return false;                                                                          \
        }                                                                                          \
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
