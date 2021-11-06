/*
    MeoAssistance (CoreLib) - A part of the MeoAssistance-Arknight project
    Copyright (C) 2021 MistEO and Contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Resource.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include <json.h>

#include "AsstDef.h"

bool asst::Resource::load(const std::string& dir) {
    constexpr static const char* TemplsFilename = "template";
    constexpr static const char* GeneralCfgFilename = "config.json";
    constexpr static const char* TaskDataFilename = "tasks.json";
    constexpr static const char* RecruitCfgFilename = "recruit.json";
    constexpr static const char* ItemCfgFilename = "item_index.json";
    constexpr static const char* InfrastCfgFilename = "infrast.json";
    constexpr static const char* UserCfgFilename = "..\\user.json";
    constexpr static const char* OcrResourceFilename = "OcrLiteOnnx";
    constexpr static const char* PenguinResourceFilename = "penguin-stats-recognize";

    /* 加载各个Json配置文件 */
    if (!m_general_cfg_unique_ins.load(dir + GeneralCfgFilename)) {
        m_last_error = m_general_cfg_unique_ins.get_last_error();
        return false;
    }
    if (!m_task_data_unique_ins.load(dir + TaskDataFilename)) {
        m_last_error = m_task_data_unique_ins.get_last_error();
        return false;
    }

    if (!m_recruit_cfg_unique_ins.load(dir + RecruitCfgFilename)) {
        m_last_error = m_recruit_cfg_unique_ins.get_last_error();
        return false;
    }
    if (!m_item_cfg_unique_ins.load(dir + ItemCfgFilename)) {
        m_last_error = m_item_cfg_unique_ins.get_last_error();
        return false;
    }
    if (!m_infrast_cfg_unique_ins.load(dir + InfrastCfgFilename)) {
        m_last_error = m_infrast_cfg_unique_ins.get_last_error();
        return false;
    }

    if (!m_user_cfg_unique_ins.load(dir + UserCfgFilename)) {
        m_last_error = m_user_cfg_unique_ins.get_last_error();
        return false;
    }
    /* 根据用户配置，覆盖原有的部分配置 */
    for (const auto& [name, path] : m_user_cfg_unique_ins.get_emulator_path()) {
        m_general_cfg_unique_ins.set_emulator_path(name, path);
    }
    const auto& opt = m_general_cfg_unique_ins.get_options();

    /* 加载模板图片资源 */
    // task所需要的模板资源
    m_templ_resource_unique_ins.append_load_required(m_task_data_unique_ins.get_templ_required());
    // 基建所需要的模板资源
    m_templ_resource_unique_ins.append_load_required(m_infrast_cfg_unique_ins.get_templ_required());

    if (!m_templ_resource_unique_ins.load(dir + TemplsFilename)) {
        m_last_error = m_templ_resource_unique_ins.get_last_error();
        return false;
    }

    /* 加载OCR库所需要的资源 */
    m_ocr_pack_unique_ins.set_param(opt.ocr_gpu_index, opt.ocr_thread_number);
    if (!m_ocr_pack_unique_ins.load(dir + OcrResourceFilename)) {
        m_last_error = m_ocr_pack_unique_ins.get_last_error();
        return false;
    }
    /* 加载企鹅数据识别库所需要的资源 */
    m_penguin_pack_unique_ins.set_language(opt.penguin_report_server);
    if (!m_penguin_pack_unique_ins.load(dir + PenguinResourceFilename)) {
        m_last_error = m_penguin_pack_unique_ins.get_last_error();
        return false;
    }

    return true;
}
