#include "ParadoxListTask.h"

#include <unordered_set>

#include "Config/Miscellaneous/BattleDataConfig.h"
#include "Controller/Controller.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "Task/ProcessTask.h"
#include "Vision/Miscellaneous/ParadoxCardAnalyzer.h"
#include "Vision/Miscellaneous/ParadoxDetailAnalyzer.h"

bool asst::ParadoxListTask::return_to_list()
{
    if (!ProcessTask(*this, { "ParadoxReturnOperListFlag" }).set_retry_times(0).run() &&
        !ProcessTask(*this, { "ParadoxReturnUntilOperList" }).set_retry_times(3).run()) {
        return false;
    }
    return ProcessTask(*this, { "BattleQuickFormationExpandRole" }).set_retry_times(3).run();
}

bool asst::ParadoxListTask::prepare()
{
    if (!ProcessTask(*this, { "ParadoxReturnOperListFlag" }).set_retry_times(0).run()) {
        if (!ProcessTask(*this, { "OperBoxBegin" }).set_retry_times(1).run() && !return_to_list()) {
            return false;
        }
    }
    if (!ProcessTask(*this, { "BattleQuickFormationExpandRole" }).set_retry_times(3).run() ||
        !ProcessTask(*this, { "BattleQuickFormationRole-All", "BattleQuickFormationRole-All-OCR" }).run()) {
        return false;
    }
    // Checking the selected tab avoids toggling its sorting direction.
    if (!ProcessTask(*this, { "OperBoxParadoxSelected" }).set_retry_times(0).run() &&
        !ProcessTask(*this, { "OperBoxOpenParadoxMenu" }).set_retry_times(3).run()) {
        return false;
    }
    return rewind();
}

bool asst::ParadoxListTask::same_page(const cv::Mat& before, const cv::Mat& after) const
{
    if (before.empty() || after.empty() || before.size() != after.size()) {
        return false;
    }
    // Exclude the menu and the scrollbar; compare the actual operator cards.
    const cv::Rect grid(20, 80, 1100, 620);
    if (before.cols < grid.x + grid.width || before.rows < grid.y + grid.height) {
        return false;
    }
    cv::Mat difference;
    cv::absdiff(before(grid), after(grid), difference);
    const auto mean = cv::mean(difference);
    return (mean[0] + mean[1] + mean[2]) / 3 < 1.0;
}

bool asst::ParadoxListTask::rewind()
{
    int unchanged = 0;
    for (int page = 0; page < 100 && !need_exit(); ++page) {
        const auto before = ctrler()->get_image();
        if (!ProcessTask(*this, { "OperBoxSwipeToTheLeft" }).run() || !sleep(500)) {
            return false;
        }
        unchanged = same_page(before, ctrler()->get_image()) ? unchanged + 1 : 0;
        if (unchanged >= 2) {
            return true;
        }
    }
    return false;
}

std::string asst::ParadoxListTask::detail_name()
{
    for (int retry = 0; retry < 3 && !need_exit(); ++retry) {
        if (!sleep(500)) {
            return {};
        }
        ParadoxDetailAnalyzer analyzer(ctrler()->get_image());
        if (const auto name = analyzer.analyze()) {
            return *name;
        }
    }
    save_img(utils::path("debug") / utils::path("paradox"));
    return {};
}

bool asst::ParadoxListTask::_run()
{
    m_result.clear();
    m_completed = false;
    if (m_candidates.empty() || !prepare()) {
        return false;
    }
    int unchanged = 0;
    for (int page = 0; page < 100 && !need_exit(); ++page) {
        ParadoxCardAnalyzer analyzer(ctrler()->get_image());
        const auto cards = analyzer.analyze();
        if (!cards) {
            return false;
        }
        for (const auto& card : *cards) {
            if (need_exit()) {
                return false;
            }
            if (!card.unlocked || (card.completed && !m_include_completed)) {
                continue;
            }
            if (!ctrler()->click(card.rect.move(Rect(20, 50, 70, 100)))) {
                return false;
            }
            const auto name = detail_name();
            if (name.empty()) {
                return false;
            }
            if (m_candidates.contains(name)) {
                // Leave the verified detail page open for skill selection.
                m_result = name;
                m_completed = card.completed;
                return true;
            }
            if (!return_to_list()) {
                return false;
            }
        }
        const auto before = ctrler()->get_image();
        if (!ProcessTask(*this, { "OperBoxSlowlySwipeToTheRight" }).run() || !sleep(500)) {
            return false;
        }
        unchanged = same_page(before, ctrler()->get_image()) ? unchanged + 1 : 0;
        if (unchanged >= 2) {
            return true;
        }
    }
    return false;
}
