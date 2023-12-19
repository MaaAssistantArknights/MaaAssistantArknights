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

    private:
        int cur_page = 0;
        int max_page = 0;
        std::vector<RoguelikeFormationImageAnalyzer::FormationOper> oper_list;
    };
}
