#include "CopilotConfig.h"

#include <meojson/json.hpp>

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "TilePack.h"
#include "Utils/Logger.hpp"

using namespace asst::battle;
using namespace asst::battle::copilot;

void asst::CopilotConfig::clear()
{
    m_data = decltype(m_data)();
}

bool asst::CopilotConfig::parse(const json::value& json)
{
    LogTraceFunction;

    clear();

    m_data.info = parse_basic_info(json);
    if (auto groups = parse_groups(json)) {
        m_data.groups = *groups;
    }
    else {
        return false;
    }
    m_data.actions = parse_actions(json);

    return true;
}

asst::battle::copilot::BasicInfo asst::CopilotConfig::parse_basic_info(const json::value& json)
{
    LogTraceFunction;

    battle::copilot::BasicInfo info;

    info.stage_name = json.at("stage_name").as_string();

    info.title = json.get("doc", "title", std::string());
    info.title_color = json.get("doc", "title_color", std::string());
    info.details = json.get("doc", "details", std::string());
    info.details_color = json.get("doc", "details_color", std::string());

    return info;
}

std::optional<asst::battle::OperUsage> asst::CopilotConfig::parse_oper_usage(const json::value& json)
{
    OperUsage oper;
    auto role = json.get("role", std::string());
    utils::tolowers(role);
    oper.role = get_role_type(role);
    oper.name = json.at("name").as_string();
    oper.skill = json.get("skill", 0);
    oper.skill_usage = static_cast<battle::SkillUsage>(json.get("skill_usage", 0));
    oper.skill_times = json.get("skill_times", 1); // 使用技能的次数，默认为 1，兼容曾经的作业

    // 兼容古早旧作业中非法的技能选择；干员查不到时沿用旧逻辑按稀有度 0 处理，不拒绝整个作业
    // 同名干员与召唤物并存时（如 “阿米娅” 与活动装置）取稀有度最高的一条，不依赖 unordered_map 遍历顺序
    std::shared_ptr<OperProps> oper_props;
    for (const auto& props : BattleData.find_opers(oper.role, oper.name)) {
        if (oper_props == nullptr || props->rarity > oper_props->rarity) {
            oper_props = props;
        }
    }
    if (!oper_props) {
        LogError << __FUNCTION__ << "| Oper" << oper.name << "with role" << enum_to_string(oper.role)
                 << "not found in BattleData.";
    }
    int rarity = oper_props ? oper_props->rarity : 0;
    if (oper.skill == 3 && rarity < 6 && (!oper_props || oper_props->id != "char_002_amiya")) {
        LogError << __FUNCTION__ << "| Oper " << oper.name << " with rarity " << rarity
                 << " cannot use skill index 3, set to 0.";
        oper.skill = 0;
    }
    else if (oper.skill == 2 && rarity < 4) {
        LogError << __FUNCTION__ << "| Oper " << oper.name << " with rarity " << rarity
                 << " cannot use skill index 2, set to 0.";
        oper.skill = 0;
    }
    else if (oper.skill == 1 && rarity < 3) {
        LogError << __FUNCTION__ << "| Oper " << oper.name << " with rarity " << rarity
                 << " cannot use skill index 1, set to 0.";
        oper.skill = 0;
    }

    int elite_require = oper.skill - 1;
    // 解析练度需求并检查非法设置
    if (auto req_opt = json.find("requirements")) {
        oper.requirements.level = req_opt->get("level", 0);
        // oper.requirements.potentiality = req_opt->get("potentiality", 0);

        if (auto skill_level_opt = req_opt->find<int>("skill_level"); !skill_level_opt) {
            oper.requirements.skill_level = 0;
        }
        else {
            oper.requirements.skill_level = *skill_level_opt;
            if (*skill_level_opt > 7) {
                elite_require = std::max(2, elite_require); // 技能专精要求精二
            }
            else if (*skill_level_opt > 4) {
                elite_require = std::max(1, elite_require);
            }
        }
        if (auto module_opt = req_opt->find<int>("module"); module_opt) {
            oper.requirements.module = *module_opt;
            if (*module_opt > 0) {
                elite_require = std::max(2, elite_require); // 模组要求精2
            }
        }
        if (auto elite_opt = req_opt->find<int>("elite"); elite_opt) {
            if (elite_require > *elite_opt) {
                LogError << __FUNCTION__ << "| Oper" << oper.name << "has higher elite requirement:" << elite_require
                         << ", but elite requirement is set to" << *elite_opt;
                return std::nullopt;
            }
            oper.requirements.elite = *elite_opt;
        }
    }

    return oper;
}

std::optional<asst::battle::copilot::OperUsageGroups> asst::CopilotConfig::parse_groups(const json::value& json)
{
    LogTraceFunction;

    battle::copilot::OperUsageGroups groups;

    if (auto opt = json.find<json::array>("opers")) {
        for (const auto& oper_info : opt.value()) {
            auto oper = parse_oper_usage(oper_info);
            if (!oper) {
                LogError << __FUNCTION__ << "| Failed to parse oper" << oper_info;
                return std::nullopt;
            }
            // 单个干员的，干员名直接作为组名
            std::string group_name = oper->name;
            groups.emplace_back(
                OperUsageGroup { std::move(group_name),
                                 oper->requirements.elite,
                                 oper->requirements.level,
                                 std::vector { std::move(*oper) } });
        }
    }

    if (auto opt = json.find<json::array>("groups")) {
        for (const auto& group_info : opt.value()) {
            std::string group_name = group_info.at("name").as_string();
            std::vector<OperUsage> oper_vec;
            int elite_min = 2;
            int level_min = 90;
            for (const auto& oper_info : group_info.at("opers").as_array()) {
                auto oper = parse_oper_usage(oper_info);
                if (!oper) {
                    LogError << __FUNCTION__ << "| Failed to parse oper" << oper_info;
                    return std::nullopt;
                }
                if (oper->requirements.elite < elite_min) {
                    elite_min = oper->requirements.elite;
                    level_min = std::min(elite_min == 1 ? 80 : 70, oper->requirements.level);
                }
                else if (oper->requirements.elite == elite_min) {
                    level_min = std::min(level_min, oper->requirements.level);
                }
                oper_vec.emplace_back(std::move(*oper));
            }
            groups.emplace_back(OperUsageGroup { std::move(group_name), elite_min, level_min, std::move(oper_vec) });
        }
    }

    return groups;
}

std::vector<asst::battle::copilot::Action> asst::CopilotConfig::parse_actions(const json::value& json)
{
    LogTraceFunction;

    std::vector<battle::copilot::Action> actions_list;

    for (const auto& action_info : json.at("actions").as_array()) {
        Action action;
        static const std::unordered_map<std::string, ActionType> ActionTypeMapping = {
            { "Deploy", ActionType::Deploy },
            { "DEPLOY", ActionType::Deploy },
            { "deploy", ActionType::Deploy },
            { "部署", ActionType::Deploy },

            { "Skill", ActionType::UseSkill },
            { "SKILL", ActionType::UseSkill },
            { "skill", ActionType::UseSkill },
            { "技能", ActionType::UseSkill },

            { "Retreat", ActionType::Retreat },
            { "RETREAT", ActionType::Retreat },
            { "retreat", ActionType::Retreat },
            { "撤退", ActionType::Retreat },

            { "SpeedUp", ActionType::SwitchSpeed },
            { "SPEEDUP", ActionType::SwitchSpeed },
            { "Speedup", ActionType::SwitchSpeed },
            { "speedup", ActionType::SwitchSpeed },
            { "二倍速", ActionType::SwitchSpeed },

            { "BulletTime", ActionType::BulletTime },
            { "BULLETTIME", ActionType::BulletTime },
            { "Bullettime", ActionType::BulletTime },
            { "bullettime", ActionType::BulletTime },
            { "子弹时间", ActionType::BulletTime },

            { "SkillUsage", ActionType::SkillUsage },
            { "SKILLUSAGE", ActionType::SkillUsage },
            { "Skillusage", ActionType::SkillUsage },
            { "skillusage", ActionType::SkillUsage },
            { "技能用法", ActionType::SkillUsage },

            { "Output", ActionType::Output },
            { "OUTPUT", ActionType::Output },
            { "output", ActionType::Output },
            { "输出", ActionType::Output },
            { "打印", ActionType::Output },

            { "SkillDaemon", ActionType::SkillDaemon },
            { "skilldaemon", ActionType::SkillDaemon },
            { "SKILLDAEMON", ActionType::SkillDaemon },
            { "Skilldaemon", ActionType::SkillDaemon },
            { "DoNothing", ActionType::SkillDaemon },
            { "摆完挂机", ActionType::SkillDaemon },
            { "开摆", ActionType::SkillDaemon },

            { "MoveCamera", ActionType::MoveCamera },
            { "movecamera", ActionType::MoveCamera },
            { "MOVECAMERA", ActionType::MoveCamera },
            { "Movecamera", ActionType::MoveCamera },
            { "移动镜头", ActionType::MoveCamera },

            { "DrawCard", ActionType::DrawCard },
            { "drawcard", ActionType::DrawCard },
            { "DRAWCARD", ActionType::DrawCard },
            { "Drawcard", ActionType::DrawCard },
            { "抽卡", ActionType::DrawCard },
            { "抽牌", ActionType::DrawCard },
            { "调配", ActionType::DrawCard },
            { "调配干员", ActionType::DrawCard },

            { "CheckIfStartOver", ActionType::CheckIfStartOver },
            { "Checkifstartover", ActionType::CheckIfStartOver },
            { "CHECKIFSTARTOVER", ActionType::CheckIfStartOver },
            { "checkifstartover", ActionType::CheckIfStartOver },
            { "检查重开", ActionType::CheckIfStartOver },

            { "ResetStopwatch", ActionType::ResetStopwatch },
            { "RESETSTOPWATCH", ActionType::ResetStopwatch },
            { "resetstopwatch", ActionType::ResetStopwatch },
            { "Resetstopwatch", ActionType::ResetStopwatch },
            { "重置全局计时器", ActionType::ResetStopwatch },
        };

        std::string type_str = action_info.get("type", "Deploy");

        if (auto iter = ActionTypeMapping.find(type_str); iter != ActionTypeMapping.end()) {
            action.type = iter->second;
        }
        else {
            Log.warn("Unknown action type:", type_str);
            continue;
        }
        action.kills = action_info.get("kills", 0);
        action.cost_changes = action_info.get("cost_changes", 0);
        action.costs = action_info.get("costs", 0);
        action.cooling = action_info.get("cooling", -1);
        auto role = action_info.get("role", std::string());
        utils::tolowers(role);
        action.role = get_role_type(role);
        action.name = action_info.get("name", std::string());

        action.location.x = action_info.get("location", 0, 0);
        action.location.y = action_info.get("location", 1, 0);
        action.direction = string_to_direction(action_info.get("direction", "Right"));

        action.modify_usage = static_cast<battle::SkillUsage>(action_info.get("skill_usage", 0));
        action.modify_times = action_info.get("skill_times", 1);
        action.pre_delay = action_info.get("pre_delay", 0);
        auto post_delay_opt = action_info.find<int>("post_delay");
        // 历史遗留字段，兼容一下
        action.post_delay = post_delay_opt ? *post_delay_opt : action_info.get("rear_delay", 0);
        action.timeout_ms = action_info.get("timeout", -1);
        action.doc = action_info.get("doc", std::string());
        action.doc_color = action_info.get("doc_color", std::string());

        if (action.type == ActionType::CheckIfStartOver) {
            if (auto tool_men = action_info.find("tool_men")) {
                action.role_counts = parse_role_counts(*tool_men);
            }
        }
        else if (action.type == ActionType::MoveCamera) {
            auto dist_arr = action_info.at("distance").as_array();
            action.distance = std::make_pair(dist_arr[0].as_double(), dist_arr[1].as_double());
        }

        // ————————————————————————————————————————————————————————————————
        // 实验性功能
        // ————————————————————————————————————————————————————————————————
        // 跳过使用未准备好的技能，主要用于关闭技能的场景 已废弃
        if (action_info.contains("skip_if_not_ready")) {
            LogWarn << "================  DEPRECATED  ================";
            LogWarn << "The field 'skip_if_not_ready' is deprecated and will be removed in future versions.";
            LogWarn << "================  DEPRECATED  ================";
            if (action_info.contains("timeout")) {
                LogError << __FUNCTION__ << "| Both 'timeout' and 'skip_if_not_ready' are setted. Ignore step";
                continue;
            }
            else {
                if (action_info.get("skip_if_not_ready", false)) {
                    action.timeout_ms = 0;
                }
            }
        }

        // 计时器
        action.elapsed_time = action_info.get("elapsed_time", 0);
        // ————————————————————————————————————————————————————————————————

        actions_list.emplace_back(std::move(action));
    }

    return actions_list;
}

asst::battle::RoleCounts asst::CopilotConfig::parse_role_counts(const json::value& json)
{
    battle::RoleCounts counts;
    for (const auto& [role_name, count] : json.as_object()) {
        auto role = get_role_type(role_name);
        if (role == Role::Unknown) {
            Log.error("Unknown role name: ", role_name);
            throw std::runtime_error("Unknown role name: " + role_name);
        }
        counts.emplace(role, count.as_integer());
    }
    return counts;
}

asst::battle::DeployDirection asst::CopilotConfig::string_to_direction(const std::string& str)
{
    // clang-format off
    static const std::unordered_map<std::string, DeployDirection> DeployDirectionMapping = {
        { "Right", DeployDirection::Right }, { "RIGHT", DeployDirection::Right },
        { "right", DeployDirection::Right }, { "右", DeployDirection::Right },

        { "Left", DeployDirection::Left },   { "LEFT", DeployDirection::Left },
        { "left", DeployDirection::Left },   { "左", DeployDirection::Left },

        { "Up", DeployDirection::Up },       { "UP", DeployDirection::Up },
        { "up", DeployDirection::Up },       { "上", DeployDirection::Up },

        { "Down", DeployDirection::Down },   { "DOWN", DeployDirection::Down },
        { "down", DeployDirection::Down },   { "下", DeployDirection::Down },

        { "None", DeployDirection::None },   { "NONE", DeployDirection::None },
        { "none", DeployDirection::None },   { "无", DeployDirection::None },
    }; // clang-format on

    if (auto iter = DeployDirectionMapping.find(str); iter != DeployDirectionMapping.end()) {
        return iter->second;
    }
    else {
        return DeployDirection::Right;
    }
}
