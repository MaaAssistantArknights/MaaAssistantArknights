#pragma once
#include "../AbstractRoguelikeTaskPlugin.h"

namespace asst
{
    class RoguelikeFoldartalGainTaskPlugin : public AbstractRoguelikeTaskPlugin
    {
    public:
        using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
        virtual ~RoguelikeFoldartalGainTaskPlugin() override = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;

    private:
        void enter_next_floor();
        bool gain_stage_award();

        void gain_foldartal(std::string name);
        // 战斗后识别密文板
        mutable bool m_ocr_after_combat = false;
        // 进入新一层后识别密文板
        mutable bool m_ocr_next_level = false;
    };
}
