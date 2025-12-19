#include "SupportList.h"

#include "Config/GeneralConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/AbstractTask.h"
#include "Task/ProcessTask.h"
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Battle/SupportListAnalyzer.h"
#include "Vision/Matcher.h"
#include "Vision/MultiMatcher.h"

asst::SupportList::SupportList(const AsstCallback& callback, Assistant* inst, std::string_view task_chain) :
    InstHelper(inst),
    m_callback(callback),
    m_task_chain(task_chain)
{
}

bool asst::SupportList::select_role(const Role role)
{
    LogTraceFunction;

    if (m_in_support_unit_detail_panel) {
        LogError << __FUNCTION__
                 << "| Invalid operation: currently in support unit detail panel; failed to select role; reverting to "
                 << enum_to_string(m_selected_role);
        return false;
    }

    if (std::ranges::find(SUPPORT_UNIT_ROLES, role) == SUPPORT_UNIT_ROLES.end()) {
        LogError << __FUNCTION__ << "| Unsupported role: " << enum_to_string(role)
                 << "; failed to select role; reverting to " << enum_to_string(m_selected_role);
        return false;
    }

    reset_list_and_view_data();

    if (ProcessTask(m_callback, m_inst, m_task_chain)
            .set_tasks(
                { enum_to_string(role, true) + "@SupportList-RoleSelected",
                  enum_to_string(role, true) + "@SupportList-SelectRole" })
            .run()) {
        m_selected_role = role;
        Log.info(__FUNCTION__, "| Successfully selected role", enum_to_string(role));
        return true;
    }

    m_selected_role = Role::Unknown;
    LogError << __FUNCTION__ << "| Failed to select role" << enum_to_string(role)
             << "the currently selected role is unknown";
    return false;
}

bool asst::SupportList::update()
{
    LogTraceFunction;

    if (m_in_support_unit_detail_panel) {
        LogError << __FUNCTION__
                 << "| Invalid operation: currently in support unit detail panel; failed to update support list";
        return false;
    }

    // 识别助战列表当前所选的职业并更新 m_selected_role
    if (m_selected_role == Role::Unknown) {
        Log.info(__FUNCTION__, "| The currently selected role is unknown; updating before proceeding");
        if (!update_selected_role(ctrler()->get_image())) {
            Log.error(__FUNCTION__, "Fail to update selected role; abandoning support list update.");
            return false;
        }
    }

    move_to_list_head();

    SupportListAnalyzer analyzer(ctrler()->get_image());

    for (size_t swipe_times = 0; swipe_times < MAX_SWIPE_TIMES && !need_exit(); ++swipe_times) {
        move_forward();

        const std::optional<int> ret = analyzer.merge_image(ctrler()->get_image());
        if (!ret) {
            return false;
        }
        if (ret.value() <= 0) {
            break;
        }
    }

    if (!analyzer.analyze(m_selected_role)) {
        return false;
    }
    m_list = analyzer.get_result();
    print_support_list();

    update_view();

    return true;
}

bool asst::SupportList::select_support_unit(const size_t index)
{
    LogTraceFunction;

    if (index >= m_list.size()) {
        LogError << __FUNCTION__ << "| Support unit No." << index + 1 << "does not exist in support list with size"
                 << m_list.size() << "; failed to use support unit";
        return false;
    }

    if (m_in_support_unit_detail_panel) {
        Log.error(
            __FUNCTION__,
            "| Invalid operation: currently in support unit detail panel; failed to select support unit");
        return false;
    }

    move_to_support_unit(index);

    // click support unit
    Log.info(__FUNCTION__, std::format("| Clicking support unit {}: {}", index + 1, m_list[index].name));
    Rect click_rect = Task.get("SupportList-ClickSupportUnit")->specific_rect;
    click_rect.x = m_support_unit_x_in_view[index];
    ctrler()->click(click_rect);
    sleep(Config.get_options().task_delay);

    if (!ProcessTask(m_parent, { "SupportList-DetailPanel-Flag", "SupportList-DetailPanel-Flag@LoadingText" })
             .set_retry_times(20)
             .run()) {
        Log.error(__FUNCTION__, "| Support unit detail panel not recognised; failed to select support unit");
        save_img(ctrler()->get_image(), "screenshot");
        return false;
    }

    m_in_support_unit_detail_panel = true;
    return true;
}

bool asst::SupportList::confirm_to_use_support_unit()
{
    LogTraceFunction;

    if (!m_in_support_unit_detail_panel) {
        Log.error(
            __FUNCTION__,
            "| Invalid operation: currently not in support unit detail panel; failed to confirm to use support_unit");
        return false;
    }

    if (ProcessTask(m_callback, m_inst, m_task_chain).set_tasks({ "SupportList-DetailPanel-Confirm" }).run()) {
        m_in_support_unit_detail_panel = false;
        reset_list_and_view_data();
        return true;
    }

    return false;
}

bool asst::SupportList::leave_support_unit_detail_panel()
{
    LogTraceFunction;

    if (!m_in_support_unit_detail_panel) {
        LogError << __FUNCTION__
                 << "| Invalid operation: currently not in support unit detail panel; failed to leave support unit "
                    "detail panel";
        return false;
    }

    if (ProcessTask(m_callback, m_inst, m_task_chain).set_tasks({ "SupportList-DetailPanel-Leave" }).run()) {
        m_in_support_unit_detail_panel = false;
        update_view();
        return true;
    }

    return false;
}

bool asst::SupportList::select_skill(const int skill, const int minimum_skill_level)
{
    LogTraceFunction;

    if (!m_in_support_unit_detail_panel) {
        LogError << __FUNCTION__
                 << "| Invalid operation: currently not in support unit detail panel; failed to select skill panel";
        return false;
    }

    if (skill < 1 || skill > 3) {
        LogError << __FUNCTION__ << "| Invalid skill:" << skill << "; failed to select skill";
        return false;
    }

    // ————————————————————————————————————————————————————————————————
    // Check Skill Level
    // ————————————————————————————————————————————————————————————————
    MatchTaskPtr skill_level_task = Task.get<MatchTaskInfo>("SupportList-DetailPanel-SkillLevel");
    Matcher skill_level_analyzer(ctrler()->get_image());
    skill_level_analyzer.set_task_info(skill_level_task);
    Rect skill_level_roi = skill_level_task->roi;
    skill_level_roi.x = skill_level_task->special_params[skill - 1];
    skill_level_analyzer.set_roi(skill_level_roi);

    if (!skill_level_analyzer.analyze()) {
        LogInfo << __FUNCTION__ << "| Skill" << skill << "does not exist; failed to select skill";
        return false;
    }

    std::optional<int> ret = get_suffix_num(skill_level_analyzer.get_result().templ_name);
    if (!ret) [[unlikely]] {
        Log.error(__FUNCTION__, "| Failed to analyze the current support unit's skill level; failed to select skill");
        return false;
    }
    const int skill_level = ret.value();

    if (skill_level < minimum_skill_level) {
        LogInfo << __FUNCTION__ << "| The current support unit's skill level" << skill_level
                << "is below the minimum requirement" << minimum_skill_level << "; failed to select skill";
        return false;
    }

    // ————————————————————————————————————————————————————————————————
    // Select Skill
    // ————————————————————————————————————————————————————————————————
    ctrler()->click(skill_level_analyzer.get_result().rect);
    sleep(Config.get_options().task_delay);

    const MatchTaskPtr skill_selected_task = Task.get<MatchTaskInfo>("SupportList-DetailPanel-SkillSelected");
    Matcher skill_selected_analyzer(ctrler()->get_image());
    skill_selected_analyzer.set_task_info(skill_selected_task);
    Rect skill_selected_roi = skill_selected_task->roi;
    skill_selected_roi.x = skill_selected_task->special_params[skill - 1];
    skill_selected_analyzer.set_roi(skill_selected_roi);

    if (!skill_selected_analyzer.analyze()) {
        LogError << __FUNCTION__ << "| Skill" << skill << "did not respond to the click; failed to select skill";
        return false;
    }

    LogInfo << __FUNCTION__ << "| Selected skill" << skill << "with level" << skill_level;
    return true;
}

bool asst::SupportList::select_module(const OperModule module, const int minimum_module_level)
{
    LogTraceFunction;

    if (!m_in_support_unit_detail_panel) {
        LogError << __FUNCTION__
                 << "| Invalid operation: currently not in support unit detail panel; failed to select skill panel";
        return false;
    }

    if (std::ranges::find(SUPPORT_UNIT_MODULES, module) == SUPPORT_UNIT_MODULES.end()) {
        LogError << __FUNCTION__ << "| Invalid module:" << enum_to_string(module) << "; failed to select module";
        return false;
    }

    // ————————————————————————————————————————————————————————————————
    // Find Module
    // ————————————————————————————————————————————————————————————————
    ProcessTask(m_callback, m_inst, m_task_chain)
        .set_tasks({ "SupportList-DetailPanel-SelectModule-MoveToHead" })
        .run();

    if (module == OperModule::Original) {
        if (!ProcessTask(m_callback, m_inst, m_task_chain)
                 .set_tasks({ "SupportList-DetailPanel-SelectOriginalModule" })
                 .run()) {
            LogInfo << __FUNCTION__ << "| Module" << enum_to_string(module)
                    << "does not exist; failed to select module";
            return false;
        }
        if (!ProcessTask(m_callback, m_inst, m_task_chain)
                 .set_tasks({ "SupportList-DetailPanel-ModuleSelected" })
                 .run()) {
            LogError << __FUNCTION__ << "| Module" << enum_to_string(module)
                     << "did not respond to the click; failed to select module";
            return false;
        }
        LogInfo << __FUNCTION__ << "| Selected module" << enum_to_string(module);
        return true;
    }

    for (size_t page = 0; page < MAX_NUM_MODULE_PAGES && !need_exit(); ++page) {
        std::vector<ModuleItem> module_items = analyze_module_page();
        for (const auto& [rect, module_, module_level] : module_items) {
            if (module_ != module) {
                continue;
            }
            // ————————————————————————————————————————————————————————————————
            // Check Module Level
            // ————————————————————————————————————————————————————————————————
            if (module_level < minimum_module_level) {
                LogInfo << __FUNCTION__ << "| The current support unit's module" << enum_to_string(module)
                        << "is at level" << module_level << "which is below the minimum requirement"
                        << minimum_module_level << "; failed to select module";
                return false;
            }

            // ————————————————————————————————————————————————————————————————
            // Select Module
            // ————————————————————————————————————————————————————————————————
            ctrler()->click(rect);
            sleep(Config.get_options().task_delay);

            const MatchTaskPtr module_selected_task = Task.get<MatchTaskInfo>("SupportList-DetailPanel-ModuleSelected");
            Matcher module_selected_analyzer(ctrler()->get_image());
            module_selected_analyzer.set_task_info(module_selected_task);
            const Rect module_selected_roi = rect.move(module_selected_task->rect_move);
            module_selected_analyzer.set_roi(module_selected_roi);

            if (!module_selected_analyzer.analyze()) {
                LogError << __FUNCTION__ << "| Module" << enum_to_string(module)
                         << "did not respond to the click; failed to select module";
                return false;
            }

            LogInfo << __FUNCTION__ << "| Selected module" << enum_to_string(module) << "with level" << module_level;
            return true;
        }

        if (page < MAX_NUM_MODULE_PAGES - 1) {
            ProcessTask(m_callback, m_inst, m_task_chain)
                .set_tasks({ "SupportList-DetailPanel-SelectModule-MoveRight" })
                .run();
        }
    }

    LogInfo << __FUNCTION__ << "| Module" << enum_to_string(module) << "does not exist; failed to select module";
    return false;
}

bool asst::SupportList::refresh_list()
{
    LogTraceFunction;

    if (m_in_support_unit_detail_panel) [[unlikely]] {
        LogError << __FUNCTION__
                 << "| Invalid operation: currently in support unit detail panel; failed to refresh support list";
        return false;
    }

    reset_list_and_view_data();

    return ProcessTask(m_callback, m_inst, m_task_chain).set_tasks({ "SupportList-RefreshAfterCooldown" }).run();
}

bool asst::SupportList::update_selected_role(const cv::Mat& image)
{
    LogTraceFunction;

    Matcher role_analyzer(image);
    for (const Role role : SUPPORT_UNIT_ROLES) {
        role_analyzer.set_task_info(enum_to_string(role, true) + "@SupportList-RoleSelected");
        if (role_analyzer.analyze()) {
            m_selected_role = role;
            Log.info(__FUNCTION__, "| The currently selected role is", enum_to_string(role));
            return true;
        }
    }

    m_selected_role = Role::Unknown;
    Log.error(__FUNCTION__, "| Fail to update currently selected role; the currently selected role is unknown");
    return false;
}

void asst::SupportList::reset_list_and_view_data()
{
    m_list.clear();
    reset_view();
}

void asst::SupportList::print_support_list() const
{
    static constexpr std::string_view FMT_STR = "{:^5} | {:^2} | {:^3} | {:^3} | {:^3} | {}";

    Log.info("Support List");
    Log.info(std::string(40, '-'));
    Log.info(std::format(FMT_STR, "Elite", "Lv", "Pot", "Mod", "Frd", "Name"));
    Log.info(std::string(40, '-'));
    for (const SupportUnit& support_unit : m_list) {
        Log.info(
            std::format(
                FMT_STR,
                support_unit.elite,
                support_unit.level,
                support_unit.potential,
                support_unit.module_enabled ? "Y" : "N",
                support_unit.friendship == Friendship::BestFriend
                    ? "BF"
                    : (support_unit.friendship == Friendship::Friend ? "F" : "S"),
                support_unit.name));
    }
    Log.info(std::string(40, '-'));
}

void asst::SupportList::update_view(const cv::Mat& image)
{
    LogTraceFunction;

    reset_view();

    cv::Mat img = image.empty() ? ctrler()->get_image() : image;

    for (size_t i = 0; i < m_list.size(); ++i) {
        if (const Matcher::ResultOpt support_unit_match_ret =
                SupportListAnalyzer::match_support_unit(img, m_list[i].templ)) {
            if (i < m_view_begin) {
                m_view_begin = i;
            }
            if (i >= m_view_end) {
                m_view_end = i + 1;
            }
            m_support_unit_x_in_view[i] = support_unit_match_ret.value().rect.x;
        }
    }

    LogInfo << __FUNCTION__ << "| Current view is [" << m_view_begin + 1 << ", " << m_view_end << "]";
}

void asst::SupportList::reset_view()
{
    m_view_begin = m_list.size();
    m_view_end = 0;
    m_support_unit_x_in_view.assign(m_list.size(), UNDEFINED);
}

void asst::SupportList::move_to_support_unit(const size_t index)
{
    LogTraceFunction;

    // sanity check
    if (index >= m_list.size()) [[unlikely]] {
        LogError << __FUNCTION__ << "| Attempt to move to support unit " << index + 1 << " out of " << m_list.size();
        return;
    }

    LogInfo << __FUNCTION__ << "| Moving to support unit " << index + 1 << ": " << m_list[index].name;

    cv::Mat image;
    while (!need_exit()) {
        if (index < m_view_begin) {
            move_backward();
            image = ctrler()->get_image();
            update_view(image);
            continue;
        }
        if (index >= m_view_end) {
            move_forward();
            image = ctrler()->get_image();
            update_view(image);
            continue;
        }
        if (m_support_unit_x_in_view[index] == UNDEFINED) [[unlikely]] {
            LogError << __FUNCTION__ << "| rect for support unit " << index + 1 << " in view is not updated";
            save_img(image, "lastly used screenshot");
            save_img(m_list[index].templ, "support unit template");
            image = ctrler()->get_image();
            update_view(image);
            continue;
        }
        break;
    }
}

void asst::SupportList::move_to_list_head()
{
    LogTraceFunction;

    ProcessTask(m_callback, m_inst, m_task_chain).set_tasks({ "SupportList-MoveToHead" }).run();
}

void asst::SupportList::move_forward()
{
    LogTraceFunction;

    ProcessTask(m_callback, m_inst, m_task_chain).set_tasks({ "SupportList-MoveRight" }).run();
}

void asst::SupportList::move_backward()
{
    LogTraceFunction;

    ProcessTask(m_callback, m_inst, m_task_chain).set_tasks({ "SupportList-MoveLeft" }).run();
}

std::vector<asst::SupportList::ModuleItem> asst::SupportList::analyze_module_page()
{
    LogTraceFunction;

    const MatchTaskPtr module_task = Task.get<MatchTaskInfo>("SupportList-DetailPanel-Module");

    cv::Mat image = ctrler()->get_image();

    MultiMatcher module_level_analyzer(image);
    module_level_analyzer.set_task_info("SupportList-DetailPanel-ModuleLevel");
    if (!module_level_analyzer.analyze()) {
        Log.info(__FUNCTION__, "No non-original module found");
        return {};
    }
    MultiMatcher::ResultsVec module_level_analyze_result = module_level_analyzer.get_result();
    sort_by_vertical_(module_level_analyze_result); // 按照水平方向排序（从左到右）

    std::vector<ModuleItem> module_items;
    for (const auto& [rect, score, templ_name] : module_level_analyze_result) {
        std::optional<int> ret = get_suffix_num(templ_name);
        if (!ret) [[unlikely]] {
            Log.error(__FUNCTION__, "| Failed to analyze the current module's level; skipping to the next one");
            continue;
        }
        const int module_level = ret.value();

        Matcher module_analyzer(image);
        module_analyzer.set_task_info(module_task);
        const Rect module_roi = rect.move(module_task->rect_move);
        module_analyzer.set_roi(module_roi);
        if (!module_analyzer.analyze()) {
            Log.error(__FUNCTION__, "Failed to recognise the current module's letter; skipping to the next one");
            save_img(image, "screenshot");
            continue;
        }
        const std::string module_str = get_suffix_str(module_analyzer.get_result().templ_name);

        static const std::unordered_map<std::string, OperModule> STR_OPER_MODULE_MAP = {
            { "Chi", OperModule::Chi },     { "Upsilon", OperModule::Upsilon }, { "Delta", OperModule::Delta },
            { "Alpha", OperModule::Alpha }, { "Beta", OperModule::Beta },
        };
        const auto iter = STR_OPER_MODULE_MAP.find(module_str);
        if (iter == STR_OPER_MODULE_MAP.end()) {
            LogError << __FUNCTION__ << "| The current module has an unknown letter " << module_str
                     << "; skipping to the next one";
            continue;
        }
        module_items.emplace_back(ModuleItem { .rect = rect, .module = iter->second, .level = module_level });
        LogInfo << __FUNCTION__ << "| Found module " << module_str << " with level " << module_level;
    }

    return module_items;
}

std::optional<int> asst::SupportList::get_suffix_num(const std::string& s, const char delimiter)
{
    const size_t pos = s.rfind(delimiter);
    if (pos == std::string::npos) [[unlikely]] {
        Log.error(__FUNCTION__, "| Unsupported string", s);
        return std::nullopt;
    }

    std::string num_str = s.substr(pos + 1);
    if (num_str.ends_with(".png")) {
        num_str.erase(num_str.size() - 4);
    }

    int num = 0;
    if (!utils::chars_to_number(num_str, num)) [[unlikely]] {
        Log.error(__FUNCTION__, "| Failed to convert text", num_str, "to number");
        return std::nullopt;
    }

    return num;
}

std::string asst::SupportList::get_suffix_str(const std::string& s, const char delimiter)
{
    std::string str;
    if (const size_t pos = s.rfind(delimiter); pos != std::string::npos) {
        str = s.substr(pos + 1);
    }
    else [[unlikely]] {
        str = s;
    }

    if (str.ends_with(".png")) {
        str.erase(str.size() - 4);
    }

    if (const size_t pos = str.rfind('_'); pos != std::string::npos) {
        str.erase(pos);
    }

    return str;
}

bool asst::SupportList::save_img(const cv::Mat& image, const std::string_view description)
{
    return utils::save_debug_image(image, utils::path("debug") / "supportList", true, description);
}
