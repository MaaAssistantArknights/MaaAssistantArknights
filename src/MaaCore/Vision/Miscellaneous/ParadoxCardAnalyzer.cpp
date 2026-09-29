#include "ParadoxCardAnalyzer.h"

#include "Config/TaskData.h"
#include "Vision/MultiMatcher.h"
#include "Vision/OCRer.h"

std::optional<std::vector<asst::ParadoxCard>> asst::ParadoxCardAnalyzer::analyze() const
{
    std::vector<ParadoxCard> cards;
    std::vector<MatchRect> flags;
    for (int role = 1; role <= 9; ++role) {
        for (const auto& roi_task : { "OperBoxFlagRoleTopROI", "OperBoxFlagRoleBottomROI" }) {
            MultiMatcher matcher(m_image);
            matcher.set_task_info("OperBoxFlagRole" + std::to_string(role));
            matcher.set_roi(Task.get(roi_task)->roi);
            if (const auto result = matcher.analyze()) {
                for (const auto& flag : *result) {
                    // Partial cards will be covered by the next overlapping page.
                    if (flag.rect.x >= 0 && flag.rect.x + 128 <= 1145) {
                        flags.emplace_back(flag);
                    }
                }
            }
        }
    }
    flags = NMS(std::move(flags));
    sort_by_horizontal_(flags);
    for (const auto& flag : flags) {
        OCRer status(m_image);
        status.set_task_info("OperBoxParadoxCompletedOCR");
        status.set_roi(flag.rect.move(Task.get("OperBoxParadoxCompletedOCR")->roi));
        const auto result = status.analyze();
        if (!result || result->size() != 1) {
            return std::nullopt;
        }
        const auto& text = result->front().text;
        ParadoxCard box;
        box.rect = flag.rect;
        box.completed = text == "已通过";
        box.unlocked = text == "已通过" || text == "未通过";
        // Identity is deliberately empty here: the status strip covers it.
        cards.emplace_back(std::move(box));
    }
    if (cards.empty()) {
        return std::nullopt;
    }
    return cards;
}
