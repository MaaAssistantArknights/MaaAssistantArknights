#include "ResourceLoader.h"

#include <filesystem>
#include <future>

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
            m_loaded = false;                                             \
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
            m_loaded = false;                                                                    \
            Log.error(#Config, "load failed, path:", full_path, ", templ dir:", full_templ_dir); \
            return false;                                                                        \
        }                                                                                        \
    }

    LogTraceFunction;
    using namespace asst::utils::path_literals;

    auto config_future = std::async(std::launch::async, [&]() -> bool {
        /* load resource with json files*/
        LoadResourceAndCheckRet(GeneralConfig, "config.json"_p);
        LoadResourceAndCheckRet(RecruitConfig, "recruitment.json"_p);
        LoadResourceAndCheckRet(StageDropsConfig, "stages.json"_p);
        LoadResourceAndCheckRet(RoguelikeCopilotConfig, "roguelike"_p / "copilot.json"_p);
        LoadResourceAndCheckRet(RoguelikeRecruitConfig, "roguelike"_p / "recruitment.json"_p);
        LoadResourceAndCheckRet(RoguelikeShoppingConfig, "roguelike"_p / "shopping.json"_p);
        LoadResourceAndCheckRet(BattleDataConfig, "battle_data.json"_p);

        /* load resource with json and template files*/
        LoadResourceWithTemplAndCheckRet(TaskData, "tasks.json"_p, "template"_p);
        LoadResourceWithTemplAndCheckRet(InfrastConfig, "infrast.json"_p, "template"_p / "infrast"_p);
        LoadResourceWithTemplAndCheckRet(ItemConfig, "item_index.json"_p, "template"_p / "items"_p);
        return true;
    });

    /* load 3rd parties resource */
    auto tile_future = std::async(std::launch::async, [&]() -> bool {
        LoadResourceAndCheckRet(TilePack, "Arknights-Tile-Pos"_p);
        return true;
    });

    auto ocr_future = std::async(std::launch::async, [&]() -> bool {
        LoadResourceAndCheckRet(WordOcr, "PaddleOCR"_p);
        return true;
    });

    auto ocr_char_future = std::async(std::launch::async, [&]() -> bool {
        LoadResourceAndCheckRet(CharOcr, "PaddleCharOCR"_p);
        return true;
    });

#undef LoadTemplByConfigAndCheckRet
#undef LoadResourceAndCheckRet

    if (!config_future.get() || !tile_future.get() || !ocr_future.get() || !ocr_char_future.get()) {
        m_loaded = false;
        return false;
    }

    m_loaded = true;
    return true;
}

bool asst::ResourceLoader::loaded() const noexcept
{
    return m_loaded;
}
