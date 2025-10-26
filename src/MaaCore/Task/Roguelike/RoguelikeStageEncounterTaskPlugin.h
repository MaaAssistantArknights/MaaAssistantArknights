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
    static int process_task(const Config::RoguelikeEvent& event, int special_val);
    int hp(const cv::Mat& image) const;

private:
    void reset_option_analysis_data();
    bool analyze_options(const std::string& theme);
    void report_analyzed_options() const;
    bool option_analyzed() const;
    bool select_analyzed_option(size_t index);

    std::optional<std::string> next_event(const Config::RoguelikeEvent& event);

    static bool save_img(const cv::Mat& image, std::string_view description = "image");

    OptionAnalyzer::Result m_analyzed_options;
    int m_num_analyzed_options = 0;
    cv::Mat m_merged_option_image;

    int MAX_SWIPE_TIMES = 1;
};
}
