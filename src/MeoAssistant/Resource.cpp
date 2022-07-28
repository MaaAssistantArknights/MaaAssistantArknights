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

    constexpr static auto TemplsFilename = "template";
    constexpr static auto GeneralCfgFilename = "config.json";
    constexpr static auto TaskDataFilename = "tasks.json";
    constexpr static auto RoguelikeRecruitCfgFilename = "roguelike_recruit.json";
    constexpr static auto RecruitCfgFilename = "recruit.json";
    constexpr static auto ItemCfgFilename = "item_index.json";
    constexpr static auto InfrastCfgFilename = "infrast.json";
    constexpr static auto InfrastTempls = "template/infrast";
    //constexpr static const char* CopilotCfgDirname = "copilot";
    constexpr static auto RoguelikeCfgDirname = "roguelike_copilot.json";
    constexpr static auto OcrResourceFilename = "PaddleOCR";
    constexpr static auto TilesCalcResourceFilename = "Arknights-Tile-Pos";
    constexpr static auto StageDropsCfgFilename = "stages.json";
    constexpr static auto StageDropsTempls = "template/items";
    constexpr static auto BattleDataCfgFilename = "battle_data.json";
    constexpr static auto RoguelikeShoppingCfgFilename = "roguelike_shopping.json";

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

    if (!m_roguelike_recruit_cfg_unique_ins.load(dir + RoguelikeRecruitCfgFilename)) {
        if (!m_loaded) {
            m_last_error = std::string(RoguelikeRecruitCfgFilename) + ": " + m_roguelike_recruit_cfg_unique_ins.get_last_error();
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

    //for (const auto& entry : std::filesystem::directory_iterator(dir + CopilotCfgDirname)) {
    //    if (entry.path().extension() != ".json") {
    //        continue;
    //    }
    //    if (!m_copilot_cfg_unique_ins.load(entry.path().string())) {
    //        m_last_error = entry.path().string() + " Load failed";
    //        return false;
    //    }
    //}

    if (!m_roguelike_cfg_unique_ins.load(dir + RoguelikeCfgDirname)) {
        if (!m_loaded) {
            m_last_error = std::string(RoguelikeCfgDirname) + ": " + m_roguelike_cfg_unique_ins.get_last_error();
            return false;
        }
    }
    else {
        overload = true;
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

    if (!m_stage_drops_cfg_unique_ins.load(dir + StageDropsCfgFilename)) {
        if (!m_loaded) {
            m_last_error = std::string(StageDropsCfgFilename) + ": " + m_stage_drops_cfg_unique_ins.get_last_error();
            return false;
        }
    }
    else {
        overload = true;
    }

    /* 加载模板图片资源 */
    // task所需要的模板资源
    m_templ_resource_unique_ins.set_load_required(TaskData::get_instance().get_templ_required());
    if (!m_templ_resource_unique_ins.load(dir + TemplsFilename)) {
        if (!m_loaded) {
            m_last_error = std::string(TemplsFilename) + ": " + m_templ_resource_unique_ins.get_last_error();
            return false;
        }
    }
    else {
        overload = true;
    }
    // 基建所需要的模板资源
    m_templ_resource_unique_ins.set_load_required(m_infrast_cfg_unique_ins.get_templ_required());
    if (!m_templ_resource_unique_ins.load(dir + InfrastTempls)) {
        if (!m_loaded) {
            m_last_error = std::string(InfrastTempls) + ": " + m_templ_resource_unique_ins.get_last_error();
            return false;
        }
    }
    else {
        overload = true;
    }
    // 关卡掉落物品的模板资源
    m_templ_resource_unique_ins.set_load_required(m_item_cfg_unique_ins.get_all_item_id());
    if (!m_templ_resource_unique_ins.load(dir + StageDropsTempls)) {
        if (!m_loaded) {
            m_last_error = std::string(StageDropsTempls) + ": " + m_templ_resource_unique_ins.get_last_error();
            return false;
        }
    }
    else {
        overload = true;
    }

    // 加载战斗中需要的数据
    if (!m_battle_data_cfg_unique_ins.load(dir + BattleDataCfgFilename)) {
        if (!m_loaded) {
            m_last_error = std::string(BattleDataCfgFilename) + ": " + m_battle_data_cfg_unique_ins.get_last_error();
            return false;
        }
    }
    else {
        overload = true;
    }

    // 加载肉鸽购物需要的数据
    if (!m_roguelike_shopping_cfg_unique_ins.load(dir + RoguelikeShoppingCfgFilename)) {
        if (!m_loaded) {
            m_last_error = std::string(RoguelikeShoppingCfgFilename) + ": " + m_roguelike_shopping_cfg_unique_ins.get_last_error();
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
