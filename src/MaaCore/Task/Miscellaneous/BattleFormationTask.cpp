#include "BattleFormationTask.h"

#include "Utils/Ranges.hpp"

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/CopilotConfig.h"
#include "Config/Miscellaneous/SSSCopilotConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Vision/TemplDetOCRer.h"

void asst::BattleFormationTask::append_additional_formation(AdditionalFormation formation)
{
    m_additional.emplace_back(std::move(formation));
}

void asst::BattleFormationTask::set_support_unit_name(std::string name)
{
    m_support_unit_name = std::move(name);
}

void asst::BattleFormationTask::set_data_resource(DataResource resource)
{
    m_data_resource = resource;
}

bool asst::BattleFormationTask::_run()
{
    LogTraceFunction;

    if (!parse_formation()) {
        return false;
    }

    if (!enter_selection_page()) {
        return false;
    }

    for (auto& [role, oper_groups] : m_formation) {
        click_role_table(role);
        bool has_error = false;
        int swipe_times = 0;
        while (!need_exit()) {
            if (select_opers_in_cur_page(oper_groups)) {
                has_error = false;
                if (oper_groups.empty()) {
                    break;
                }
                swipe_page();
                ++swipe_times;
            }
            else if (has_error) {
                swipe_to_the_left(swipe_times);
                // reset page
                click_role_table(role == battle::Role::Unknown ? battle::Role::Pioneer : battle::Role::Unknown);
                click_role_table(role);
                swipe_to_the_left(swipe_times);
                swipe_times = 0;
                has_error = false;
            }
            else {
                has_error = true;
                swipe_to_the_left(swipe_times);
                swipe_times = 0;
            }
        }
    }

    add_additional();

    confirm_selection();

    // 借一个随机助战
    if (m_support_unit_name == "_RANDOM_") {
        if (!select_random_support_unit()) {
            return false;
        }
    }

    return true;
}

bool asst::BattleFormationTask::add_additional()
{
    LogTraceFunction;

    if (m_additional.empty()) {
        return false;
    }

    for (const auto& addition : m_additional) {
        std::string filter_name;
        switch (addition.filter) {
        case Filter::Cost:
            filter_name = "BattleQuickFormationFilter-Cost";
            break;
        case Filter::Trust:
            // TODO
            break;
        default:
            break;
        }
        if (!filter_name.empty()) {
            ProcessTask(*this, { "BattleQuickFormationFilter" }).run();
            ProcessTask(*this, { filter_name }).run();
            if (addition.double_click_filter) {
                ProcessTask(*this, { filter_name }).run();
            }
            ProcessTask(*this, { "BattleQuickFormationFilterClose" }).run();
        }
        for (const auto& [role, number] : addition.role_counts) {
            // unknown role means "all"
            click_role_table(role);

            auto opers_result = analyzer_opers();

            // TODO 这里要识别一下干员之前有没有被选中过
            for (size_t i = 0; i < static_cast<size_t>(number) && i < opers_result.size(); ++i) {
                const auto& oper = opers_result.at(i);
                ctrler()->click(oper.rect);
            }
        }
    }

    return true;
}

bool asst::BattleFormationTask::select_random_support_unit()
{
    return ProcessTask(*this, { "BattleSupportUnitFormation" }).run();
}

std::vector<asst::TextRect> asst::BattleFormationTask::analyzer_opers()
{
    auto formation_task_ptr = Task.get("BattleQuickFormationOCR");
    auto image = ctrler()->get_image();
    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");
    std::vector<TemplDetOCRer::Result> opers_result;

    for (int i = 0; i < 8; ++i) {
        std::string task_name = "BattleQuickFormation-OperNameFlag" + std::to_string(i);

        TemplDetOCRer name_analyzer(image);
        name_analyzer.set_task_info(task_name, "BattleQuickFormationOCR");
        name_analyzer.set_replace(ocr_replace->replace_map, ocr_replace->replace_full);
        auto cur_opt = name_analyzer.analyze();
        if (!cur_opt) {
            continue;
        }
        for (auto& res : *cur_opt) {
            constexpr int kMinDistance = 5;
            auto find_it = ranges::find_if(opers_result, [&res](const TemplDetOCRer::Result& pre) {
                return std::abs(pre.flag_rect.x - res.flag_rect.x) < kMinDistance &&
                       std::abs(pre.flag_rect.y - res.flag_rect.y) < kMinDistance;
            });
            if (find_it != opers_result.end()) {
                continue;
            }
            opers_result.emplace_back(std::move(res));
        }
    }

    if (opers_result.empty()) {
        Log.error("BattleFormationTask: no oper found");
        return {};
    }
    sort_by_vertical_(opers_result);

    std::vector<TextRect> tr_res;
    for (const auto& res : opers_result) {
        tr_res.emplace_back(TextRect { res.rect, res.score, res.text });
    }
    Log.info(tr_res);
    return tr_res;
}

bool asst::BattleFormationTask::enter_selection_page()
{
    return ProcessTask(*this, { "BattleQuickFormation" }).run();
}

bool asst::BattleFormationTask::select_opers_in_cur_page(std::vector<OperGroup>& groups)
{
    auto opers_result = analyzer_opers();

    static const std::array<Rect, 3> SkillRectArray = {
        Task.get("BattleQuickFormationSkill1")->specific_rect,
        Task.get("BattleQuickFormationSkill2")->specific_rect,
        Task.get("BattleQuickFormationSkill3")->specific_rect,
    };

    if (!opers_result.empty()) {
        if (m_last_oper_name == opers_result.back().text) {
            Log.info("last oper name is same as current, skip");
            return false;
        }
        m_last_oper_name = opers_result.back().text;
    }

    int delay = Task.get("BattleQuickFormationOCR")->post_delay;

    int skill = 1;
    for (const auto& res : opers_result) {
        const std::string& name = res.text;
        auto iter = groups.begin();
        bool found = false;
        for (; iter != groups.end(); ++iter) {
            for (const auto& oper : *iter) {
                if (oper.name == name) {
                    found = true;
                    skill = oper.skill;
                    break;
                }
            }
            if (found) {
                break;
            }
        }

        if (iter == groups.end()) {
            continue;
        }
        ctrler()->click(res.rect);
        sleep(delay);
        if (1 <= skill && skill <= 3) {
            ctrler()->click(SkillRectArray.at(skill - 1ULL));
            sleep(delay);
        }
        groups.erase(iter);

        json::value info = basic_info_with_what("BattleFormationSelected");
        auto& details = info["details"];
        details["selected"] = name;
        callback(AsstMsg::SubTaskExtraInfo, info);
    }

    return true;
}

void asst::BattleFormationTask::swipe_page()
{
    ProcessTask(*this, { "BattleFormationOperListSlowlySwipeToTheRight" }).run();
}

void asst::BattleFormationTask::swipe_to_the_left(int times)
{
    for (int i = 0; i < times; ++i) {
        ProcessTask(*this, { "BattleFormationOperListSwipeToTheLeft" }).run();
    }
}

bool asst::BattleFormationTask::confirm_selection()
{
    return ProcessTask(*this, { "BattleQuickFormationConfirm" }).run();
}

bool asst::BattleFormationTask::click_role_table(battle::Role role)
{
    static const std::unordered_map<battle::Role, std::string> RoleNameType = {
        { battle::Role::Caster, "Caster" }, { battle::Role::Medic, "Medic" },     { battle::Role::Pioneer, "Pioneer" },
        { battle::Role::Sniper, "Sniper" }, { battle::Role::Special, "Special" }, { battle::Role::Support, "Support" },
        { battle::Role::Tank, "Tank" },     { battle::Role::Warrior, "Warrior" },
    };
    m_last_oper_name.clear();

    auto role_iter = RoleNameType.find(role);

    std::vector<std::string> tasks;
    if (role_iter == RoleNameType.cend()) {
        tasks = { "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" };
    }
    else {
        tasks = { "BattleQuickFormationRole-" + role_iter->second };
    }
    return ProcessTask(*this, tasks).set_retry_times(0).run();
}

bool asst::BattleFormationTask::parse_formation()
{
    json::value info = basic_info_with_what("BattleFormation");
    auto& details = info["details"];
    auto& formation = details["formation"];

    auto* groups = &Copilot.get_data().groups;
    if (m_data_resource == DataResource::SSSCopilot) {
        groups = &SSSCopilot.get_data().groups;
    }

    for (const auto& [name, opers_vec] : *groups) {
        if (opers_vec.empty()) {
            continue;
        }
        formation.array_emplace(name);

        bool same_role = true;
        battle::Role role = BattleData.get_role(opers_vec.front().name);
        for (const auto& oper : opers_vec) {
            same_role &= BattleData.get_role(oper.name) == role;
        }

        // for unknown, will use { "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" }
        m_formation[same_role ? role : battle::Role::Unknown].emplace_back(opers_vec);
    }

    callback(AsstMsg::SubTaskExtraInfo, info);
    return true;
}
