#pragma once
#include "AbstractTaskPlugin.h"

#include <unordered_map>

namespace asst
{
    class RoguelikeSkillSelectionTaskPlugin final : public AbstractTaskPlugin
    {
    public:
        using SkillMap = std::unordered_map<std::string, int>;
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeSkillSelectionTaskPlugin() = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

        void set_skill_map(SkillMap skill_mapping);

    protected:
        virtual bool _run() override;

        SkillMap m_skill_mapping;
    };
}
