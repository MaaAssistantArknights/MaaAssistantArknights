#pragma once

#include "AbstractRoguelikeTaskPlugin.h"
#include "Vision/Roguelike/RoguelikeFormationImageAnalyzer.h"

namespace asst
{
    // 集成战略模式快捷编队任务
    class RoguelikeFormationTaskPlugin : public AbstractRoguelikeTaskPlugin
    {
    public:
        static constexpr size_t MaxNumOfOperPerPage = 8;

    public:
        using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
        virtual ~RoguelikeFormationTaskPlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;

        void clear_and_reselect();
        bool analyze();
        bool select(RoguelikeFormationImageAnalyzer::FormationOper oper);
        void swipe_to_first_page();

    private:
        int cur_page = 1;
        int max_page = 0;
        std::vector<RoguelikeFormationImageAnalyzer::FormationOper> oper_list;
        std::vector<std::string> m_last_detected_oper_names; // 上一页识别到的干员
    };
}
