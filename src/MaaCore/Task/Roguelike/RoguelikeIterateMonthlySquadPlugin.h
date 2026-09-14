#pragma once

#include <optional>

#include "AbstractRoguelikeTaskPlugin.h"

namespace asst
{

class RoguelikeIterateMonthlySquadPlugin : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeIterateMonthlySquadPlugin() override = default;
    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual bool load_params(const json::value& params) override;

protected:
    virtual bool _run() override;

private:
    enum class MonthlySquadTaskType
    {
        ReachThirdFloor,
        DeployOperator,
        DeployOperatorSummon,
        UseOperatorSkill,
    };

    enum class MonthlySquadSkill
    {
        Skill1 = 1,
        Skill2 = 2,
        Skill3 = 3,
    };

    // 月度小队数量
    std::unordered_map<std::string, int> monthlySquadCount = { { "Phantom", 8 },
                                                               { "Mizuki", 8 },
                                                               { "Sami", 8 },
                                                               { "Sarkaz", 8 },
                                                               { "JieGarden", 8 } };
    bool m_checkComms;
    bool m_completed;
    bool m_iterateMS;
    std::optional<int> m_monthly_squad_index;
    std::optional<MonthlySquadTaskType> m_monthly_squad_task_type;
    std::optional<MonthlySquadSkill> m_monthly_squad_skill;
    std::optional<int> recognize_monthly_squad_index() const;
    virtual bool try_task(const char*) const;
};

}
