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
#define LoadResouceAndCheckRet(Configer, Filename)     \
    {                                                  \
        auto full_path = path / Filename;              \
        bool ret = load_resource<Configer>(full_path); \
        if (!ret) {                                    \
            Log.error("Load", full_path, "failed");    \
            return false;                              \
        }                                              \
    }

#define LoadResouceWithTemplAndCheckRet(Configer, Filename, TemplDir)             \
    {                                                                             \
        auto full_path = path / Filename;                                         \
        auto full_templ_dir = path / TemplDir;                                    \
        bool ret = load_resource_with_templ<Configer>(full_path, full_templ_dir); \
        if (!ret) {                                                               \
            Log.error("Load", full_path, "with", full_templ_dir, "failed");       \
            return false;                                                         \
        }                                                                         \
    }

    LogTraceFunction;

    /* load resource with json files*/
    LoadResouceAndCheckRet(GeneralConfiger, "config.json");
    LoadResouceAndCheckRet(RecruitConfiger, "recruit.json");
    LoadResouceAndCheckRet(StageDropsConfiger, "stages.json");
    LoadResouceAndCheckRet(RoguelikeCopilotConfiger, "roguelike_copilot.json");
    LoadResouceAndCheckRet(RoguelikeRecruitConfiger, "roguelike_recruit.json");
    LoadResouceAndCheckRet(RoguelikeShoppingConfiger, "roguelike_shopping.json");
    LoadResouceAndCheckRet(BattleDataConfiger, "battle_data.json");

    /* load resource with json and template files*/
    LoadResouceWithTemplAndCheckRet(TaskData, "tasks.json", "template");
    LoadResouceWithTemplAndCheckRet(InfrastConfiger, "infrast.json", "template" / "infrast");
    LoadResouceWithTemplAndCheckRet(ItemConfiger, "item_index.json", "template" / "items");

    /* load 3rd parties resource */
    LoadResouceAndCheckRet(TilePack, "Arknights-Tile-Pos" / "levels.json");
    LoadResouceAndCheckRet(OcrPack, "PaddleOCR");

    m_loaded = true;

#undef LoadTemplByConfigerAndCheckRet
#undef LoadResouceAndCheckRet

    return true;
}
