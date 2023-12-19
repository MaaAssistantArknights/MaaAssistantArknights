#include "RoguelikeConfig.h"

void asst::RoguelikeConfig::clear()
{
    m_recruitment_count = 0;
    m_recruitment_starts_complete = false;
    m_recruitment_team_complete = false;
    m_trader_no_longer_buy = false;
    m_core_char = std::string();
    m_team_full_without_rookie = false;
    m_use_support = false;
    m_oper = std::unordered_map<std::string, RoguelikeOper>();

    m_foldartal_floor.reset();
    m_foldartal = std::vector<std::string>();
}
