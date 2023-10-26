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
        json::array get_array(auto& status_string);
        bool store_to_status(std::string ciphertext_board, auto& status_string);
        // 战斗后识别密文板
        mutable bool m_ocr_after_combat = false;
        // 进入新一层后识别密文板
        mutable bool m_ocr_next_level = false;
    };
}
