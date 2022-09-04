#include "ResourceLoader.h"

#include <filesystem>

#include "Logger.hpp"

#include "BattleDataConfiger.h"
#include "CopilotConfiger.h"
#include "GeneralConfiger.h"
#include "InfrastConfiger.h"
#include "ItemConfiger.h"
#include "OcrPack.h"
#include "RecruitConfiger.h"
#include "RoguelikeCopilotConfiger.h"
#include "RoguelikeRecruitConfiger.h"
#include "RoguelikeShoppingConfiger.h"
#include "StageDropsConfiger.h"
#include "TaskData.h"
#include "TemplResource.h"
#include "TilePack.h"

bool asst::ResourceLoader::load(const std::filesystem::path& path)
{
#define LoadResourceAndCheckRet(Configer, Filename)                         \
    {                                                                       \
        LogTraceScope(std::string("LoadResourceAndCheckRet ") + #Configer); \
        auto full_path = path / Filename;                                   \
        bool ret = load_resource<Configer>(full_path);                      \
        if (!ret) {                                                         \
            Log.error(#Configer, " load failed, path:", full_path);         \
            return false;                                                   \
        }                                                                   \
    }

#define LoadResourceWithTemplAndCheckRet(Configer, Filename, TemplDir)                             \
    {                                                                                              \
        LogTraceScope(std::string("LoadResourceWithTemplAndCheckRet ") + #Configer);               \
        auto full_path = path / Filename;                                                          \
        auto full_templ_dir = path / TemplDir;                                                     \
        bool ret = load_resource_with_templ<Configer>(full_path, full_templ_dir);                  \
        if (!ret) {                                                                                \
            Log.error(#Configer, "load failed, path:", full_path, ", templ dir:", full_templ_dir); \
            return false;                                                                          \
        }                                                                                          \
    }

    LogTraceFunction;

    /* load resource with json files*/
    LoadResourceAndCheckRet(GeneralConfiger, "config.json");
    LoadResourceAndCheckRet(RecruitConfiger, "recruit.json");
    LoadResourceAndCheckRet(StageDropsConfiger, "stages.json");
    LoadResourceAndCheckRet(RoguelikeCopilotConfiger, "roguelike_copilot.json");
    LoadResourceAndCheckRet(RoguelikeRecruitConfiger, "roguelike_recruit.json");
    LoadResourceAndCheckRet(RoguelikeShoppingConfiger, "roguelike_shopping.json");
    LoadResourceAndCheckRet(BattleDataConfiger, "battle_data.json");

    /* load resource with json and template files*/
    LoadResourceWithTemplAndCheckRet(TaskData, "tasks.json", "template");
    LoadResourceWithTemplAndCheckRet(InfrastConfiger, "infrast.json", "template" / "infrast");
    LoadResourceWithTemplAndCheckRet(ItemConfiger, "item_index.json", "template" / "items");

    /* load 3rd parties resource */
    LoadResourceAndCheckRet(TilePack, "Arknights-Tile-Pos" / "levels.json");
    LoadResourceAndCheckRet(OcrPack, "PaddleOCR");

    m_loaded = true;

#undef LoadTemplByConfigerAndCheckRet
#undef LoadResourceAndCheckRet

    return true;
}
