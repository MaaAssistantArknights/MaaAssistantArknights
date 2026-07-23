#include "RoguelikeStageEncounterTaskPlugin.h"

#include "Config/Roguelike/RoguelikeStageEncounterConfig.h"
#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "MaaUtils/ImageIo.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Task/ProcessTask.h"
#include "Task/Roguelike/Map/RoguelikeBoskyPassageMap.h"
#include "Utils/DebugImageHelper.hpp"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/RegionOCRer.h"

bool asst::RoguelikeStageEncounterTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    // 安全屋，掷骰子之类的带选项的也都是视为不期而遇了
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (!RoguelikeConfig::is_valid_theme(m_config->get_theme())) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }
    const std::string roguelike_name = m_config->get_theme() + "@";
    const std::string& task = details.get("details", "task", "");
    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view == "Roguelike@StageEncounterJudgeOption") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeStageEncounterTaskPlugin::_run()
{
    LogTraceFunction;

    const std::string& theme = m_config->get_theme();
    const auto event_name_task_ptr = Task.get("Roguelike@StageEncounterOcr");
    sleep(event_name_task_ptr->pre_delay);

    if (need_exit()) {
        return false;
    }

    auto current_event_name = recognize_event_name(ctrler()->get_image());
    if (!current_event_name) {
        Log.error("Unknown Event");
        callback(AsstMsg::SubTaskExtraInfo, basic_info_with_what("EncounterOcrError"));
        return true;
    }

    // 处理主事件及其链式 next_event
    std::string previous_event_name;
    size_t same_event_steps = 0;
    while (current_event_name && !current_event_name->empty()) {
        if (*current_event_name == previous_event_name) {
            ++same_event_steps;
        }
        else {
            previous_event_name = *current_event_name;
            same_event_steps = 1;
        }

        const auto& event_map = RoguelikeStageEncounter.get_events(theme, m_config->get_mode());
        auto event_it = event_map.find(*current_event_name);
        if (event_it == event_map.end()) {
            Log.error("Unknown event:", *current_event_name);
            break;
        }
        if (same_event_steps > event_it->second.max_steps) {
            Log.warn("Encounter step limit reached for event:", *current_event_name);
            break;
        }

        auto next = handle_single_event(*current_event_name);
        if (!next) {
            break;
        }
        current_event_name = std::move(next);
    }

    return true;
}

std::optional<std::string> asst::RoguelikeStageEncounterTaskPlugin::recognize_event_name(const cv::Mat& image) const
{
    const std::string& theme = m_config->get_theme();
    OCRer analyzer(image);
    analyzer.set_task_info(Task.get("Roguelike@StageEncounterOcr"));
    analyzer.set_required(RoguelikeStageEncounter.get_event_names(theme));
    if (!analyzer.analyze() || analyzer.get_result().empty()) {
        return std::nullopt;
    }
    return analyzer.get_result().front().text;
}

std::optional<std::string> asst::RoguelikeStageEncounterTaskPlugin::handle_single_event(const std::string& event_name)
{
    const std::string& theme = m_config->get_theme();
    const RoguelikeMode& mode = m_config->get_mode();
    const auto& event_map = RoguelikeStageEncounter.get_events(theme, mode);

    auto it = event_map.find(event_name);
    if (it == event_map.end()) {
        Log.error("Unknown event:", event_name);
        return std::nullopt;
    }

    const auto& event = it->second;

    if (m_config->get_theme() == RoguelikeTheme::Blackflow &&
        (event.name == "险路尽头" || event.name == "三重身")) {
        m_config->set_blackflow_force_zoom_reset_after_event(true);
        Log.info("Blackflow encounter ends the layer; next map will reset zoom:", event.name);
    }

    cv::Mat image = ctrler()->get_image();

    int special_val = 0;
    // 水月的不好识别，先试试萨米能不能用
    if (m_config->get_theme() == RoguelikeTheme::Sami) {
        OCRer analyzer(image);
        analyzer.set_task_info(m_config->get_theme() + "@Roguelike@SpecialValRecognition");
        analyzer.set_replace(Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map);
        analyzer.set_use_char_model(true);
        if (!analyzer.analyze()) {
            // return std::nullopt;
            Log.error("Failed to recognize special value for event:", event.name);
        }
        else {
            utils::chars_to_number(analyzer.get_result().front().text, special_val);
        }
    }

    size_t choose_option = process_task(event, special_val);
    Log.info("Event:", event.name, "special_val", special_val, "choose option", choose_option);

    auto info = basic_info_with_what("RoguelikeEvent");
    info["details"]["name"] = event.name;
    info["details"]["default_choose"] = event.default_choose;
    info["details"]["choose_option"] = choose_option;
    callback(AsstMsg::SubTaskExtraInfo, info);

    // 萨卡兹内容拓展 II，#11861
    if (event.name.starts_with("魂灵见闻：")) {
        Matcher matcher(image);
        matcher.set_task_info("Sarkaz@Roguelike@CloseCollectionClose");
        if (matcher.analyze()) {
            Log.trace("Found extra 'Plans', click CloseCollectionClose and StageEncounterJudgeClick");
            ctrler()->click(matcher.get_result().rect);
            ProcessTask(*this, { "Roguelike@StageEncounterJudgeClick" }).run();
            ProcessTask(*this, { "Roguelike@StageEncounterJudgeClick2" }).run();
        }
    }

    // 界园树洞
    if (m_config->get_theme() == RoguelikeTheme::JieGarden) {
        auto& bosky_map = RoguelikeBoskyPassageMap::get_instance();
        if (event.name == "掷地有声") {
            bosky_map.set_node_subtype(bosky_map.get_curr_pos(), RoguelikeBoskySubNodeType::Ling);
        }
        else if (event.name == "种因得果") {
            bosky_map.set_node_subtype(bosky_map.get_curr_pos(), RoguelikeBoskySubNodeType::Shu);
        }
        else if (event.name == "三缺一") {
            bosky_map.set_node_subtype(bosky_map.get_curr_pos(), RoguelikeBoskySubNodeType::Nian);
        }

        if (bosky_map.get_target_subtype() != RoguelikeBoskySubNodeType::Unknown) {
            if (bosky_map.get_node_subtype(bosky_map.get_curr_pos()) == bosky_map.get_target_subtype()) {
                Log.info(__FUNCTION__, "| Found target playtime node, completing task and exiting");

                auto target_info = basic_info_with_what("RoguelikeJieGardenTargetFound");
                target_info["details"]["target_subtype"] = subtype2name(bosky_map.get_target_subtype());
                callback(AsstMsg::SubTaskExtraInfo, target_info);

                // 完成任务，退出
                m_control_ptr->exit_then_stop();
                m_task_ptr->set_enable(false);
                return std::nullopt;
            }
        }
    }

    // 使用 OCR 选项和分支签名选择动态事件；黑流树海禁止未知选项的盲点兜底。
    if (event.dynamic_options) {
        reset_option_list_and_view_data();
        if (update_option_list(event.name)) {
            if (auto choice = select_dynamic_option(event)) {
                if (select_analyzed_option(*choice)) {
                    return next_event(event);
                }
            }
            else {
                Log.warn(
                    "RoguelikeEncounter | No enabled safe option matched; refusing blind selection for event",
                    event.name);
                return std::nullopt;
            }

            if (theme != RoguelikeTheme::Blackflow) {
                // 界园旧配置保留从下到上的安全兜底。
                for (size_t choice = m_option_list.size(); choice > 0; --choice) {
                    if (m_option_list[choice - 1].enabled && select_analyzed_option(choice - 1)) {
                        return next_event(event);
                    }
                }
            }
            else {
                Log.error("RoguelikeEncounter | Dynamic option click failed; stopping event", event.name);
                return std::nullopt;
            }
        }
        else if (theme == RoguelikeTheme::Blackflow) {
            Log.error("RoguelikeEncounter | Failed to analyze Blackflow options; stopping event", event.name);
            return std::nullopt;
        }
    }

    if (theme == RoguelikeTheme::JieGarden) {
        // 兼容界园旧配置的动态文本选择。
        reset_option_list_and_view_data();
        if (update_option_list(event.name)) {
            size_t choice = 0;
            if (!event.option_text.empty()) {
                for (const std::string& event_text : event.option_text) {
                    const auto option_it =
                        std::ranges::find_if(m_option_list, [&event_text](const OptionAnalyzer::Option& option) {
                            return option.enabled && option.text == event_text;
                        });
                    if (option_it != m_option_list.end()) {
                        choice = std::distance(m_option_list.begin(), option_it) + 1;
                        break;
                    }
                }
            }
            else if (event.option_num == m_option_list.size()) {
                choice = choose_option;
            }
            else {
                for (const auto& [total, item] : event.fallback_choices) {
                    if (total == m_option_list.size()) {
                        choice = item;
                        break;
                    }
                }
            }
            if (choice != 0 && select_analyzed_option(choice - 1)) {
                return next_event(event);
            }

            // 兜底：从下到上依次选择
            for (choice = m_option_list.size(); choice > 0; --choice) {
                if (m_option_list[choice - 1].enabled && select_analyzed_option(choice - 1)) {
                    return next_event(event);
                }
            }
        }
    }

    const auto click_option_task_name = [&](size_t item, size_t total) {
        if (item > total) {
            Log.warn("Event:", event.name, "Total:", total, "Choice", item, "out of range, switch to choice", total);
            item = total;
        }
        return m_config->get_theme() + "@Roguelike@OptionChoose" + std::to_string(total) + "-" + std::to_string(item);
    };

    for (int j = 0; j < 2; ++j) {
        ProcessTask(*this, { click_option_task_name(choose_option, event.option_num) }).run();
        sleep(300);
    }

    sleep(1500);

    // 判断是否点击成功，成功进入对话后左上角的生命值会消失
    image = ctrler()->get_image();
    bool hp_disappeared = (hp(image) < 0);
    // fallback 可变选项，临时处理，之后还得改成更通用的方式
    if (!hp_disappeared) {
        for (const auto& [total, item] : event.fallback_choices) {
            Log.info("Trying fallback choice", total, "-", item);
            for (int j = 0; j < 2; ++j) {
                ProcessTask(*this, { click_option_task_name(item, total) }).run();
                sleep(300);
            }
            sleep(500);
            image = ctrler()->get_image();
            if (hp(image) < 0) {
                Log.info("Fallback choice success");
                hp_disappeared = true;
                break;
            }
        }
    }

    if (hp_disappeared) {
        return next_event(event);
    }

    // 兜底处理，从 option_num-option_num 点到 1-1
    size_t max_time = event.option_num;
    while (max_time > 0) {
        for (size_t i = max_time; i > 0; --i) {
            bool ret =
                ProcessTask(*this, { "Roguelike@TutorialButton" }).set_reusable_image(image).set_retry_times(3).run();
            if (ret) {
                return std::nullopt;
            }

            for (int j = 0; j < 2; ++j) {
                ProcessTask(*this, { click_option_task_name(i, max_time) }).run();
                sleep(300);
            }

            if (need_exit()) {
                return std::nullopt;
            }

            sleep(500);
            image = ctrler()->get_image();
            if (hp(image) < 0) {
                return std::nullopt;
            }
        }
        --max_time;
    }

    return event.next_event.empty() ? std::nullopt : std::optional { event.next_event };
}

bool asst::RoguelikeStageEncounterTaskPlugin::satisfies_condition(
    const Config::ChoiceRequire& requirement,
    const int special_val)
{
    int value = 0;
    bool ret = utils::chars_to_number(requirement.vision.value, value);
    Log.trace("special_val: ", special_val, "value: ", value);
    switch (requirement.vision.type) {
    case Config::ComparisonType::GreaterThan:
        ret &= special_val > value;
        Log.trace("special_val > value: ", special_val > value ? "true" : "false");
        break;
    case Config::ComparisonType::LessThan:
        ret &= special_val < value;
        Log.trace("special_val < value: ", special_val < value ? "true" : "false");
        break;
    case Config::ComparisonType::Equal:
        ret &= special_val == value;
        Log.trace("special_val == value: ", special_val == value ? "true" : "false");
        break;
    case Config::ComparisonType::None:
        Log.warn("no vision type");
        break;
    case Config::ComparisonType::Unsupported:
        Log.warn("unsupported vision type");
        return false;
    }
    /*
    switch (requirement.hp.type) {
        // ...
    }
    */
    if (!ret) {
        return false;
    }
    return true;
}

size_t asst::RoguelikeStageEncounterTaskPlugin::process_task(const Config::RoguelikeEvent& event, const int special_val)
{
    for (const auto& requirement : event.choice_require) {
        if (requirement.choose == -1) {
            continue;
        }
        if (satisfies_condition(requirement, special_val)) {
            return requirement.choose;
        }
    }
    return event.default_choose;
}

int asst::RoguelikeStageEncounterTaskPlugin::hp(const cv::Mat& image) const
{
    LogTraceFunction;

    if (!ProcessTask(*this, { "Roguelike@HpFlag" }).run()) {
        Log.info("Not found HpFlag");
        return -1;
    }

    auto task = Task.get<OcrTaskInfo>("Roguelike@HpRecognition");
    std::vector<std::pair<std::string, std::string>> merged_map;
    merged_map.insert(merged_map.end(), task->replace_map.begin(), task->replace_map.end());
    merged_map.emplace_back("(.*)/.*", "$1");

    auto roi_image = make_roi(image, task->roi).clone();
    cv::Mat r_channel;
    cv::extractChannel(roi_image, r_channel, 2);
    cv::Mat mask;
    cv::threshold(r_channel, mask, 50, 255, cv::THRESH_BINARY);
    cv::Mat inv_mask;
    cv::bitwise_not(mask, inv_mask);
    roi_image.setTo(cv::Scalar(0, 0, 0), mask);

    RegionOCRer analyzer(roi_image);
    analyzer.set_replace(merged_map);
    analyzer.set_use_char_model(true);
    analyzer.set_bin_threshold(60); // 血量没有红色通道，虽然它看着很明显，但实际上在灰度中只有 2/3

    auto res_vec_opt = analyzer.analyze();
    if (!res_vec_opt) {
        return 0;
    }

    int hp_val;
    return utils::chars_to_number(res_vec_opt->text, hp_val) ? hp_val : 0;
}

bool asst::RoguelikeStageEncounterTaskPlugin::update_option_list(const std::string& event_name)
{
    LogTraceFunction;

    const std::string& theme = m_config->get_theme();

    // 不期而遇默认位置会在选项列表中央, 为了从上到下检视选项列表, 需要先向上滑动
    move_to_option_list_head();

    cv::Mat image = ctrler()->get_image();
    RoguelikeEncounterOptionAnalyzer analyzer(image);
    analyzer.set_theme(theme);
    for (size_t swipe_times = 0; swipe_times < MAX_SWIPE_TIMES && !need_exit(); ++swipe_times) {
        move_forward();
        image = ctrler()->get_image();
        const std::optional<int> ret = analyzer.merge_image(image);
        if (!ret) {
            return false;
        }
        if (ret.value() <= 0) {
            break;
        }
    }

    if (!analyzer.analyze()) {
        return false;
    }

    m_option_list = analyzer.get_result();

    // 黑流树海“得偿所愿-无人商店”中，OCR 偶尔会把首字误识为“二”，
    // 形成“二搬一个大桶”，导致配置中的“搬一个大桶”无法命中。
    if (m_config->get_theme() == RoguelikeTheme::Blackflow && event_name == "无人商店") {
        for (auto& option : m_option_list) {
            if (option.text == "二搬一个大桶") {
                Log.info("RoguelikeEncounter | Corrected Blackflow option OCR: 二搬一个大桶 -> 搬一个大桶");
                option.text = "搬一个大桶";
            }
        }
    }
    report_analyzed_options();

    update_view(image);

    return true;
}

std::optional<size_t> asst::RoguelikeStageEncounterTaskPlugin::select_dynamic_option(
    const Config::RoguelikeEvent& event)
{
    std::vector<RoguelikeEncounterOptionSelector::VisibleOption> options;
    options.reserve(m_option_list.size());
    for (const auto& option : m_option_list) {
        options.emplace_back(option.enabled, option.text);
    }

    const auto result = RoguelikeEncounterOptionSelector::select(
        options,
        event.option_rule,
        event.prefer_event_rule ? std::vector<RoguelikeEncounterOptionSelector::Rule> {} : event.option_variants);
    if (!result.index) {
        Log.warn(
            "RoguelikeEncounter | No configured safe choice for",
            event.name,
            result.matched_variant ? result.rule_id : "default");
        return std::nullopt;
    }

    Log.info(
        "RoguelikeEncounter | Selected",
        event.name,
        "option",
        *result.index + 1,
        result.used_safe_fallback ? "(safe fallback)" : "(preferred)");
    return result.index;
}

bool asst::RoguelikeStageEncounterTaskPlugin::select_analyzed_option(size_t index)
{
    LogTraceFunction;

    // sanity check
    if (index >= m_option_list.size()) [[unlikely]] {
        Log.error(
            __FUNCTION__,
            std::format("| Attempt to select option {} out of {}", index + 1, m_option_list.size()));
        return false;
    }
    if (!m_option_list[index].enabled) {
        Log.info(__FUNCTION__, std::format("| Attempt to select disabled option {}", index + 1));
        return false;
    }

    move_to_analyzed_option(index);

    // click option
    Log.info(__FUNCTION__, std::format("| Clicking option {}: {}", index + 1, m_option_list[index].text));
    Rect click_rect = Task.get(m_config->get_theme() + "@RoguelikeEncounter-ClickOption")->specific_rect;
    click_rect.y = m_option_y_in_view[index];
    for (int j = 0; j < 2; ++j) {
        ctrler()->click(click_rect);
        sleep(300);
    }
    sleep(1500);

    if (hp(ctrler()->get_image()) < 0) {
        return true;
    }

    Log.error(__FUNCTION__, "| The option doesn't respond to click");
    save_img(ctrler()->get_image(), "current screenshot");

    return false;
}

void asst::RoguelikeStageEncounterTaskPlugin::reset_option_list_and_view_data()
{
    m_option_list.clear();
    reset_view();
}

void asst::RoguelikeStageEncounterTaskPlugin::report_analyzed_options()
{
    std::vector<json::value> options;

    Log.info("Analyzed Options");
    Log.info(std::string(40, '-'));
    Log.info(std::format("{:^9} | {}", "Enabled", "Text"));
    Log.info(std::string(40, '-'));
    for (const auto& [enabled, templ, text] : m_option_list) {
        json::value option = json::object {
            { "enabled", enabled },
            { "text", text },
        };
        options.emplace_back(std::move(option));
        Log.info(std::format("{:^9} | {}", enabled ? "Y" : "N", text));
    }
    Log.info(std::string(40, '-'));

    json::value info = basic_info_with_what("RoguelikeEncounterOptions");
    info["details"]["options"] = std::move(options);
    callback(AsstMsg::SubTaskExtraInfo, info);
}

void asst::RoguelikeStageEncounterTaskPlugin::update_view(const cv::Mat& image)
{
    LogTraceFunction;

    reset_view();

    cv::Mat img = image.empty() ? ctrler()->get_image() : image;

    for (size_t i = 0; i < m_option_list.size(); ++i) {
        if (const Matcher::ResultOpt option_match_ret =
                RoguelikeEncounterOptionAnalyzer::match_option(m_config->get_theme(), img, m_option_list[i].templ);
            option_match_ret) {
            if (i < m_view_begin) {
                m_view_begin = i;
            }
            if (i >= m_view_end) {
                m_view_end = i + 1;
            }
            m_option_y_in_view[i] = option_match_ret.value().rect.y;
        }
    }

    Log.info(__FUNCTION__, std::format("| Current view is [{}, {}]", m_view_begin + 1, m_view_end));
}

void asst::RoguelikeStageEncounterTaskPlugin::reset_view()
{
    m_view_begin = m_option_list.size();
    m_view_end = 0;
    m_option_y_in_view.assign(m_option_list.size(), UNDEFINED);
}

void asst::RoguelikeStageEncounterTaskPlugin::move_to_analyzed_option(size_t index)
{
    LogTraceFunction;

    // sanity check
    if (index >= m_option_list.size()) [[unlikely]] {
        Log.error(
            __FUNCTION__,
            std::format("| Attempt to move to option {} out of {}", index + 1, m_option_list.size()));
        return;
    }

    Log.info(__FUNCTION__, std::format("Moving to option {}: {}", index + 1, m_option_list[index].text));

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
        if (m_option_y_in_view[index] == UNDEFINED) {
            Log.error(__FUNCTION__, "| y for option {} in view is not updated", index + 1);
            save_img(image, "lastly used screenshot");
            save_img(m_option_list[index].templ, "option template");
            image = ctrler()->get_image();
            update_view(image);
            continue;
        }
        break;
    }
}

void asst::RoguelikeStageEncounterTaskPlugin::move_to_option_list_head()
{
    LogTraceFunction;

    ProcessTask(*this, { m_config->get_theme() + "@RoguelikeEncounter-InitialMoveUp" }).run();
}

void asst::RoguelikeStageEncounterTaskPlugin::move_forward()
{
    LogTraceFunction;

    ProcessTask(*this, { m_config->get_theme() + "@RoguelikeEncounter-MoveDown" }).run();
}

void asst::RoguelikeStageEncounterTaskPlugin::move_backward()
{
    LogTraceFunction;

    ProcessTask(*this, { m_config->get_theme() + "@RoguelikeEncounter-MoveUp" }).run();
}

std::optional<std::string> asst::RoguelikeStageEncounterTaskPlugin::next_event(const Config::RoguelikeEvent& event)
{
    LogTraceFunction;

    // Blackflow marks ordinary OCR-selectable encounters as dynamic_options,
    // but they do not have a follow-up event. Once the option is selected and the
    // map is visible again, click the map's "show player" button to clear the
    // selected option state. This position is fixed and harmless to click, so do
    // not use a nested ProcessTask here: this plugin intentionally has retry_times=0.
    // StageEncounterJudgeClick's fixed coordinate can hit a map node and consume
    // action points.
    if (event.next_event.empty() && m_config->get_theme() == RoguelikeTheme::Blackflow) {
        constexpr Point show_player_button { 55, 515 };
        ctrler()->click(show_player_button);
        sleep(800);
        Log.debug("Blackflow encounter finished; clicked map show-player button at", show_player_button.x, show_player_button.y);
        return std::nullopt;
    }

    if (event.next_event.empty() && !event.dynamic_options) {
        return std::nullopt;
    }

    const auto& task = Task.get("Roguelike@StageEncounterJudgeClick");
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 2; ++j) {
            ctrler()->click(task->specific_rect);
            sleep(500);
        }
        if (hp(ctrler()->get_image()) >= 0) {
            // 多点一次，确保选项恢复
            ctrler()->click(task->specific_rect);
            sleep(500);
            if (!event.next_event.empty()) {
                Log.debug("HP restored, going to next_event:", event.next_event);
                return event.next_event;
            }

            if (auto reocr_event = recognize_event_name(ctrler()->get_image())) {
                Log.debug("Re-OCR same-title/multi-stage encounter:", reocr_event.value());
                return reocr_event;
            }
            return std::nullopt;
        }
    }

    return std::nullopt;
}

bool asst::RoguelikeStageEncounterTaskPlugin::save_img(const cv::Mat& image, const std::string_view description)
{
    return utils::save_debug_image(image, utils::path("debug") / "roguelike" / "encounter", true, description);
}
