#pragma once

#include "Config/AbstractConfig.h"

#include <vector>

#include "Common/AsstBattleDef.h"
#include "Common/AsstTypes.h"
#include "Task/Roguelike/RoguelikeConfig.h"

namespace asst
{
class RoguelikeStageEncounterConfig final : public SingletonHolder<RoguelikeStageEncounterConfig>, public AbstractConfig
{
public:
    virtual ~RoguelikeStageEncounterConfig() override = default;

    const auto& get_events(const std::string& theme, const RoguelikeMode& mode) const noexcept
    {
        std::pair<std::string, int> key = std::make_pair(theme, static_cast<int>(mode));
        if (!m_events.contains(key)) {
            key.second = -1;
        }
        return m_events.at(key);
    }

    const auto& get_event_names(const std::string& theme) const noexcept { return m_event_names.at(theme); }

    enum class ComparisonType
    {
        GreaterThan,
        LessThan,
        Equal,
        None,        // 没有配置
        Unsupported, // 配置错误或其他
    };

    struct Vision
    {
        std::string value;
        ComparisonType type = ComparisonType::None;
    };

    struct ChoiceRequire
    {
        std::string name;
        int choose = -1;
        Vision vision; // 现在只有Vision解析，之后要改成requirements且支持多个条件判断同一个选择
    };

    struct RoguelikeEvent
    {
        std::string name;
        int option_num = 0;
        int default_choose = 0;
        std::vector<ChoiceRequire> choice_require;
        std::string next_event;

        std::vector<std::pair<int, int>>
            fallback_choices; // 备用选项，格式为 (选项数量，选择的选项)，有些事件的选项数量可变
    };

private:
    virtual bool parse(const json::value& json) override;

    static ComparisonType parse_comparison_type(const std::string& type_str);

    std::unordered_map<
        std::pair<std::string, int>,
        std::unordered_map<std::string, RoguelikeEvent>,
        std::pair_hash<std::string, int>>
        m_events;
    std::unordered_map<std::string, std::vector<std::string>> m_event_names;
};

inline static auto& RoguelikeStageEncounter = RoguelikeStageEncounterConfig::get_instance();
}
