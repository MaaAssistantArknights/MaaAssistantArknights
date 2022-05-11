#pragma once

#include "AbstractResource.h"

#include <memory>
#include <utility>

#include "CombatRecruitConfiger.h"
#include "GeneralConfiger.h"
#include "InfrastConfiger.h"
#include "ItemConfiger.h"
#include "OcrPack.h"
#include "PenguinPack.h"
#include "RecruitConfiger.h"
#include "TaskData.h"
#include "TemplResource.h"
#include "TilePack.h"
#include "CopilotConfiger.h"
#include "RoguelikeCopilotConfiger.h"

namespace asst
{
    class Resource : public AbstractResource
    {
    public:
        virtual ~Resource() = default;

        static Resource& get_instance()
        {
            static Resource unique_instance;
            return unique_instance;
        }

        virtual bool load(const std::string& dir) override;

        TemplResource& templ() noexcept
        {
            return m_templ_resource_unique_ins;
        }
        GeneralConfiger& cfg() noexcept
        {
            return m_general_cfg_unique_ins;
        }
        RecruitConfiger& recruit() noexcept
        {
            return m_recruit_cfg_unique_ins;
        }
        CombatRecruitConfiger& combatrecruit() noexcept
        {
            return m_combatrecruit_cfg_unique_ins;
        }
        ItemConfiger& item() noexcept
        {
            return m_item_cfg_unique_ins;
        }
        InfrastConfiger& infrast() noexcept
        {
            return m_infrast_cfg_unique_ins;
        }
        CopilotConfiger& copilot() noexcept
        {
            return m_copilot_cfg_unique_ins;
        }
        RoguelikeCopilotConfiger& roguelike() noexcept
        {
            return m_roguelike_cfg_unique_ins;
        }
        OcrPack& ocr() noexcept
        {
            return m_ocr_pack_unique_ins;
        }
        PenguinPack& penguin() noexcept
        {
            return m_penguin_pack_unique_ins;
        }
        TilePack& tile() noexcept
        {
            return m_tile_pack_unique_ins;
        }

        const TemplResource& templ() const noexcept
        {
            return m_templ_resource_unique_ins;
        }
        const GeneralConfiger& cfg() const noexcept
        {
            return m_general_cfg_unique_ins;
        }
        const RecruitConfiger& recruit() const noexcept
        {
            return m_recruit_cfg_unique_ins;
        }
        const CombatRecruitConfiger& combatrecruit() const noexcept
        {
            return m_combatrecruit_cfg_unique_ins;
        }
        const ItemConfiger& item() const noexcept
        {
            return m_item_cfg_unique_ins;
        }
        const InfrastConfiger& infrast() const noexcept
        {
            return m_infrast_cfg_unique_ins;
        }
        const CopilotConfiger& copilot() const noexcept
        {
            return m_copilot_cfg_unique_ins;
        }
        const RoguelikeCopilotConfiger& roguelike() const noexcept
        {
            return m_roguelike_cfg_unique_ins;
        }
        const OcrPack& ocr() const noexcept
        {
            return m_ocr_pack_unique_ins;
        }
        const PenguinPack& penguin() const noexcept
        {
            return m_penguin_pack_unique_ins;
        }
        const TilePack& tile() const noexcept
        {
            return m_tile_pack_unique_ins;
        }

        Resource& operator=(const Resource&) = delete;
        Resource& operator=(Resource&&) noexcept = delete;

    private:
        Resource() = default;

        TemplResource m_templ_resource_unique_ins;
        GeneralConfiger m_general_cfg_unique_ins;
        RecruitConfiger m_recruit_cfg_unique_ins;
        CombatRecruitConfiger m_combatrecruit_cfg_unique_ins;
        CopilotConfiger m_copilot_cfg_unique_ins;
        RoguelikeCopilotConfiger m_roguelike_cfg_unique_ins;
        ItemConfiger m_item_cfg_unique_ins;
        InfrastConfiger m_infrast_cfg_unique_ins;
        OcrPack m_ocr_pack_unique_ins;
        PenguinPack m_penguin_pack_unique_ins;
        TilePack m_tile_pack_unique_ins;

        bool m_loaded = false;
    };

    //static auto& resource = Resource::get_instance();
#define Resrc Resource::get_instance()
}
