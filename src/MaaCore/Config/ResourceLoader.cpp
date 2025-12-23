#include "ResourceLoader.h"

#include <array>
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
#include "Roguelike/JieGarden/RoguelikeCoppersConfig.h"
#include "Roguelike/RoguelikeCopilotConfig.h"
#include "Roguelike/RoguelikeMapConfig.h"
#include "Roguelike/RoguelikeRecruitConfig.h"
#include "Roguelike/RoguelikeShoppingConfig.h"
#include "Roguelike/RoguelikeStageEncounterConfig.h"
#include "Roguelike/Sami/RoguelikeCollapsalParadigmConfig.h"
#include "Roguelike/Sami/RoguelikeFoldartalConfig.h"
#include "TaskData.h"
#include "TemplResource.h"
#include "Utils/Logger.hpp"

asst::ResourceLoader::ResourceLoader()
{
    m_load_thread = std::thread(&ResourceLoader::load_thread_func, this);
}

void asst::ResourceLoader::load_thread_func()
{
    while (!m_load_thread_exit) {
        std::unique_lock<std::mutex> lock(m_load_mutex);

        if (m_load_queue.empty()) {
            m_load_cv.wait(lock);
            continue;
        }

        auto [res_ptr, path] = std::move(m_load_queue.front());
        m_load_queue.pop_front();
        lock.unlock();

        res_ptr->load(path);
    }
}

void asst::ResourceLoader::add_load_queue(AbstractResource& res, const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path)) {
        return;
    }

    std::unique_lock<std::mutex> lock(m_load_mutex);
    m_load_queue.emplace_back(&res, path);
    m_load_cv.notify_all();
}

void asst::ResourceLoader::cancel()
{
    m_load_thread_exit = true;

    {
        std::unique_lock<std::mutex> lock(m_load_mutex);
        m_load_cv.notify_all();
    }

    if (m_load_thread.joinable()) {
        m_load_thread.join();
    }
}

asst::ResourceLoader::~ResourceLoader()
{
    cancel();
}

bool asst::ResourceLoader::load(const std::filesystem::path& path)
{
    using namespace asst::utils::path_literals;

    if (!std::filesystem::exists(path)) {
        Log.error("Resource path not exists, path:", path);
        return false;
    }

    std::unique_lock<std::mutex> lock(m_entry_mutex);
    LogTraceFunction;

    // ==================== 辅助 lambda：资源加载与错误处理 ====================

    // 加载资源并尝试加载对应的 _custom.json
    // 注意：custom 文件始终位于资源根目录（path），这是历史设计，勿修改
    auto load_with_custom = [&]<typename T>(const std::filesystem::path& filename, const char* res_name) -> bool {
        auto full_path = path / filename;
        if (!load_resource<T>(full_path)) {
            Log.error(res_name, " load failed, path:", full_path);
            return false;
        }
        auto custom_path = path / (full_path.stem().string() + "_custom.json");
        if (std::filesystem::exists(custom_path)) {
            Log.info("Loading custom file for ", res_name, ", path:", custom_path);
            if (!load_resource<T>(custom_path)) {
                Log.error(res_name, " load failed, path:", custom_path);
                return false;
            }
        }
        return true;
    };

    // 加载带模板目录的资源
    auto load_with_templ = [&]<typename T>(
                               const std::filesystem::path& filename,
                               const std::filesystem::path& templ_dir,
                               const char* res_name) -> bool {
        auto full_path = path / filename;
        auto full_templ_dir = path / templ_dir;
        if (!load_resource_with_templ<T>(full_path, full_templ_dir)) {
            Log.error(res_name, "load failed, path:", full_path, ", templ dir:", full_templ_dir);
            return false;
        }
        return true;
    };

    // 加载缓存目录（不检查返回值，目录不存在时自动创建）
    auto load_cache = [&]<typename T>(const std::filesystem::path& cache_subdir) {
        auto full_path = UserDir.get() / "cache"_p / cache_subdir;
        if (!std::filesystem::exists(full_path)) {
            std::filesystem::create_directories(full_path);
        }
        T::get_instance().load(full_path);
    };

    // ==================== ONNX 模型（惰性加载） ====================
    if (!load_with_custom.template operator()<OnnxSessions>("onnx"_p / "skill_ready_cls.onnx"_p, "OnnxSessions") ||
        !load_with_custom.template operator()<OnnxSessions>("onnx"_p / "deploy_direction_cls.onnx"_p, "OnnxSessions") ||
        !load_with_custom.template operator()<OnnxSessions>("onnx"_p / "operators_det.onnx"_p, "OnnxSessions")) {
        return false;
    }

    // ==================== OCR 模型 ====================
    if (!load_with_custom.template operator()<WordOcr>("PaddleOCR"_p, "WordOcr") ||
        !load_with_custom.template operator()<CharOcr>("PaddleCharOCR"_p, "CharOcr")) {
        return false;
    }

    // ==================== 核心配置文件（同步加载） ====================
    if (!load_with_custom.template operator()<GeneralConfig>("config.json"_p, "GeneralConfig") ||
        !load_with_custom.template operator()<RecruitConfig>("recruitment.json"_p, "RecruitConfig") ||
        !load_with_custom.template operator()<BattleDataConfig>("battle_data.json"_p, "BattleDataConfig") ||
        !load_with_custom.template operator()<OcrConfig>("ocr_config.json"_p, "OcrConfig")) {
        return false;
    }

    // ==================== 缓存加载（依赖 BattleDataConfig） ====================
    load_cache.template operator()<AvatarCacheManager>("avatars"_p);

    // ==================== 任务配置（带模板） ====================
    if (std::filesystem::is_directory(path / "tasks"_p)) {
        if (!load_with_templ.template operator()<TaskData>("tasks"_p, "template"_p, "TaskData")) {
            return false;
        }
    }
    else if (std::filesystem::is_regular_file(path / "tasks.json"_p)) {
        Log.warn("================  DEPRECATED  ================");
        Log.warn(__FUNCTION__, "resource/tasks.json has been deprecated since v5.15.4");
        Log.warn("================  DEPRECATED  ================");
        if (!load_with_templ.template operator()<TaskData>("tasks.json"_p, "template"_p, "TaskData")) {
            return false;
        }
    }

    // ==================== OTA 配置（路径固定，勿修改） ====================
    if (!load_with_templ
             .template operator()<InfrastConfig>("infrast.json"_p, "template"_p / "infrast"_p, "InfrastConfig") ||
        !load_with_templ.template operator()<ItemConfig>("item_index.json"_p, "template"_p / "items"_p, "ItemConfig") ||
        !load_with_custom.template operator()<StageDropsConfig>("stages.json"_p, "StageDropsConfig") ||
        !load_with_custom.template operator()<TilePack>("Arknights-Tile-Pos"_p / "overview.json"_p, "TilePack")) {
        return false;
    }

    // ==================== Roguelike 配置 ====================
    // 参考: https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/6188
    // 原先使用异步加载，但存在竞态问题，现改为同步加载

    constexpr std::array roguelike_themes = { "Phantom", "Mizuki", "Sami", "Sarkaz", "JieGarden" };

    auto roguelike_path = [](std::string_view theme, const std::filesystem::path& subpath) {
        return "roguelike"_p / theme / subpath;
    };

    // Copilot Config
    for (auto theme : roguelike_themes) {
        if (!load_with_custom.template operator()<RoguelikeCopilotConfig>(
                roguelike_path(theme, "autopilot"_p),
                "RoguelikeCopilotConfig")) {
            return false;
        }
    }

    // Recruitment Config
    for (auto theme : roguelike_themes) {
        if (!load_with_custom.template operator()<RoguelikeRecruitConfig>(
                roguelike_path(theme, "recruitment.json"_p),
                "RoguelikeRecruitConfig")) {
            return false;
        }
    }

    // Shopping Config
    for (auto theme : roguelike_themes) {
        if (!load_with_custom.template operator()<RoguelikeShoppingConfig>(
                roguelike_path(theme, "shopping.json"_p),
                "RoguelikeShoppingConfig")) {
            return false;
        }
    }

    // Encounter Config
    for (auto theme : roguelike_themes) {
        if (!load_with_custom.template operator()<RoguelikeStageEncounterConfig>(
                roguelike_path(theme, "encounter"_p / "default.json"_p),
                "RoguelikeStageEncounterConfig")) {
            return false;
        }
    }
    // 额外的 encounter 配置（deposit / collapse）
    for (auto theme : { "Phantom", "Mizuki", "Sami" }) {
        if (!load_with_custom.template operator()<RoguelikeStageEncounterConfig>(
                roguelike_path(theme, "encounter"_p / "deposit.json"_p),
                "RoguelikeStageEncounterConfig")) {
            return false;
        }
    }
    if (!load_with_custom.template operator()<RoguelikeStageEncounterConfig>(
            roguelike_path("Sami", "encounter"_p / "collapse.json"_p),
            "RoguelikeStageEncounterConfig")) {
        return false;
    }

    // Map Config（仅 Sarkaz 和 JieGarden）
    for (auto theme : { "Sarkaz", "JieGarden" }) {
        if (!load_with_custom.template operator()<RoguelikeMapConfig>(
                roguelike_path(theme, "map.json"_p),
                "RoguelikeMapConfig")) {
            return false;
        }
    }

    // ==================== 主题专属插件配置 ====================
    // Sami
    if (!load_with_custom.template operator()<RoguelikeFoldartalConfig>(
            roguelike_path("Sami", "foldartal.json"_p),
            "RoguelikeFoldartalConfig") ||
        !load_with_custom.template operator()<RoguelikeCollapsalParadigmConfig>(
            roguelike_path("Sami", "collapsal_paradigms.json"_p),
            "RoguelikeCollapsalParadigmConfig")) {
        return false;
    }
    // JieGarden
    if (!load_with_custom.template operator()<RoguelikeCoppersConfig>(
            roguelike_path("JieGarden", "coppers.json"_p),
            "RoguelikeCoppersConfig")) {
        return false;
    }

    m_loaded = true;
    Log.info(__FUNCTION__, "ret", m_loaded);
    return m_loaded;
}

void asst::ResourceLoader::set_connection_extras(const std::string& name, const json::object& diff)
{
    GeneralConfig::get_instance().set_connection_extras(name, diff);
}

bool asst::ResourceLoader::loaded() const noexcept
{
    return m_loaded;
}
