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
    if (!std::filesystem::exists(path)) {
        Log.error("Resource path not exists, path:", path);
        return false;
    }

#define LoadResourceAndCheckRet(Config, Filename)                 \
    {                                                             \
        auto full_path = path / Filename;                         \
        bool ret = load_resource<Config>(full_path);              \
        if (!ret) {                                               \
            Log.error(#Config, " load failed, path:", full_path); \
            return false;                                         \
        }                                                         \
    }

#ifdef ASST_DEBUG
    // DEBUG 模式下这里同步加载，并检查返回值的，方便排查问题
#define AsyncLoadConfig(Config, Filename) LoadResourceAndCheckRet(Config, Filename)
#else
#define AsyncLoadConfig(Config, Filename)                         \
    {                                                             \
        auto full_path = path / Filename;                         \
        bool ret = async_load_config<Config>(full_path);          \
        if (!ret) {                                               \
            Log.error(#Config, " load failed, path:", full_path); \
            return false;                                         \
        }                                                         \
    }
#endif // ASST_DEBUG

#define LoadResourceWithTemplAndCheckRet(Config, Filename, TemplDir)                             \
    {                                                                                            \
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

    std::vector<std::future<bool>> futures;

#ifdef ASST_DEBUG
    futures.emplace_back(std::async(std::launch::async, [&]() -> bool {
#endif // ASST_DEBUG
       // 不太重要又加载的慢的资源，但不怎么占内存的，实时异步加载
       // DEBUG 模式下这里还是检查返回值的，方便排查问题
        AsyncLoadConfig(StageDropsConfig, "stages.json"_p);
        AsyncLoadConfig(TilePack, "Arknights-Tile-Pos"_p);
        AsyncLoadConfig(RoguelikeCopilotConfig, "roguelike"_p / "copilot.json"_p);
        AsyncLoadConfig(RoguelikeRecruitConfig, "roguelike"_p / "recruitment.json"_p);
        AsyncLoadConfig(RoguelikeShoppingConfig, "roguelike"_p / "shopping.json"_p);
        AsyncLoadConfig(RoguelikeStageEncounterConfig, "roguelike"_p / "stage_encounter.json"_p);
#ifdef ASST_DEBUG
        return true;
    }));
#endif // ASST_DEBUG

    // 太占内存的资源，都是惰性加载
    futures.emplace_back(std::async(std::launch::async, [&]() -> bool {
        // 战斗中技能识别，二分类模型
        LoadResourceAndCheckRet(OnnxSessions, "onnx"_p / "skill_ready_cls.onnx"_p);
        // 战斗中部署方向识别，四分类模型
        LoadResourceAndCheckRet(OnnxSessions, "onnx"_p / "deploy_direction_cls.onnx"_p);
        // 战斗中干员（血条）检测，yolov8 检测模型
        LoadResourceAndCheckRet(OnnxSessions, "onnx"_p / "operators_det.onnx"_p);

        /* ocr */
        LoadResourceAndCheckRet(WordOcr, "PaddleOCR"_p);
        LoadResourceAndCheckRet(CharOcr, "PaddleCharOCR"_p);
        return true;
    }));

    // 重要的资源，实时加载
    futures.emplace_back(std::async(std::launch::async, [&]() -> bool {
        /* load resource with json files*/
        LoadResourceAndCheckRet(GeneralConfig, "config.json"_p);
        LoadResourceAndCheckRet(RecruitConfig, "recruitment.json"_p);
        LoadResourceAndCheckRet(BattleDataConfig, "battle_data.json"_p);
        LoadResourceAndCheckRet(OcrConfig, "ocr_config.json"_p);

        /* load cache */
        // 这个任务依赖 BattleDataConfig
        LoadCacheWithoutRet(AvatarCacheManager, "avatars"_p);
        return true;
    }));

    // 重要的资源，实时加载（图片还是惰性的）
    futures.emplace_back(std::async(std::launch::async, [&]() -> bool {
        /* load resource with json and template files*/
        LoadResourceWithTemplAndCheckRet(TaskData, "tasks.json"_p, "template"_p);
        LoadResourceWithTemplAndCheckRet(InfrastConfig, "infrast.json"_p, "template"_p / "infrast"_p);
        LoadResourceWithTemplAndCheckRet(ItemConfig, "item_index.json"_p, "template"_p / "items"_p);
        return true;
    }));

#undef LoadTemplByConfigAndCheckRet
#undef LoadResourceAndCheckRet
#undef LoadCacheWithoutRet

    m_loaded = ranges::all_of(futures, [](auto& f) { return f.get(); });
    return m_loaded;
}

bool asst::ResourceLoader::loaded() const noexcept
{
    return m_loaded;
}
