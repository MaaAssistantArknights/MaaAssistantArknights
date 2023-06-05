#pragma once
#include "Task/AbstractTaskPlugin.h"
#include "Vision/Roguelike/RoguelikeFormationImageAnalyzer.h"

namespace asst
{
    // 集成战略模式快捷编队任务
    class RoguelikeFormationTaskPlugin : public AbstractTaskPlugin
    {
    public:
        static constexpr size_t MaxNumOfOperPerPage = 8;

    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeFormationTaskPlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;

        void clear_and_reselect();
        bool analyze();
        bool select(asst::RoguelikeFormationImageAnalyzer::FormationOper oper);

    private:
        int cur_page = 0;
        bool first_page_full = false;
        std::vector<asst::RoguelikeFormationImageAnalyzer::FormationOper> oper_list;
    };
}
