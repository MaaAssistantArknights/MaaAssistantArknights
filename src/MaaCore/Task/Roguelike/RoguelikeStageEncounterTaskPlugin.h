#pragma once
#include "AbstractRoguelikeTaskPlugin.h"
#include "Config/Roguelike/RoguelikeStageEncounterConfig.h"
#include "Config/TaskData.h"
#include "Vision/OCRer.h"
#include "RoguelikeCollapsalParadigmTaskPlugin.h"

namespace asst
{
    class RoguelikeStageEncounterTaskPlugin : public AbstractRoguelikeTaskPlugin
    {
    public:
        // using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
        RoguelikeStageEncounterTaskPlugin(const AsstCallback& callback,
                                             Assistant* inst,
                                             std::string_view task_chain,
                                             std::shared_ptr<RoguelikeConfig> config)
                                             : AbstractRoguelikeTaskPlugin(callback, inst, task_chain, config)
        {
            register_plugin<RoguelikeCollapsalParadigmTaskPlugin>(config);
        }
        using Config = RoguelikeStageEncounterConfig;
        virtual ~RoguelikeStageEncounterTaskPlugin() override = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;
        std::unordered_map<RoguelikeMode, std::string_view> m_mode_tag = {
            {RoguelikeMode::Investment,  RoguelikeModeTag::Deposit},
            {RoguelikeMode::Collectible, RoguelikeModeTag::Deposit},
            {RoguelikeMode::CLP_PDS,     RoguelikeModeTag::Collapse}
        };

    protected:
        virtual bool _run() override;
        static bool satisfies_condition(const Config::ChoiceRequire& requirement, int special_val);
        static int process_task(const Config::RoguelikeEvent& event, const int special_val);
        static int hp(const cv::Mat& image);
    };
}
