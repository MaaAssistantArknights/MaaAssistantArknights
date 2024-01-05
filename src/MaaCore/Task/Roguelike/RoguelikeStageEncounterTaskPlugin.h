#pragma once
#include "AbstractRoguelikeTaskPlugin.h"
#include "Config/Roguelike/RoguelikeStageEncounterConfig.h"
#include "Config/TaskData.h"
#include "Vision/OCRer.h"

namespace asst
{
    class RoguelikeStageEncounterTaskPlugin : public AbstractRoguelikeTaskPlugin
    {
    public:
        using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
        virtual ~RoguelikeStageEncounterTaskPlugin() override = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;
        static bool satisfies_condition(const asst::ChoiceRequire& requirement, const int special_val)
        {
            int num = 0;
            switch (requirement.chaos_level.type) {
            case ">":
                return utils::chars_to_number(requirement.chaos_level.value, num) && special_val > num;
            case "<":
                return utils::chars_to_number(requirement.chaos_level.value, num) && special_val < num;
            case "=":
                return utils::chars_to_number(requirement.chaos_level.value, num) && special_val == num;
            default:
                return false;
            }
        }

        static int process_task(const asst::RoguelikeEvent& event, const int special_val)
        {
            for (const auto& requirement : event.choice_require) {
                if (satisfies_condition(requirement, special_val)) {
                    return requirement.choose == -1 ? event.default_choose : requirement.choose;
                }
            }
            return event.default_choose;
        }

        static int hp(const cv::Mat& image)
        {
            int hp_val = -1;
            asst::OCRer analyzer(image);
            analyzer.set_task_info("Roguelike@HpRecognition");
            analyzer.set_replace(Task.get<OcrTaskInfo>("NumberOcrReplace")->replace_map);
            analyzer.set_use_char_model(true);

            if (!analyzer.analyze()) {
                return 0;
            }
            return utils::chars_to_number(analyzer.get_result().front().text, hp_val) ? hp_val : -1;
        }
    };
}
