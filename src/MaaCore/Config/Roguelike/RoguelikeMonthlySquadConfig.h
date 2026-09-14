#pragma once

#include "Config/AbstractConfig.h"

#include <optional>
#include <string>
#include <unordered_map>

#include "MaaUtils/SingletonHolder.hpp"

namespace asst
{
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

struct MonthlySquadTask
{
    MonthlySquadTaskType type = MonthlySquadTaskType::ReachThirdFloor;
    std::string oper_name;
    std::string summon_name;
    int required_count = 0;
    int completed_count = 0;
    std::optional<MonthlySquadSkill> skill;

    std::string theme;
    std::string squad_key;
};

class RoguelikeMonthlySquadConfig final :
    public MAA_NS::SingletonHolder<RoguelikeMonthlySquadConfig>,
    public AbstractConfig
{
public:
    virtual ~RoguelikeMonthlySquadConfig() override = default;

    std::optional<MonthlySquadTask> get_task(
        const std::string& theme,
        const std::optional<int>& squad_index) const;

protected:
    virtual bool parse(const json::value& json) override;

private:
    std::unordered_map<std::string, std::unordered_map<std::string, MonthlySquadTask>> m_tasks;
};

inline static auto& RoguelikeMonthlySquad = RoguelikeMonthlySquadConfig::get_instance();
} // namespace asst
