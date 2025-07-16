#include "BattleSpyTask.h"

#include "Utils/Ranges.hpp"
#include <chrono>
#include <future>
#include <thread>

#include "Utils/NoWarningCV.h"

#include "Config/GeneralConfig.h"
#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/CopilotConfig.h"
#include "Config/Miscellaneous/TilePack.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Algorithm.hpp"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Battle/BattlefieldMatcher.h"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"
#include <Vision/Miscellaneous/StageDropsImageAnalyzer.h>
#include <Vision/Battle/BattlefieldDetector.h>
#include <Vision/Battle/BattlefieldClassifier.h>

using namespace asst::battle;
using namespace asst::battle::copilot;

asst::BattleSpyTask::BattleSpyTask(const AsstCallback& callback, Assistant* inst, std::string_view task_chain) :
    AbstractTask(callback, inst, task_chain),
    BattleHelper(inst)
{
}

bool asst::BattleSpyTask::_run()
{
    LogTraceFunction;
    clear();

    // TODO, 识别关卡名
    /*if (!calc_tiles_info(m_stage_name)) {
        Log.error("get stage info failed");
        return false;
    }*/

    if (!update_deployment(true)) {
        Log.error("update deployment failed");
        return false;
    }

    to_group();

    while (need_to_wait_until_end() && check_in_battle() && update_deployment() && update_kills(ctrler()->get_image())) {
        // TODO: 分析技能准备情况
        //cv::Mat image = ctrler()->get_image();
        //BattlefieldDetector analyzer(image);
        //analyzer.set_object_of_interest({ .operators = true });
        //auto result_opt = analyzer.analyze();
        //for (auto& it : result_opt->operators) {
        //    auto f = image(
        //        cv::Rect(it.rect.x, it.rect.y + static_cast<int>(image.rows * -0.2), it.rect.width, it.rect.height + static_cast<int>(image.rows * 0.17)));
        //    cv::imshow("test1", f);
        //    cv::waitKey(0);
        //    auto ready = is_skill_ready(f);
        //    printf("%d", ready);
        //}

        //cv::imshow("test", analyzer.get_draw());
        //cv::waitKey(0);

        // 获取携带干员
        auto names =
            m_cur_deployment_opers | std::views::transform([](const DeploymentOper& user) { return user.name; });

        json::value value;
        value["deployment"] = std::vector<std::string> { names.begin(), names.end() };
        value["kills"] = m_kills;
        notify_action(value);
    }

    int maxTry = 5;
    for (int i = 0; i < maxTry; i++) {
        sleep(2000);
        StageDropsImageAnalyzer analyzer(ctrler()->get_image());
        // 大概率失败
        // 也有可能是因为匹配到一些关卡，然后输出 `_INVALID_`，见 `StageDrops-StageName`
        if (analyzer.analyze()) {
            auto&& [code, difficulty] = analyzer.get_stage_key();

            std::string stage_code = std::move(code);
            ranges::transform(stage_code, stage_code.begin(), [](char ch) -> char {
                return static_cast<char>(::toupper(ch));
            });

            Log.info(__FUNCTION__, "Stage Code:", stage_code, "Stars:", analyzer.get_stars());
            break;
        }
        else {
            // 几星完成作战？
            int star = analyzer.get_stars();
            auto&& [code, difficulty] = analyzer.get_stage_key();

            // 识别出内容就退出
            if (!code.empty()) {
                notify_action({ { "star", star }, { "difficulty", difficulty } });
                break;
            }
        }
    }

    return true;
}

void asst::BattleSpyTask::clear()
{
    BattleHelper::clear();

    m_oper_in_group.clear();
    m_in_bullet_time = false;
}

bool asst::BattleSpyTask::set_stage_name(const std::string& stage_name)
{
    LogTraceFunction;

    if (!BattleHelper::set_stage_name(stage_name)) {
        json::value info = basic_info_with_what("UnsupportedLevel");
        auto& details = info["details"];
        details["level"] = stage_name;
        callback(AsstMsg::SubTaskExtraInfo, info);

        return false;
    }

    m_combat_data = Copilot.get_data();

    return true;
}

void asst::BattleSpyTask::set_wait_until_end(bool wait_until_end)
{
    m_need_to_wait_until_end = wait_until_end;
}

void
    asst::BattleSpyTask::set_formation_task_ptr(std::shared_ptr<std::unordered_map<std::string, std::string>> value)
{
    m_formation_ptr = value;
}

bool asst::BattleSpyTask::to_group()
{
    std::unordered_map<std::string, std::vector<std::string>> groups;
    // 从编队任务中获取<干员-组名>映射
    if (m_formation_ptr != nullptr) {
        for (const auto& [group, oper] : *m_formation_ptr) {
            groups.emplace(oper, std::vector<std::string> { group });
        }
    }
    // 补充剩余的干员
    for (const auto& [group_name, oper_list] : get_combat_data().groups) {
        if (groups.contains(group_name)) {
            continue;
        }
        std::vector<std::string> oper_name_list;
        ranges::transform(oper_list, std::back_inserter(oper_name_list), [](const auto& oper) { return oper.name; });
        groups.emplace(group_name, std::move(oper_name_list));
    }

    std::unordered_set<std::string> char_set; // 干员集合
    for (const auto& oper : m_cur_deployment_opers) {
        char_set.emplace(oper.name);
    }

    auto result_opt = algorithm::get_char_allocation_for_each_group(groups, char_set);
    if (result_opt) {
        m_oper_in_group = *result_opt;
    }
    else {
        Log.warn("get_char_allocation_for_each_group failed");
        for (const auto& [gp, names] : groups) {
            if (names.empty()) {
                continue;
            }
            m_oper_in_group.emplace(gp, names.front());
        }
    }

    std::unordered_map<std::string, std::string> ungrouped;
    const auto& grouped_view = m_oper_in_group | views::values;
    for (const auto& name : char_set) {
        if (ranges::find(grouped_view, name) != grouped_view.end()) {
            continue;
        }
        ungrouped.emplace(name, name);
    }
    m_oper_in_group.merge(ungrouped);

    for (const auto& action : m_combat_data.actions) {
        const std::string& action_name = action.name;
        if (action_name.empty() || m_oper_in_group.contains(action_name)) {
            continue;
        }
        m_oper_in_group.emplace(action_name, action_name);
    }

    for (const auto& [group_name, oper_name] : m_oper_in_group) {
        const auto& group_it = ranges::find_if(get_combat_data().groups, [&](const OperUsageGroup& pair) {
            return pair.first == group_name;
        });
        if (group_it == get_combat_data().groups.end()) {
            Log.warn(__FUNCTION__, "Group not found in combat data: ", group_name);
            continue;
        }
        const auto& this_group = group_it->second;
        // there is a build error on macOS
        // https://github.com/MaaAssistantArknights/MaaAssistantArknights/actions/runs/3779762713/jobs/6425284487
        const std::string& oper_name_for_lambda = oper_name;
        auto iter = ranges::find_if(this_group, [&](const auto& oper) { return oper.name == oper_name_for_lambda; });
        if (iter == this_group.end()) {
            continue;
        }
        m_skill_usage.emplace(oper_name, iter->skill_usage);
        m_skill_times.emplace(oper_name, iter->skill_times);
    }

    return true;
}

void asst::BattleSpyTask::notify_action(const json::value& value)
{
    json::value info = basic_info_with_what("BattleSpyTask");
    info["details"] = value;
    
    callback(AsstMsg::SubTaskExtraInfo, info);
}

bool asst::BattleSpyTask::is_skill_ready(const cv::Mat& imageRect)
{
    BattlefieldClassifier skill_analyzer(imageRect);
    skill_analyzer.set_object_of_interest({ .skill_ready = true });
    

    //const Point& battlefield_point = target_iter->second.pos;
    skill_analyzer.set_base_point({0,0});

    return skill_analyzer.analyze()->skill_ready.ready;
}
