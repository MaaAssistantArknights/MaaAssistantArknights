#pragma once

#include <memory>
#include <string_view>
#include <utility>

#include "AbstractRoguelikeTaskPlugin.h"
#include "Config/Roguelike/RoguelikeStageEncounterConfig.h"
#include "Vision/Roguelike/RoguelikeEncounterOptionAnalyzer.h"

namespace asst::blackflow
{
class BlackFlowSession;
struct EncounterRule;
}

namespace asst
{
class RoguelikeStageEncounterTaskPlugin : public AbstractRoguelikeTaskPlugin
{
public:
    using Config = RoguelikeStageEncounterConfig;
    using OptionAnalyzer = RoguelikeEncounterOptionAnalyzer;

    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeStageEncounterTaskPlugin() override = default;

    virtual bool verify(AsstMsg msg, const json::value& details) const override;

    void set_blackflow_session(std::shared_ptr<blackflow::BlackFlowSession> session)
    {
        m_blackflow_session = std::move(session);
    }

protected:
    virtual bool _run() override;

    std::optional<std::string> handle_single_event(const std::string& event_name);
    static bool satisfies_condition(const Config::ChoiceRequire& requirement, int special_val);
    static size_t process_task(const Config::RoguelikeEvent& event, int special_val);
    int hp(const cv::Mat& image) const;

private:
    // 一次选择所需的全部配置，由事件默认配置或命中的策略规则生成。
    struct SelectionPlan
    {
        std::vector<std::string> option_text;
        size_t option_num = 0;
        size_t choose = 0; // 从 1 开始编号，0 表示未指定。
        std::vector<std::pair<size_t, size_t>> fallback_choices;
        bool allow_fallback = true;
        bool continue_single_option = false;
    };

    struct SelectedOption
    {
        size_t index = 0;
        bool used_fallback = false;
    };

    static SelectionPlan plan_from_event(const Config::RoguelikeEvent& event, size_t choose_option);
    static SelectionPlan plan_from_rule(const blackflow::EncounterRule& rule);

    bool update_option_list();
    std::optional<std::string> select_blackflow_option(const Config::RoguelikeEvent& event, size_t choose_option);
    std::optional<SelectedOption> select_event_option(const SelectionPlan& plan, std::string_view event_name);
    bool select_analyzed_option(size_t index);
    void report_selected_option(
        const Config::RoguelikeEvent& event,
        const SelectedOption& selected,
        const blackflow::EncounterRule* rule);
    void set_blackflow_result(std::string_view base_task);
    void reset_option_list_and_view_data();
    void report_analyzed_options();
    void update_view(const cv::Mat& image = cv::Mat());
    void reset_view();
    void move_to_analyzed_option(size_t index);
    void move_to_option_list_head();
    void move_forward();
    void move_backward();

    std::optional<std::string> next_event(const Config::RoguelikeEvent& event);
    std::optional<std::string> continue_blackflow_event(const Config::RoguelikeEvent& event);

    static bool save_img(const cv::Mat& image, std::string_view description = "image");

    std::shared_ptr<blackflow::BlackFlowSession> m_blackflow_session;
    std::string m_reported_event_name;
    OptionAnalyzer::Result m_option_list;
    size_t m_view_begin = 0;
    size_t m_view_end = 0;
    std::vector<int> m_option_y_in_view;
    std::vector<Rect> m_option_rect_in_view;

    static constexpr size_t MAX_SWIPE_TIMES = 1;

    static constexpr int UNDEFINED = -1;
};
}
