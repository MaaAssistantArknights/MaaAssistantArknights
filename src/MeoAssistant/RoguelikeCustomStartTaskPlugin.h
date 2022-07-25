#pragma once
#include "AbstractTaskPlugin.h"

namespace asst
{
    enum class RoguelikeCustomType
    {
        None,
        Squad,       // 分队类型， like 指挥分队, 矛头分队, etc
        Roles,       // 职业类型， like 先手必胜, 稳扎稳打, etc
        CoreChar,    // 首选干员， 目前的格式是 "职业:干员名"，后续可能把职业放到 core 自动判断
        //CoCoreChar,  // 次选干员， 目前的格式是 "职业:干员名"，后续可能把职业放到 core 自动判断
    };

    class RoguelikeCustomStartTaskPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeCustomStartTaskPlugin() = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;

        void set_custom(RoguelikeCustomType type, std::string custom);

    protected:
        virtual bool _run() override;

    private:
        bool hijack_squad();
        bool hijack_roles();
        bool hijack_core_char(); // not work yet

    private:
        std::unordered_map<RoguelikeCustomType, std::string> m_customs;
        mutable RoguelikeCustomType m_waiting_to_run = RoguelikeCustomType::None;
    };
}
