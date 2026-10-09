#include "ParadoxDetailAnalyzer.h"
#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Vision/OCRer.h"
#include <unordered_set>

std::optional<std::string> asst::ParadoxDetailAnalyzer::analyze() const
{
    // The small English name is readable even when the decorative Chinese
    // lettering is not. Require a complete canonical name, never a substring.
    OCRer ocr(m_image);
    ocr.set_task_info("ParadoxDetailOperatorNameEN");
    const auto result = ocr.analyze();
    if (!result) {
        return std::nullopt;
    }
    std::unordered_set<std::string> names;
    for (const auto& text : *result) {
        if (text.score < 0.85) {
            continue;
        }
        for (const auto& [id, oper] : BattleData.get_all_chars()) {
            if (oper && id.starts_with("char_") && oper->role != battle::Role::Drone && text.text == oper->name_en) {
                names.emplace(oper->name);
            }
        }
    }
    if (names.size() != 1) {
        return std::nullopt;
    }
    return *names.begin();
}
