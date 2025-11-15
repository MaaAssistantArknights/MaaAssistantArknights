#pragma once
#include "AbstractRoguelikeTaskPlugin.h"
#include "Config/Roguelike/RoguelikeStageEncounterConfig.h"
#include "Vision/Roguelike/RoguelikeEncounterOptionAnalyzer.h"

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

protected:
    virtual bool _run() override;

    std::optional<std::string> handle_single_event(const std::string& event_name);
    static bool satisfies_condition(const Config::ChoiceRequire& requirement, int special_val);
    static size_t process_task(const Config::RoguelikeEvent& event, int special_val);
    int hp(const cv::Mat& image) const;

private:
    bool update_option_list();
    bool select_analyzed_option(size_t index);
    void reset_option_list_and_view_data();
    void report_analyzed_options();
    void update_view(const cv::Mat& image = cv::Mat());
    void reset_view();
    void move_to_analyzed_option(size_t index);
    void move_to_option_list_head();
    void move_forward();
    void move_backward();

    std::optional<std::string> next_event(const Config::RoguelikeEvent& event);

    static bool save_img(const cv::Mat& image, std::string_view description = "image");

    OptionAnalyzer::Result m_option_list;
    size_t m_view_begin = 0;
    size_t m_view_end = 0;
    std::vector<int> m_option_y_in_view;

    static constexpr size_t MAX_SWIPE_TIMES = 1;

    static constexpr int UNDEFINED = -1;
};
}
