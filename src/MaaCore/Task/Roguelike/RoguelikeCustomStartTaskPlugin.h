#pragma once
#include "AbstractRoguelikeTaskPlugin.h"

namespace asst
{
    enum class RoguelikeCustomType
    {
        None,
        Squad,              // 分队类型， like 指挥分队, 矛头分队, etc
        Roles,              // 职业类型， like 先手必胜, 稳扎稳打, etc
        CoreChar,           // 首选干员， 干员名
        UseSupport,         // 使用助战
        UseSupportMinLevel, // 助战干员最低等级限制
        UseNonfriendSupport // 可以使用非好友助战
        // CoCoreChar,  // 次选干员， 干员名
    };

    class RoguelikeCustomStartTaskPlugin : public AbstractRoguelikeTaskPlugin
    {
    public:
        using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
        virtual ~RoguelikeCustomStartTaskPlugin() override = default;

    public:
        virtual bool verify(AsstMsg msg, const json::value& details) const override;
        void set_custom(RoguelikeCustomType type, std::string custom);

    protected:
        virtual bool _run() override;

    private:
        bool hijack_squad();
        bool hijack_roles();
        bool hijack_core_char();

    private:
        std::unordered_map<RoguelikeCustomType, std::string> m_customs;
        mutable RoguelikeCustomType m_waiting_to_run = RoguelikeCustomType::None;
    };
}
