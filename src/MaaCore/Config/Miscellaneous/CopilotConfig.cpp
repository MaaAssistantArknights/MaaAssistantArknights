#include "CopilotConfig.h"

#include <meojson/json.hpp>

#include "TilePack.h"
#include "Utils/Logger.hpp"
#include "Utils/NoWarningCPR.h"

using namespace asst::battle;
using namespace asst::battle::copilot;

bool asst::CopilotConfig::parse_magic_code(const std::string& copilot_magic_code)
{
    if (copilot_magic_code.empty()) {
        Log.error("copilot_magic_code is empty");
        return false;
    }

    cpr::Response response =
        cpr::Get(cpr::Url("https://prts.maa.plus/copilot/get/" + copilot_magic_code), cpr::Timeout { 10000 });

    if (response.status_code != 200) {
        Log.error("copilot_magic_code request failed");
        return false;
    }

    auto json = json::parse(response.text);

    if (json && json->contains("status_code") && json->at("status_code").as_integer() == 200) {
        if (json->contains("data") && json->at("data").contains("content")) {
            auto content_str = json->at("data").at("content").as_string();
            auto content = json::parse(content_str);
            if (content) {
                return parse(*content);
            }
        }
    }

    Log.error("using copilot_code failed, response:", response.text);
    return false;
}

void asst::CopilotConfig::clear()
{
    m_data = decltype(m_data)();
}

bool asst::CopilotConfig::parse(const json::value& json)
{
    LogTraceFunction;

    clear();

    m_data.info = parse_basic_info(json);
    m_data.groups = parse_groups(json);
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

asst::battle::copilot::OperUsageGroups asst::CopilotConfig::parse_groups(const json::value& json)
{
    LogTraceFunction;

    battle::copilot::OperUsageGroups groups;

    if (auto opt = json.find<json::array>("groups")) {
        for (const auto& group_info : opt.value()) {
            std::string group_name = group_info.at("name").as_string();
            std::vector<OperUsage> oper_vec;
            for (const auto& oper_info : group_info.at("opers").as_array()) {
                OperUsage oper;
                oper.name = oper_info.at("name").as_string();
                oper.skill = oper_info.get("skill", 1);
                oper.skill_usage = static_cast<battle::SkillUsage>(oper_info.get("skill_usage", 0));
                oper.skill_times = oper_info.get("skill_times", 1); // 使用技能的次数，默认为 1，兼容曾经的作业
                oper_vec.emplace_back(std::move(oper));
            }
            groups.emplace(std::move(group_name), std::move(oper_vec));
        }
    }

    if (auto opt = json.find<json::array>("opers")) {
        for (const auto& oper_info : opt.value()) {
            OperUsage oper;
            oper.name = oper_info.at("name").as_string();
            oper.skill = oper_info.get("skill", 1);
            oper.skill_usage = static_cast<battle::SkillUsage>(oper_info.get("skill_usage", 0));
            oper.skill_times = oper_info.get("skill_times", 1); // 使用技能的次数，默认为 1，兼容曾经的作业

            // 单个干员的，干员名直接作为组名
            std::string group_name = oper.name;
            groups.emplace(std::move(group_name), std::vector { std::move(oper) });
        }
    }

    return groups;
}

TriggerInfo asst::CopilotConfig::parse_trigger(const json::value& json)
{
    TriggerInfo trigger;

    trigger.kills = json.get("kills", TriggerInfo::DEACTIVE_KILLS);
    trigger.costs = json.get("costs", TriggerInfo::DEACTIVE_COST);
    trigger.cost_changes = json.get("cost_changes", TriggerInfo::DEACTIVE_COUNT);
    trigger.cooling = json.get("cooling", TriggerInfo::DEACTIVE_COOLING);
    trigger.count = json.get("count", TriggerInfo::DEACTIVE_COUNT);

    if (auto category = json.find("category")) {
        trigger.category = TriggerInfo::loadCategoryFrom(category.value().as_string());
    }

    return trigger;
}

bool asst::CopilotConfig::parse_action(const json::value& action_info, asst::battle::copilot::Action* _Out)
{
    LogTraceFunction;

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

        { "SkillUsage", ActionType::SkillUsage },
        { "SKILLUSAGE", ActionType::SkillUsage },
        { "Skillusage", ActionType::SkillUsage },
        { "skillusage", ActionType::SkillUsage },
        { "技能用法", ActionType::SkillUsage },

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

        { "Loop", ActionType::Loop },
        { "loop", ActionType::Loop },
        { "LOOP", ActionType::Loop },
        { "循环", ActionType::Loop },

        { "Case", ActionType::Case },
        { "case", ActionType::Case },
        { "CASE", ActionType::Case },
        { "派发", ActionType::Case },
        { "干员派发", ActionType::Case },

        { "Check", ActionType::Check },
        { "check", ActionType::Check },
        { "CHECK", ActionType::Check },
        { "检查", ActionType::Check },
        { "分支", ActionType::Check },

        { "Until", ActionType::Until },
        { "until", ActionType::Until },
        { "UNTIL", ActionType::Until },
        { "直到", ActionType::Until },
    };

    auto& action = (*_Out);

    std::string type_str = action_info.get("type", "Deploy");

    if (auto iter = ActionTypeMapping.find(type_str); iter != ActionTypeMapping.end()) {
        action.type = iter->second;
    }
    else {
        Log.warn("Unknown action type:", type_str);
        return false;
    }
    // 解析锚点编码，可选
    action.point_code = action_info.get("point_code", std::string());

    // 解析动作触发信息
    action.trigger = parse_trigger(action_info);

    // 解析前置条件满足之后的前后时延
    action.delay.pre_delay = action_info.get("pre_delay", 0);
    auto post_delay_opt = action_info.find<int>("post_delay");
    action.delay.post_delay = post_delay_opt ? *post_delay_opt : action_info.get("rear_delay", 0);
    // 历史遗留字段，兼容一下
    action.delay.time_out = action_info.get("timeout", INT_MAX);

    // 解析行动的相关附加文本
    action.text.doc = action_info.get("doc", std::string());
    action.text.doc_color = action_info.get("doc_color", std::string());

    // 根据动作的类型解析载荷
    switch (action.type) {
    case ActionType::Deploy:
    case ActionType::UseSkill:
    case ActionType::Retreat:
    case ActionType::BulletTime:
    case ActionType::SkillUsage: {
        auto& avatar = action.payload.emplace<AvatarInfo>();
        avatar.name = action_info.get("name", std::string());
        avatar.location.x = action_info.get("location", 0, 0);
        avatar.location.y = action_info.get("location", 1, 0);
        avatar.direction = string_to_direction(action_info.get("direction", "Right"));

        avatar.modify_usage = static_cast<battle::SkillUsage>(action_info.get("skill_usage", 0));
        avatar.modify_times = action_info.get("skill_times", 1);
    } break;
    case ActionType::CheckIfStartOver: {
        auto& info = action.payload.emplace<CheckIfStartOverInfo>();

        info.name = action_info.get("name", std::string());
        if (auto tool_men = action_info.find("tool_men")) {
            info.role_counts = parse_role_counts(*tool_men);
        }
    } break;
    case ActionType::MoveCamera: {
        auto dist_arr = action_info.at("distance").as_array();
        action.payload.emplace<MoveCameraInfo>(std::make_pair(dist_arr[0].as_double(), dist_arr[1].as_double()));
    } break;
    case ActionType::Loop: {
        auto& loop = action.payload.emplace<LoopInfo>();

        // 必选字段
        loop.end_info = parse_trigger(action_info.at("end"));
        if (loop.end_info.category == TriggerInfo::Category::None) {
            // 默认使用all策略，表示只有全部满足才算生效
            loop.end_info.category = TriggerInfo::Category::All;
        }

        // 可选字段
        if (auto t = action_info.find("continue")) {
            loop.continue_info = parse_trigger(t.value());

            if (loop.continue_info.category == TriggerInfo::Category::None) {
                // 默认使用all策略，表示只有全部满足才算生效
                loop.continue_info.category = TriggerInfo::Category::All;
            }
        }

        // 可选字段
        if (auto t = action_info.find("break")) {
            loop.break_info = parse_trigger(t.value());

            if (loop.break_info.category == TriggerInfo::Category::None) {
                // 默认使用all策略，表示只有全部满足才算生效
                loop.break_info.category = TriggerInfo::Category::All;
            }
        }

        // 可选字段
        if (auto t = action_info.find("loop_actions")) {
            loop.loop_actions = parse_actions_ptr(t.value());
        }

    } break;
    case ActionType::Case: {
        auto& case_info = action.payload.emplace<CaseInfo>();

        // 必选字段
        case_info.group_select = action_info.at("select").as_string();

        // 可选字段
        if (auto t = action_info.find("dispatch_actions")) {
            for (auto const& [name, batch] : t.value().as_object()) {
                case_info.dispatch_actions.emplace(name, parse_actions_ptr(batch));
            }
        }

        // 可选字段
        if (auto t = action_info.find("default_action")) {
            case_info.default_action = parse_actions_ptr(t.value());
        }
    } break;
    case ActionType::Check: {
        auto& check = action.payload.emplace<CheckInfo>();

        // 必选字段
        check.condition_info = parse_trigger(action_info.at("condition"));

        if (check.condition_info.category == TriggerInfo::Category::None) {
            // 默认使用all策略，表示只有全部满足才算生效
            check.condition_info.category = TriggerInfo::Category::All;
        }

        // 可选字段
        if (auto t = action_info.find("then_actions")) {
            check.then_actions = parse_actions_ptr(t.value());
        }

        // 可选字段
        if (auto t = action_info.find("else_actions")) {
            check.else_actions = parse_actions_ptr(t.value());
        }
    } break;
    case ActionType::Until: {
        auto& until = action.payload.emplace<UntilInfo>();

        // 必选字段
        until.category = TriggerInfo::loadCategoryFrom(action_info.at("category").as_string());
        switch (until.category) {
        case TriggerInfo::Category::Any:
        case TriggerInfo::Category::All:
        case TriggerInfo::Category::Succ:
            break;
        default:
            until.category = TriggerInfo::Category::All;
            break;
        }

        // 必选字段，缺省值为0
        action.trigger.count = action_info.get("limit", 0);

        // 可选字段
        if (auto t = action_info.find("candidate_actions")) {
            until.candidate = parse_actions_ptr(t.value());
        }
    } break;
    case ActionType::SwitchSpeed:
    case ActionType::Output:
    case ActionType::SkillDaemon:
    case ActionType::DrawCard:
    case ActionType::SavePoint:
    case ActionType::SyncPoint:
    case ActionType::CheckPoint:
        [[fallthrough]];
    default:
        break;
    }

    return true;
}

std::vector<asst::battle::copilot::Action> asst::CopilotConfig::parse_actions(const json::value& json)
{
    LogTraceFunction;

    std::vector<battle::copilot::Action> actions_list;

    for (const auto& action_info : json.at("actions").as_array()) {
        battle::copilot::Action action;
        if (!parse_action(action_info, &action)) {
            continue;
        }

        actions_list.emplace_back(std::move(action));
    }

    return actions_list;
}

std::vector<asst::battle::copilot::ActionPtr> asst::CopilotConfig::parse_actions_ptr(const json::value& json)
{
    LogTraceFunction;

    std::vector<battle::copilot::ActionPtr> actions_list;

    for (const auto& action_info : json.as_array()) {
        battle::copilot::ActionPtr action = std::make_shared<battle::copilot::Action>();
        if (!parse_action(action_info, action.get())) {
            continue;
        }

        actions_list.emplace_back(action);
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
    };

    if (auto iter = DeployDirectionMapping.find(str); iter != DeployDirectionMapping.end()) {
        return iter->second;
    }
    else {
        return DeployDirection::Right;
    }
}
