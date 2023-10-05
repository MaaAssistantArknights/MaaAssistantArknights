#pragma once
#include "RoguelikeConfig.h"
#include "Task/AbstractTaskPlugin.h"

namespace asst
{
    class RoguelikeCiphertextBoardGainTaskPlugin : public AbstractTaskPlugin, public RoguelikeConfig
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeCiphertextBoardGainTaskPlugin() override = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;

    private:
        mutable bool m_ocr_after_combat = false;
    };
}
