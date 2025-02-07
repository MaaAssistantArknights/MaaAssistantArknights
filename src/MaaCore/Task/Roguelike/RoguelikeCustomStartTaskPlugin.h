#pragma once
#include "AbstractRoguelikeTaskPlugin.h"

namespace asst
{
enum class RoguelikeCustomType
{
    None,
    Squad,    // 分队类型， like 指挥分队, 矛头分队, etc
    Reward,   // 奖励类型， like 热水壶, 演讲稿, etc
    Roles,    // 职业类型， like 先手必胜, 稳扎稳打, etc
    CoreChar, // 首选干员， 干员名
    // CoCoreChar,  // 次选干员， 干员名
};

struct RoguelikeStartSelect // 刷开局模式下凹开局奖励选择
{
    bool hot_water = false; // 热水壶
    bool shield = false;    // 盾；傀影没盾，是生命
    bool ingot = false;     // 源石锭
    bool hope = false;      // 希望
    bool random = false;    // 随机奖励
    bool key = false;       // 钥匙
    bool dice = false;      // 骰子
    bool ideas = false;     // 构想
};

class RoguelikeCustomStartTaskPlugin : public AbstractRoguelikeTaskPlugin
{
public:
    using AbstractRoguelikeTaskPlugin::AbstractRoguelikeTaskPlugin;
    virtual ~RoguelikeCustomStartTaskPlugin() override = default;

public:
    virtual bool verify(AsstMsg msg, const json::value& details) const override;
    virtual bool load_params([[maybe_unused]] const json::value& params) override;
    void set_custom(RoguelikeCustomType type, std::string custom);

protected:
    virtual bool _run() override;

private:
    bool hijack_squad();
    bool hijack_reward();
    bool hijack_roles();
    bool hijack_core_char();
    std::vector<std::string> get_select_list() const;

private:
    RoguelikeStartSelect m_start_select; // 开局选择
    std::unordered_map<RoguelikeCustomType, std::string> m_customs;
    mutable RoguelikeCustomType m_waiting_to_run = RoguelikeCustomType::None;

    std::string m_squad;
    std::string m_collectible_mode_squad;
};
}
