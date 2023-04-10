#include "ResourceLoader.h"

#include <filesystem>
#include <future>

#include "GeneralConfig.h"
#include "Miscellaneous/AvatarCacheManager.h"
#include "Miscellaneous/BattleDataConfig.h"
#include "Miscellaneous/CopilotConfig.h"
#include "Miscellaneous/InfrastConfig.h"
#include "Miscellaneous/ItemConfig.h"
#include "Miscellaneous/OcrConfig.h"
#include "Miscellaneous/OcrPack.h"
#include "Miscellaneous/RecruitConfig.h"
#include "Miscellaneous/StageDropsConfig.h"
#include "Miscellaneous/TilePack.h"
#include "OnnxSessions.h"
#include "Roguelike/RoguelikeCopilotConfig.h"
#include "Roguelike/RoguelikeRecruitConfig.h"
#include "Roguelike/RoguelikeShoppingConfig.h"
#include "Roguelike/RoguelikeStageEncounterConfig.h"
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

#define LoadCacheWithoutRet(Config, Dir)                              \
    {                                                                 \
        LogTraceScope(std::string("LoadCacheWithoutRet ") + #Config); \
        auto full_path = UserDir.get() / "cache"_p / Dir;             \
        SingletonHolder<Config>::get_instance().load(full_path);      \
    }

    LogTraceFunction;
    using namespace asst::utils::path_literals;

    auto word_ocr_future = std::async(std::launch::async, [&]() -> bool {
        LoadResourceAndCheckRet(WordOcr, "PaddleOCR"_p);
        return true;
    });

    auto char_ocr_future = std::async(std::launch::async, [&]() -> bool {
        LoadResourceAndCheckRet(CharOcr, "PaddleCharOCR"_p);
        return true;
    });

    /* load resource with json files*/
    LoadResourceAndCheckRet(GeneralConfig, "config.json"_p);
    LoadResourceAndCheckRet(RecruitConfig, "recruitment.json"_p);
    LoadResourceAndCheckRet(StageDropsConfig, "stages.json"_p);
    LoadResourceAndCheckRet(RoguelikeCopilotConfig, "roguelike"_p / "copilot.json"_p);
    LoadResourceAndCheckRet(RoguelikeRecruitConfig, "roguelike"_p / "recruitment.json"_p);
    LoadResourceAndCheckRet(RoguelikeShoppingConfig, "roguelike"_p / "shopping.json"_p);
    LoadResourceAndCheckRet(RoguelikeStageEncounterConfig, "roguelike"_p / "stage_encounter.json"_p);
    LoadResourceAndCheckRet(BattleDataConfig, "battle_data.json"_p);
    LoadResourceAndCheckRet(OcrConfig, "ocr_config.json"_p);

    /* load resource with json and template files*/
    LoadResourceWithTemplAndCheckRet(TaskData, "tasks.json"_p, "template"_p);
    LoadResourceWithTemplAndCheckRet(InfrastConfig, "infrast.json"_p, "template"_p / "infrast"_p);
    LoadResourceWithTemplAndCheckRet(ItemConfig, "item_index.json"_p, "template"_p / "items"_p);

    /* load cache */
    LoadCacheWithoutRet(AvatarCacheManager, "avatars"_p);

    /*** lazy loading ***/
    // 战斗中技能识别，二分类模型
    LoadResourceAndCheckRet(OnnxSessions, "onnx"_p / "skill_ready_cls.onnx"_p);
    // 战斗中部署方向识别，四分类模型
    LoadResourceAndCheckRet(OnnxSessions, "onnx"_p / "deploy_direction_cls.onnx"_p);
    // 战斗中干员（血条）检测，yolov8 检测模型
    LoadResourceAndCheckRet(OnnxSessions, "onnx"_p / "operators_det.onnx"_p);

    /* tiles info */
    LoadResourceAndCheckRet(TilePack, "Arknights-Tile-Pos"_p);

#undef LoadTemplByConfigAndCheckRet
#undef LoadResourceAndCheckRet
#undef LoadCacheWithoutRet

    m_loaded = true;
    m_loaded &= word_ocr_future.get();
    m_loaded &= char_ocr_future.get();

    return m_loaded;
}

bool asst::ResourceLoader::loaded() const noexcept
{
    return m_loaded;
}
