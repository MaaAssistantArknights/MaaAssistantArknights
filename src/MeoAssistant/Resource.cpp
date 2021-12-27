#include "Resource.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include <meojson/json.h>

#include "AsstDef.h"

bool asst::Resource::load(const std::string& dir)
{
    constexpr static const char* TemplsFilename = "template";
    constexpr static const char* GeneralCfgFilename = "config.json";
    constexpr static const char* TaskDataFilename = "tasks.json";
    constexpr static const char* RecruitCfgFilename = "recruit.json";
    constexpr static const char* ItemCfgFilename = "item_index.json";
    constexpr static const char* InfrastCfgFilename = "infrast.json";
    constexpr static const char* UserCfgFilename = "..\\user.json";
    constexpr static const char* OcrResourceFilename = "PaddleOCR";
    constexpr static const char* PenguinResourceFilename = "penguin-stats-recognize";

    /* 加载各个Json配置文件 */
    if (!m_general_cfg_unique_ins.load(dir + GeneralCfgFilename)) {
        m_last_error = std::string(GeneralCfgFilename) + ": " + m_general_cfg_unique_ins.get_last_error();
        return false;
    }
    if (!task.load(dir + TaskDataFilename)) {
        m_last_error = std::string(TaskDataFilename) + ": " + task.get_last_error();
        return false;
    }

    if (!m_recruit_cfg_unique_ins.load(dir + RecruitCfgFilename)) {
        m_last_error = std::string(RecruitCfgFilename) + ": " + m_recruit_cfg_unique_ins.get_last_error();
        return false;
    }
    if (!m_item_cfg_unique_ins.load(dir + ItemCfgFilename)) {
        m_last_error = std::string(ItemCfgFilename) + ": " + m_item_cfg_unique_ins.get_last_error();
        return false;
    }
    if (!m_infrast_cfg_unique_ins.load(dir + InfrastCfgFilename)) {
        m_last_error = std::string(InfrastCfgFilename) + ": " + m_infrast_cfg_unique_ins.get_last_error();
        return false;
    }

    if (!m_user_cfg_unique_ins.load(dir + UserCfgFilename)) {
        m_last_error = std::string(UserCfgFilename) + ": " + m_user_cfg_unique_ins.get_last_error();
        return false;
    }
    /* 根据用户配置，覆盖原有的部分配置 */
    for (const auto& [name, path] : m_user_cfg_unique_ins.get_emulator_path()) {
        m_general_cfg_unique_ins.set_emulator_path(name, path);
    }
    const auto& opt = m_general_cfg_unique_ins.get_options();

    /* 加载模板图片资源 */
    // task所需要的模板资源
    m_templ_resource_unique_ins.append_load_required(task.get_templ_required());
    // 基建所需要的模板资源
    m_templ_resource_unique_ins.append_load_required(m_infrast_cfg_unique_ins.get_templ_required());

    if (!m_templ_resource_unique_ins.load(dir + TemplsFilename)) {
        m_last_error = std::string(TemplsFilename) + ": " + m_templ_resource_unique_ins.get_last_error();
        return false;
    }

    /* 加载OCR库所需要的资源 */
    //m_ocr_pack_unique_ins.set_param(opt.ocr_gpu_index, opt.ocr_thread_number);
    if (!m_ocr_pack_unique_ins.load(dir + OcrResourceFilename)) {
        m_last_error = std::string(OcrResourceFilename) + ": " + m_ocr_pack_unique_ins.get_last_error();
        return false;
    }
    /* 加载企鹅数据识别库所需要的资源 */
    m_penguin_pack_unique_ins.set_language(opt.penguin_report.server);
    if (!m_penguin_pack_unique_ins.load(dir + PenguinResourceFilename)) {
        m_last_error = std::string(PenguinResourceFilename) + ": " + m_penguin_pack_unique_ins.get_last_error();
        return false;
    }

    return true;
}
