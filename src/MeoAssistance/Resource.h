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

#pragma once

#include "AbstractResource.h"

#include <memory>
#include <utility>

#include <opencv2/opencv.hpp>

#include "GeneralConfiger.h"
#include "InfrastConfiger.h"
#include "ItemConfiger.h"
#include "OcrPack.h"
#include "PenguinPack.h"
#include "RecruitConfiger.h"
#include "TaskData.h"
#include "TemplResource.h"
#include "UserConfiger.h"

namespace asst {
    class Resource : public AbstractResource {
    public:
        virtual ~Resource() = default;

        static Resource& get_instance() {
            static Resource unique_instance;
            return unique_instance;
        }

        virtual bool load(const std::string& dir) override;

        TemplResource& templ() noexcept {
            return m_templ_resource_unique_ins;
        }
        GeneralConfiger& cfg() noexcept {
            return m_general_cfg_unique_ins;
        }
        TaskData& task() noexcept {
            return m_task_data_unique_ins;
        }
        RecruitConfiger& recruit() noexcept {
            return m_recruit_cfg_unique_ins;
        }
        ItemConfiger& item() noexcept {
            return m_item_cfg_unique_ins;
        }
        InfrastConfiger& infrast() noexcept {
            return m_infrast_cfg_unique_ins;
        }
        UserConfiger& user() noexcept {
            return m_user_cfg_unique_ins;
        }
        OcrPack& ocr() noexcept {
            return m_ocr_pack_unique_ins;
        }
        PenguinPack& penguin() noexcept {
            return m_penguin_pack_unique_ins;
        }

        const TemplResource& templ() const noexcept {
            return m_templ_resource_unique_ins;
        }
        const GeneralConfiger& cfg() const noexcept {
            return m_general_cfg_unique_ins;
        }
        const TaskData& task() const noexcept {
            return m_task_data_unique_ins;
        }
        const RecruitConfiger& recruit() const noexcept {
            return m_recruit_cfg_unique_ins;
        }
        const ItemConfiger& item() const noexcept {
            return m_item_cfg_unique_ins;
        }
        const InfrastConfiger& infrast() const noexcept {
            return m_infrast_cfg_unique_ins;
        }
        const UserConfiger& user() const noexcept {
            return m_user_cfg_unique_ins;
        }
        const OcrPack& ocr() const noexcept {
            return m_ocr_pack_unique_ins;
        }
        const PenguinPack& penguin() const noexcept {
            return m_penguin_pack_unique_ins;
        }

        Resource& operator=(const Resource&) = delete;
        Resource& operator=(Resource&&) noexcept = delete;

    private:
        Resource() = default;

        TemplResource m_templ_resource_unique_ins;
        GeneralConfiger m_general_cfg_unique_ins;
        TaskData m_task_data_unique_ins;
        RecruitConfiger m_recruit_cfg_unique_ins;
        ItemConfiger m_item_cfg_unique_ins;
        InfrastConfiger m_infrast_cfg_unique_ins;
        UserConfiger m_user_cfg_unique_ins;
        OcrPack m_ocr_pack_unique_ins;
        PenguinPack m_penguin_pack_unique_ins;
    };

    static auto& resource = Resource::get_instance();
}
