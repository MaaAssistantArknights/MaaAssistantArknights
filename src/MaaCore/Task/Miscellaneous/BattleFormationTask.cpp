#include "BattleFormationTask.h"

#include <ranges>

#include "Config/GeneralConfig.h"
#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Config/Miscellaneous/CopilotConfig.h"
#include "Config/Miscellaneous/SSSCopilotConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "UseSupportUnitTaskPlugin.h"
#include "Utils/ImageIo.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Miscellaneous/OperNameAnalyzer.h"
#include "Vision/MultiMatcher.h"
#include "Vision/RegionOCRer.h"

asst::BattleFormationTask::BattleFormationTask(
    const AsstCallback& callback,
    Assistant* inst,
    std::string_view task_chain) :
    AbstractTask(callback, inst, task_chain),
    m_use_support_unit_task_ptr(std::make_shared<UseSupportUnitTaskPlugin>(callback, inst, task_chain))
{
}

bool asst::BattleFormationTask::set_specific_support_unit(const std::string& name)
{
    LogTraceFunction;

    if (m_support_unit_usage != SupportUnitUsage::Specific) {
        Log.error(__FUNCTION__, "| Current support unit usage is not SupportUnitUsage::Specific");
        return false;
    }

    const battle::Role role = (m_specific_support_unit.role = BattleData.get_role(name));
    if (role == battle::Role::Unknown) {
        // 无法根据干员名称获取其职业
        Log.error(__FUNCTION__, "| Invalid specific support unit");
        return false;
    }
    m_specific_support_unit.name = name; // 此处可能需要对阿米娅进行特殊处理
    // 之后在 parse_formation 中，如果发现这名助战干员，则将其技能设定为对应的所需技能
    m_specific_support_unit.skill = 0;
    return true;
};

bool asst::BattleFormationTask::_run()
{
    LogTraceFunction;

    const auto& img = ctrler()->get_image();
    if (!is_formation_valid(img)) {
        return true; // 编队不可用，直接返回，常见于TR关卡
    }

    if (!parse_formation()) {
        return false;
    }
    else if (compare_formation()) {
        Log.info(__FUNCTION__, "| Formation is the same as last time, skip");
        for (auto& [name, opers] : m_formation | std::views::values | std::views::join) {
            const auto& pair_it =
                std::ranges::find_if(*m_opers_in_formation, [&](const auto& pair) { return pair.second == name; });
            if (pair_it == m_opers_in_formation->end()) {
                continue;
            }

            auto oper_it =
                std::ranges::find_if(opers, [&](const battle::OperUsage& oper) { return oper.name == pair_it->first; });
            if (oper_it == opers.end()) {
                Log.error(__FUNCTION__, "| Cannot find oper ", pair_it->first, " in m_formation");
            }
            else {
                oper_it->status = battle::OperStatus::Selected; // 更新编队情况
            }
        }
        return true; // 编队未变更，跳过
    }

    if (m_select_formation_index > 0 && !select_formation(m_select_formation_index, img)) {
        return false;
    }

    if (!enter_selection_page(img)) {
        save_img(utils::path("debug") / utils::path("other"));
        return false;
    }
    formation_with_last_opers();
    for (auto& [role, oper_groups] : m_formation) {
        bool need_check =
            std::ranges::any_of(oper_groups, [&](const OperGroup& group) { return has_oper_unchecked(group.second); });
        if (!need_check) {
            continue; // 干员已编入, 跳过
        }
        std::vector<OperGroup*> groups;
        for (auto& oper_group : oper_groups) {
            if (has_oper_unchecked(oper_group.second)) {
                groups.emplace_back(&oper_group);
            }
        }
        add_formation(role, groups);
    }

    // 记录缺失干员组的数量
    int missing_numbers = (int)std::ranges::count_if(
        m_formation | std::views::transform([&](const auto& pair) { return pair.second; }) | std::views::join,
        [&](const OperGroup& group) { return !has_oper_selected(group.second); });
    // 在有且仅有一个缺失干员组时尝试寻找助战干员补齐编队
    if (use_suppprt_unit_when_needed() && missing_numbers == 1 && !m_used_support_unit) {
        // 之后再重构数据结构，先凑合用
        std::vector<battle::RequiredOper> required_opers;
        for (const battle::OperUsage& oper :
             m_formation | std::views::transform([&](const auto& pair) { return pair.second; }) /* 剔除职业 */ |
                 std::views::join | /* 拿到缺干员的组 */ std::views::filter([&](const OperGroup& group) {
                     return std::ranges::any_of(group.second, [&](const battle::OperUsage& oper) {
                         return oper.status == battle::OperStatus::Missing;
                     });
                 }) |
                 std::views::transform([&](const OperGroup& pair) { return pair.second; }) | std::views::join) {
            // 如果指定助战干员正好可以补齐编队，则只招募指定助战干员就好了，记得再次确认一下 skill
            // 如果编队里正好有【艾雅法拉 - 2】和 【艾雅法拉 - 3】呢？
            if (oper.name == m_specific_support_unit.name) {
                m_specific_support_unit.skill = oper.skill;
                required_opers.clear();
                required_opers.emplace_back(m_specific_support_unit);
                break;
            }
            required_opers.emplace_back(BattleData.get_role(oper.name), oper.name, oper.skill);
        }

        // 先退出去招募助战再回来，好蠢
        confirm_selection();
        Log.info(__FUNCTION__, "| Left quick formation scene");
        if (m_use_support_unit_task_ptr->try_add_support_unit(required_opers, 5, true)) {
            m_used_support_unit = true;
        }
        // 再到快速编队页面
        if (!enter_selection_page()) {
            save_img(utils::path("debug") / utils::path("other"));
            return false;
        }
        Log.info(__FUNCTION__, "| Returned to quick formation scene");
    }

    // 在尝试补齐编队后依然有缺失干员，自动编队失败
    bool has_missing = std::ranges::any_of(m_formation, [&](const auto& pair) {
        return std::ranges::any_of(pair.second /* role, groups */, [&](const OperGroup& group) {
            return !has_oper_selected(group.second);
        });
    });
    if (has_missing) {
        report_missing_operators();
        return false;
    }

    // 对于有在干员组中存在的自定干员，无法提前得知是否成功编入，故不提前加入编队
    if (!m_user_additional.empty()) {
        std::unordered_map<battle::Role, std::vector<OperGroup>> user_formation; // 解析后用户自定编队
        auto limit = 12 - (int)m_opers_in_formation->size();
        for (const auto& [name, skill] : m_user_additional) {
            if (m_opers_in_formation->contains(name)) {
                continue;
            }
            if (--limit < 0) {
                break;
            }
            asst::battle::OperUsage oper;
            oper.name = name;
            oper.skill = skill;
            std::vector<asst::battle::OperUsage> usage { std::move(oper) };
            user_formation[BattleData.get_role(name)].emplace_back(name, std::move(usage));
        }
        click_role_table(battle::Role::Unknown);
        for (auto& [role, oper_groups] : user_formation) {
            std::vector<OperGroup*> groups;
            for (auto& oper_group : oper_groups) {
                if (has_oper_unchecked(oper_group.second)) {
                    groups.emplace_back(&oper_group);
                }
            }
            add_formation(role, groups);
        }
    }

    // add_additional();
    if (m_add_trust) {
        add_trust_operators();
    }
    confirm_selection();

    if (m_support_unit_usage == SupportUnitUsage::Specific && !m_used_support_unit) { // 使用指定助战干员
        m_used_support_unit = m_use_support_unit_task_ptr->try_add_support_unit({ m_specific_support_unit }, 5, true);
    }
    else if (m_support_unit_usage == SupportUnitUsage::Random && !m_used_support_unit) { // 使用随机助战干员
        m_used_support_unit = m_use_support_unit_task_ptr->try_add_support_unit({}, 5, false);
    }

    return true;
}

void asst::BattleFormationTask::formation_with_last_opers()
{
    // 此时的oper_in_formation是根据上次编队结果复用的, 但是task此时已清空, 重新选一下好了
    if (m_opers_in_formation->empty()) {
        return;
    }

    const auto& opers_result = analyzer_opers(ctrler()->get_image());

    if (!opers_result.empty()) {
        if (m_last_oper_name == opers_result.back().text) {
            Log.info("last oper name is same as current, skip");
            return;
        }
        m_last_oper_name = opers_result.back().text;
    }

    // 展平的干员组
    auto formation_view = m_formation | std::views::values | std::views::join;
    const int delay = Task.get("BattleQuickFormationOCR")->post_delay;
    for (auto it = m_opers_in_formation->begin(); !need_exit() && it != m_opers_in_formation->end();) {
        const auto& oper_in_page_it = std::ranges::find_if(opers_result, [&](const QuickFormationOper& op) {
            return !op.is_selected && op.text == it->first;
        }); // 编队页中的干员
        if (oper_in_page_it == opers_result.end()) [[unlikely]] {
            Log.warn(__FUNCTION__, "| Oper", it->first, "was selected last time, but not found in current page");
            it = m_opers_in_formation->erase(it);
            continue; // 该干员找不到, 一页只能放下10个干员, 可能被右侧挡住. 打回到正常编队逻辑
        }

        auto opers_view =
            formation_view |
            std::views::filter([&](const std::pair<std::string, std::vector<asst::battle::OperUsage>>& pair) {
                return pair.first == it->second;
            }) |
            std::views::values | std::views::join;
        auto oper_it =
            std::ranges::find_if(opers_view, [&](const battle::OperUsage& op) { return op.name == it->first; });
        if (oper_it == opers_view.end()) {
            LogError << __FUNCTION__ << "| Group" << it->second << ",Oper" << it->first
                     << "was selected last time, but not found in current pair";
            continue; // 很怪, 按理说不会找不到, 有组相同但是找不到干员
        }

        ctrler()->click(oper_in_page_it->flag_rect);
        sleep(delay);

        oper_it->status = battle::OperStatus::Selected;
        json::value info = basic_info_with_what("BattleFormationSelected");
        auto& details = info["details"];
        details["selected"] = it->first;
        details["group_name"] = it->second;
        callback(AsstMsg::SubTaskExtraInfo, info);
        ++it;
    }
}

bool asst::BattleFormationTask::add_formation(battle::Role role, const std::vector<OperGroup*>& oper_group)
{
    LogTraceFunction;

    click_role_table(role);
    bool has_error = false;
    int swipe_times = 0;
    int overall_swipe_times = 0; // 完整从左到右滑动的次数
    while (!need_exit()) {
        if (select_opers_in_cur_page(oper_group)) {
            has_error = false;
            bool exit =
                std::ranges::all_of(oper_group, [&](OperGroup* group) { return !has_oper_unchecked(group->second); });
            if (exit) {
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
            if (overall_swipe_times == m_missing_retry_times) {
                for (auto& group : oper_group) {
                    if (has_oper_selected(group->second)) {
                        continue;
                    }
                    for (auto& oper : group->second) {
                        if (oper.status == battle::OperStatus::Unchecked) {
                            oper.status = battle::OperStatus::Missing;
                        }
                    }
                }
                return true;
            }

            ++overall_swipe_times;

            has_error = true;
            swipe_to_the_left(swipe_times);
            swipe_times = 0;
        }
    }
    return true;
}

bool asst::BattleFormationTask::add_additional()
{
    // （但是干员名在除开获取时间的情况下都会被遮挡，so ?
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

            auto opers_result = analyzer_opers(ctrler()->get_image());

            // TODO 这里要识别一下干员之前有没有被选中过
            for (size_t i = 0; i < static_cast<size_t>(number) && i < opers_result.size(); ++i) {
                const auto& oper = opers_result.at(i);
                ctrler()->click(oper.flag_rect);
            }
        }
    }

    return true;
}

bool asst::BattleFormationTask::add_trust_operators()
{
    LogTraceFunction;

    if (need_exit()) {
        return false;
    }

    // 需要追加的信赖干员数量
    int append_count = 12 - (int)m_opers_in_formation->size();
    if (append_count == 0) {
        return true;
    }

    ProcessTask(*this, { "BattleQuickFormationFilter" }).run();
    // 双击信赖
    ProcessTask(*this, { "BattleQuickFormationFilter-Trust" }).run();
    ProcessTask(*this, { "BattleQuickFormationFilterClose" }).run();
    // 检查特关是否开启
    ProcessTask(*this, { "BattleQuickFormationFilter-PinUnactivated", "BattleQuickFormationFilter-PinActivated" })
        .run();

    // 重置职业选择，保证处于最左
    click_role_table(battle::Role::Caster);
    click_role_table(battle::Role::Unknown);
    int failed_count = 0;
    while (!need_exit() && append_count > 0 && failed_count < 3) {
        MultiMatcher matcher(ctrler()->get_image());
        matcher.set_task_info("BattleQuickFormationTrustIcon");
        if (!matcher.analyze() || matcher.get_result().size() == 0) {
            failed_count++;
        }
        else {
            failed_count = 0;
            std::vector<MatchRect> result = matcher.get_result();
            // 按先上下后左右排个序
            sort_by_vertical_(result);
            for (const auto& trust_icon : result) {
                // 匹配完干员左下角信赖表，将roi偏移到整个干员标
                ctrler()->click(trust_icon.rect.move({ 20, -225, 110, 250 }));
                --append_count;
                if (append_count <= 0 || need_exit()) {
                    break;
                }
            }
        }
        if (!need_exit() && append_count > 0) {
            swipe_page();
        }
    }

    return append_count == 0;
}

bool asst::BattleFormationTask::select_random_support_unit()
{
    return ProcessTask(*this, { "BattleSupportUnitFormation" }).run();
}

void asst::BattleFormationTask::report_missing_operators()
{
    auto info = basic_info();

    std::map<std::string, json::array> oper_names;
    for (const auto& [_, groups] : m_formation) {
        for (const auto& [name, opers_in_group] : groups) {
            if (has_oper_selected(opers_in_group)) {
                continue; // 该干员组中有干员已被选中
            }
            json::array opers_array;
            for (const auto& oper : opers_in_group) {
                json::object json { { "name", oper.name } };
                switch (oper.status) {
                case battle::OperStatus::Selected: // 理论上这里就不该进这case
                    break;
                case battle::OperStatus::Unchecked:
                    json["reason"] = "Unchecked";
                    opers_array.emplace_back(std::move(json));
                    break;
                case battle::OperStatus::Unavailable:
                    json["reason"] = "Unavailable";
                    opers_array.emplace_back(std::move(json));
                    break;
                case battle::OperStatus::Missing:
                    json["reason"] = "Missing";
                    opers_array.emplace_back(std::move(json));
                    break;
                }
            }
            oper_names.emplace(name, std::move(opers_array));
        }
    }

    info["why"] = "OperatorMissing";

    info["details"] = json::object { { "opers", json::object(oper_names) } };
    callback(AsstMsg::SubTaskError, info);
}

bool asst::BattleFormationTask::has_oper_selected(const std::vector<asst::battle::OperUsage>& opers) const
{
    return std::ranges::any_of(opers, [](const battle::OperUsage& op) {
        return op.status == battle::OperStatus::Selected;
    });
}

bool asst::BattleFormationTask::has_oper_unchecked(const std::vector<asst::battle::OperUsage>& opers) const
{
    return !has_oper_selected(opers) && std::ranges::any_of(opers, [](const battle::OperUsage& op) {
        return op.status == battle::OperStatus::Unchecked;
    });
}

std::vector<asst::BattleFormationTask::QuickFormationOper>
    asst::BattleFormationTask::analyzer_opers(const cv::Mat& image)
{
    const auto& ocr_replace = Task.get<OcrTaskInfo>("CharsNameOcrReplace");
    std::vector<asst::BattleFormationTask::QuickFormationOper> opers_result;
    cv::Mat select;
    for (int i = 0; i < 8; ++i) {
        std::string task_name = "BattleQuickFormation-OperNameFlag" + std::to_string(i);

        const auto& ocr_task = Task.get("BattleQuickFormationOCR");
        MultiMatcher multi(image);
        multi.set_task_info(task_name);
        if (!multi.analyze()) [[unlikely]] {
            continue;
        }
        for (const auto& flag : multi.get_result()) {
            OperNameAnalyzer region(image);
            region.set_task_info(ocr_task);
            region.set_roi(flag.rect.move(ocr_task->rect_move));
            region.set_bin_threshold(ocr_task->special_params[0]);
            region.set_bin_expansion(ocr_task->special_params[1]);
            region.set_bin_trim_threshold(ocr_task->special_params[2], ocr_task->special_params[3]);
            region.set_bottom_line_height(ocr_task->special_params[4]);
            region.set_width_threshold(ocr_task->special_params[5]);
            region.set_replace(ocr_replace->replace_map, ocr_replace->replace_full);
            region.set_use_raw(true);
            if (!region.analyze()) [[unlikely]] {
                continue;
            }

            const auto& ocr_result = region.get_result();
            asst::BattleFormationTask::QuickFormationOper res;
            res.text = ocr_result.text;
            res.rect = ocr_result.rect;
            res.score = ocr_result.score;
            res.flag_rect = flag.rect;
            res.flag_score = flag.score;

            constexpr int kMinDistance = 5;
            auto find_it =
                std::ranges::find_if(opers_result, [&res](const asst::BattleFormationTask::QuickFormationOper& pre) {
                    return std::abs(pre.flag_rect.x - res.flag_rect.x) < kMinDistance &&
                           std::abs(pre.flag_rect.y - res.flag_rect.y) < kMinDistance;
                });
            if (find_it != opers_result.end() || res.text.empty()) {
                continue;
            }
            select = make_roi(image, res.flag_rect.move({ 0, -10, 5, 4 }));
            cv::inRange(select, cv::Scalar(200, 140, 0), cv::Scalar(255, 180, 100), select);
            res.is_selected = cv::hasNonZero(select);
            opers_result.emplace_back(std::move(res));
        }
    }
    if (opers_result.empty()) {
        Log.error("BattleFormationTask: no oper found");
        return {};
    }
    sort_by_vertical_(opers_result);

    Log.info(opers_result);
    return opers_result;
}

bool asst::BattleFormationTask::enter_selection_page(const cv::Mat& img)
{
    return ProcessTask(*this, { "BattleQuickFormation" }).set_reusable_image(img).set_retry_times(3).run();
}

bool asst::BattleFormationTask::select_opers_in_cur_page(const std::vector<OperGroup*>& groups)
{
    const auto& opers_result = analyzer_opers(ctrler()->get_image());

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

    const int delay = Task.get("BattleQuickFormationOCR")->post_delay;
    battle::OperUsage* oper = nullptr;
    bool ret = true;
    for (const auto& res :
         opers_result | std::views::filter([](const QuickFormationOper& op) { return !op.is_selected; })) {
        const auto& iter = std::ranges::find_if(groups, [&](OperGroup* group) {
            if (!has_oper_unchecked(group->second)) { // 干员组没有干员已选中且存在可用干员
                return false;
            }
            auto it = std::ranges::find_if(group->second, [&](const battle::OperUsage& op) {
                return op.name == res.text && op.status != battle::OperStatus::Unavailable;
            });
            if (it != group->second.cend()) {
                oper = &(*it);
                return true; // 找到干员
            }
            return false;
        });

        if (iter == groups.end()) {
            continue;
        }
        else if (!oper) {
            Log.error(__FUNCTION__, "| Oper was founded, but pointer is null");
        }

        ctrler()->click(res.flag_rect);
        sleep(delay);
        if (1 <= oper->skill && oper->skill <= 3) {
            if (oper->skill == 3) {
                ProcessTask(*this, { "BattleQuickFormationSkill-SwipeToTheDown" }).run();
            }
            ctrler()->click(SkillRectArray.at(oper->skill - 1ULL));
            sleep(delay);
        }
        if (oper->requirements.module >= 0 && oper->requirements.module <= 4) {
            ret = ProcessTask(*this, { "BattleQuickFormationModulePage" }).run();
            ret =
                ret &&
                ProcessTask(*this, { "BattleQuickFormationModule-" + std::to_string(oper->requirements.module) }).run();
            if (!ret) {
                LogWarn << __FUNCTION__ << "| Module " << oper->requirements.module
                        << "not found, please check the module number";

                json::value info = basic_info_with_what("BattleFormationOperUnavailable");
                info["details"]["oper_name"] = res.text;
                info["details"]["requirement_type"] = "module";
                callback(AsstMsg::SubTaskExtraInfo, info);
                if (m_ignore_requirements) {
                    // 模组不满足时选择默认模组
                    LogInfo << __FUNCTION__ << "| Module " << oper->requirements.module
                            << " not satisfied, skip module selection";
                }
                else {
                    ctrler()->click(res.flag_rect); // 选择模组失败时反选干员
                    sleep(delay);
                    // 继续检查同组其他干员
                    oper->status = battle::OperStatus::Unavailable;
                    continue;
                }
            }
            sleep(delay);
        }
        oper->status = battle::OperStatus::Selected;
        m_opers_in_formation->emplace(res.text, (*iter)->first);
        json::value info = basic_info_with_what("BattleFormationSelected");
        auto& details = info["details"];
        details["selected"] = res.text;
        details["group_name"] = (*iter)->first;
        callback(AsstMsg::SubTaskExtraInfo, info);
        oper = nullptr; // reset oper pointer
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
    sleep(Config.get_options().task_delay); // 可能有界面回弹，睡一会儿
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
        tasks = { "BattleQuickFormationRole-" + role_iter->second,
                  "BattleQuickFormationRole-All",
                  "BattleQuickFormationRole-All-OCR" };
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

    std::swap(m_formation, m_formation_last);
    m_formation.clear();
    m_opers_in_formation->clear();
    for (const auto& [name, opers_vec] : *groups) {
        if (opers_vec.empty()) {
            continue;
        }
        formation.emplace(name);

        // 判断干员/干员组的职业，放进对应的分组
        bool same_role = true;
        battle::Role role = BattleData.get_role(opers_vec.front().name);
        for (const auto& oper : opers_vec) {
            same_role &= BattleData.get_role(oper.name) == role;

            // （仅一次）如果发现这名助战干员，则将其技能设定为对应的所需技能
            if (oper.name == m_specific_support_unit.name && m_specific_support_unit.skill == 0) {
                m_specific_support_unit.skill = oper.skill;
            }
        }

        // for unknown, will use { "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" }
        m_formation[same_role ? role : battle::Role::Unknown].emplace_back(name, opers_vec);
    }
    callback(AsstMsg::SubTaskExtraInfo, info);
    return true;
}

bool asst::BattleFormationTask::compare_formation()
{
    for (auto& [role, groups_] : m_formation) {
        const auto& role_it = m_formation_last.find(role);
        if (role_it == m_formation_last.cend()) {
            continue;
        }

        auto& last_groups = role_it->second;
        for (auto& group : groups_) {
            auto last_group_it = std::ranges::find_if(last_groups, [&](const OperGroup& g) {
                return g.first == group.first && g.second == group.second;
            });
            if (last_group_it == last_groups.end()) { // fallback to only opers equal
                last_group_it =
                    std::ranges::find_if(last_groups, [&](const OperGroup& g) { return g.second == group.second; });
            }
            if (last_group_it == last_groups.end()) {
                continue; // not find the same group in last formation
            }

            const auto& oper_last_it = std::ranges::find_if(last_group_it->second, [&](const battle::OperUsage& op) {
                return op.status == battle ::OperStatus::Selected;
            });
            if (oper_last_it == last_group_it->second.cend()) [[unlikely]] {
                LogError << __FUNCTION__ << "| Group" << last_group_it->first
                         << "was selected last time, but no oper was selected";
                continue; // 旧编队中该干员组有干员被选中，但找不到被选中的干员
            }
            m_opers_in_formation->emplace(oper_last_it->name, group.first);
            last_groups.erase(last_group_it); // 移除已匹配的干员组
        }
    }

    return static_cast<size_t>(std::ranges::distance(m_formation | std::views::values | std::views::join)) ==
           m_opers_in_formation->size();
}

bool asst::BattleFormationTask::select_formation(int select_index, const cv::Mat& img)
{
    // 编队不会触发改名的区域有两组
    // 一组是上面的黑长条 260*9
    // 第二组是名字最左边和最右边的一块区域
    // 右边比左边窄，暂定为左边 10*58

    static const std::array<std::string, 4> select_formation_task = { "BattleSelectFormation1",
                                                                      "BattleSelectFormation2",
                                                                      "BattleSelectFormation3",
                                                                      "BattleSelectFormation4" };

    return ProcessTask { *this, { select_formation_task[select_index - 1] } }.set_reusable_image(img).run();
}

bool asst::BattleFormationTask::is_formation_valid(const cv::Mat& img) const
{
    static const std::string valid_task = "BattleStartPre@BattleQuickFormation";
    ProcessTask task(*this, { "BattleFormationInvalid", valid_task });
    task.set_reusable_image(img);
    return task.run() && task.get_last_task_name() == valid_task;
}
