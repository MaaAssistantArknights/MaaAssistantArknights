#include "RoguelikeConfig.h"

void asst::RoguelikeConfig::clear() {
    m_recruitment_count = 0;
    m_recruitment_starts_complete = false;
    m_recruitment_team_complete = false;
    m_trader_no_longer_buy = false;
    m_core_char = std::string();
    m_team_full_without_rookie = false;
<<<<<<< HEAD
    m_foldartal_floor.reset();
    m_foldartal.clear();
=======
>>>>>>> parent of 94439e87c (refactor: 重构肉鸽每一层的预见密文板获取)
}
