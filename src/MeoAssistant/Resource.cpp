#include "Resource.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include <meojson/json.hpp>

#include "AsstTypes.h"
#include "Logger.hpp"

bool asst::Resource::load(const std::string& dir)
{
    LogTraceFunction;

    constexpr static const char* TemplsFilename = "template";
    constexpr static const char* GeneralCfgFilename = "config.json";
    constexpr static const char* TaskDataFilename = "tasks.json";
    constexpr static const char* CombatRecruitCfgFilename = "combat_recruit.json";
    constexpr static const char* RecruitCfgFilename = "recruit.json";
    constexpr static const char* ItemCfgFilename = "item_index.json";
    constexpr static const char* InfrastCfgFilename = "infrast.json";
    constexpr static const char* CopilotCfgDirname = "copilot";
    constexpr static const char* RoguelikeCfgDirname = "roguelike";
    constexpr static const char* OcrResourceFilename = "PaddleOCR";
    constexpr static const char* PenguinResourceFilename = "penguin-stats-recognize";
    constexpr static const char* TilesCalcResourceFilename = "Arknights-Tile-Pos";

    bool overload = false;

    /* 加载各个Json配置文件 */
    if (!m_general_cfg_unique_ins.load(dir + GeneralCfgFilename)) {
        if (!m_loaded) {
            m_last_error = std::string(GeneralCfgFilename) + ": " + m_general_cfg_unique_ins.get_last_error();
            return false;
        }
    }
    else {
        overload = true;
    }

    if (!TaskData::get_instance().load(dir + TaskDataFilename)) {
        if (!m_loaded) {
            m_last_error = std::string(TaskDataFilename) + ": " + TaskData::get_instance().get_last_error();
            return false;
        }
    }
    else {
        overload = true;
    }

    if (!m_recruit_cfg_unique_ins.load(dir + RecruitCfgFilename)) {
        if (!m_loaded) {
            m_last_error = std::string(RecruitCfgFilename) + ": " + m_recruit_cfg_unique_ins.get_last_error();
            return false;
        }
    }
    else {
        overload = true;
    }

    if (!m_combatrecruit_cfg_unique_ins.load(dir + CombatRecruitCfgFilename)) {
        if (!m_loaded) {
            m_last_error = std::string(CombatRecruitCfgFilename) + ": " + m_combatrecruit_cfg_unique_ins.get_last_error();
            return false;
        }
    }
    else {
        overload = true;
    }

    if (!m_item_cfg_unique_ins.load(dir + ItemCfgFilename)) {
        if (!m_loaded) {
            m_last_error = std::string(ItemCfgFilename) + ": " + m_item_cfg_unique_ins.get_last_error();
            return false;
        }
    }
    else {
        overload = true;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir + CopilotCfgDirname)) {
        if (entry.path().extension() != ".json") {
            continue;
        }
        if (!m_copilot_cfg_unique_ins.load(entry.path().u8string())) {
            m_last_error = entry.path().u8string() + " Load failed";
            return false;
        }
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir + RoguelikeCfgDirname)) {
        if (entry.path().extension() != ".json") {
            continue;
        }
        if (!m_roguelike_cfg_unique_ins.load(entry.path().u8string())) {
            m_last_error = entry.path().u8string() + " Load failed";
            return false;
        }
    }

    if (!m_infrast_cfg_unique_ins.load(dir + InfrastCfgFilename)) {
        if (!m_loaded) {
            m_last_error = std::string(InfrastCfgFilename) + ": " + m_infrast_cfg_unique_ins.get_last_error();
            return false;
        }
    }
    else {
        overload = true;
    }

    /* 加载模板图片资源 */
    // task所需要的模板资源
    m_templ_resource_unique_ins.append_load_required(TaskData::get_instance().get_templ_required());
    // 基建所需要的模板资源
    m_templ_resource_unique_ins.append_load_required(m_infrast_cfg_unique_ins.get_templ_required());

    if (!m_templ_resource_unique_ins.load(dir + TemplsFilename)) {
        if (!m_loaded) {
            m_last_error = std::string(TemplsFilename) + ": " + m_templ_resource_unique_ins.get_last_error();
            return false;
        }
    }
    else {
        overload = true;
    }

    /* 加载OCR库所需要的资源 */
    //m_ocr_pack_unique_ins.set_param(opt.ocr_gpu_index, opt.ocr_thread_number);
    if (!m_ocr_pack_unique_ins.load(dir + OcrResourceFilename)) {
        if (!m_loaded) {
            m_last_error = std::string(OcrResourceFilename) + ": " + m_ocr_pack_unique_ins.get_last_error();
            return false;
        }
    }
    else {
        overload = true;
    }

    /* 加载企鹅数据识别库所需要的资源 */
    if (!m_penguin_pack_unique_ins.load(dir + PenguinResourceFilename)) {
        if (!m_loaded) {
            m_last_error = std::string(PenguinResourceFilename) + ": " + m_penguin_pack_unique_ins.get_last_error();
            return false;
        }
    }
    else {
        overload = true;
    }

    /* 加载地图格子识别库所需要的资源 */

    if (!m_tile_pack_unique_ins.load(dir + TilesCalcResourceFilename)) {
        if (!m_loaded) {
            m_last_error = std::string(TilesCalcResourceFilename) + ": " + m_tile_pack_unique_ins.get_last_error();
            return false;
        }
    }
    else {
        overload = true;
    }

    if (m_loaded) {
        return overload;
    }

    m_loaded = true;
    return true;
}
