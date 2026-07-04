#include "StageNavigationTask.h"

#include <boost/regex.hpp>
#include <optional>
#include <ranges>

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Task/StageNavigationHelper.h"
#include "Utils/Logger.hpp"
#include "Vision/OCRer.h"

namespace
{
enum class ChapterDifficultyMode
{
    Unsupported,
    PreStageNormalHard,
    PostStageNormalHard,
};

std::optional<int> parse_chapter_number(const std::string& chapter)
{
    try {
        return std::stoi(chapter);
    }
    catch (...) {
        return std::nullopt;
    }
}

ChapterDifficultyMode get_chapter_difficulty_mode(int chapter_num)
{
    if (chapter_num >= 10 && chapter_num <= 14) {
        return ChapterDifficultyMode::PreStageNormalHard;
    }
    if (chapter_num >= 15) {
        return ChapterDifficultyMode::PostStageNormalHard;
    }
    return ChapterDifficultyMode::Unsupported;
}
}

bool asst::StageNavigationTask::set_stage_name(const std::string& stage_name)
{
    LogTraceFunction;

    clear();

    // 已有关卡优先使用 tasks.json 中的逻辑
    if (Task.get(stage_name)) {
        m_is_directly = true;
        m_directly_task = stage_name;
        Log.info("directly task", m_directly_task);
        return true;
    }
    m_is_directly = false;

    static const boost::regex stage_regex(R"(^([A-Za-z]{0,3})(\d{1,2})-(\d{1,2})(?:-?(\w+))*$)");
    boost::smatch stage_sm;
    if (!boost::regex_match(stage_name, stage_sm, stage_regex)) {
        Log.error("The stage name is not in invalid, or is not main line stage", stage_name);
        return false;
    }

    // 10-1 -> [1] "", [2] "10", [3] "1", [4] ""
    // JT8-2 -> [1] "JT", [2] "8", [3] "2", [4] ""
    // H10-1-Hard -> [1] "H", [2] "10", [3] "1", [4] "Hard"
    const std::string& stage_prefix = stage_sm[1].str();
    const std::string& chapter = stage_sm[2].str();
    const std::string& stage_index = stage_sm[3].str();
    const std::string& difficulty = stage_sm[4].str();

    static const std::string episode_task_prefix = "Episode";
    m_chapter_task = episode_task_prefix + chapter;
    Log.info("chapter task", m_chapter_task);
    if (!Task.get(m_chapter_task)) {
        Log.error("chapter task not exists", m_chapter_task);
        return false;
    }

    if (!difficulty.empty()) {
        std::string upper_difficulty = difficulty;
        upper_difficulty[0] = static_cast<char>(::toupper(upper_difficulty[0]));
        for (size_t i = 1; i < upper_difficulty.size(); ++i) {
            upper_difficulty[i] = static_cast<char>(::tolower(upper_difficulty[i]));
        }

        const auto chapter_num = parse_chapter_number(chapter);
        if (!chapter_num.has_value()) {
            Log.error("chapter is invalid", chapter);
            return false;
        }

        const auto mode = get_chapter_difficulty_mode(*chapter_num);
        m_switch_difficulty_after_stage_selection = mode == ChapterDifficultyMode::PostStageNormalHard;
        if (mode == ChapterDifficultyMode::PreStageNormalHard) {
            if (upper_difficulty != "Hard" && upper_difficulty != "Normal") {
                Log.error("only Normal/Hard is supported for chapter 10-14", upper_difficulty);
                return false;
            }
            static const std::string difficulty_task_prefix = "ChapterDifficulty";
            m_difficulty_tasks = { difficulty_task_prefix + upper_difficulty };
        }
        else if (mode == ChapterDifficultyMode::PostStageNormalHard) {
            if (upper_difficulty == "Hard") {
                m_difficulty_tasks = { "ChangeToRaidDifficulty", "RaidConfirm" };
            }
            else if (upper_difficulty == "Normal") {
                m_difficulty_tasks = { "ChangeToNormalDifficulty", "NormalConfirm" };
            }
            else {
                Log.error("only Normal/Hard is supported for chapter 15+", upper_difficulty);
                return false;
            }
        }
        else {
            Log.error("difficulty suffix is not supported in this chapter", chapter, upper_difficulty);
            return false;
        }

        for (const auto& difficulty_task : m_difficulty_tasks) {
            Log.info("difficulty task", difficulty_task);
            if (!Task.get(difficulty_task)) {
                Log.error("difficulty task not exists", difficulty_task);
                return false;
            }
        }
    }

    std::string upper_prefix = stage_prefix;
    std::ranges::transform(upper_prefix, upper_prefix.begin(), [](const char ch) -> char {
        return static_cast<char>(::toupper(ch));
    });
    m_stage_code = upper_prefix + chapter + "-" + stage_index;
    Log.info("stage code", m_stage_code);

    return true;
}

bool asst::StageNavigationTask::_run()
{
    LogTraceFunction;

    if (m_is_directly) {
        ProcessTask task(*this, { m_directly_task });
        task.set_retry_times(RetryTimesDefault);
        bool ret = task.run();
        if (!ret && task.get_last_task_name().empty() && m_directly_task.ends_with(AnnihilationSuffix)) {
            m_fight_task_ptr->set_enable(false);
            return true;
        }
        else if (ret && task.get_last_task_name().ends_with("Annihilation@UnableToAgent2")) {
            return false;
        }
        else {
            return ret;
        }
    }

    return chapter_wayfinding() && swipe_and_find_stage() && switch_difficulty_after_stage_selection();
}

void asst::StageNavigationTask::clear() noexcept
{
    m_is_directly = false;
    m_directly_task.clear();
    m_chapter_task.clear();
    m_difficulty_tasks.clear();
    m_stage_code.clear();
    m_switch_difficulty_after_stage_selection = false;
}

bool asst::StageNavigationTask::chapter_wayfinding()
{
    LogTraceFunction;

    if (!ProcessTask(*this, { m_chapter_task }).set_retry_times(RetryTimesDefault).run()) {
        return false;
    }

    if (!m_difficulty_tasks.empty() && !m_switch_difficulty_after_stage_selection) {
        return ProcessTask(*this, m_difficulty_tasks).set_retry_times(RetryTimesDefault).run();
    }

    return true;
}

bool asst::StageNavigationTask::swipe_and_find_stage()
{
    LogTraceFunction;

    // 优先检查是否存在对应活动关卡名的模板资源，如果存在则走模板匹配
    std::string templ_path = StageNavigationHelper::get_stage_template_path(m_stage_code);
    if (!templ_path.empty()) {
        Log.info("Stage template found, using template matching for", m_stage_code, ", templ:", templ_path);
        Task.get<MatchTaskInfo>(m_stage_code + "@ClickStageByTemplate")->templ_names = { templ_path + ".png" };
        Task.get<OcrTaskInfo>(m_stage_code + "@ClickedCorrectStageByTemplateOrSwipe")->text = { m_stage_code };
        return ProcessTask(
                   *this,
                   { m_stage_code + "@StageNavigationByTemplateMatchBegin" })
            .set_retry_times(RetryTimesDefault)
            .run();
    }

    // 无模板，使用 OCR 匹配
    Task.get<OcrTaskInfo>(m_stage_code + "@ClickStageName")->text = { m_stage_code };
    std::string replace_m_stage_code = m_stage_code;
    utils::string_replace_all_in_place(replace_m_stage_code, { { "-", "" } });
    Task.get<OcrTaskInfo>(m_stage_code + "@ClickedCorrectStage")->text = { m_stage_code, replace_m_stage_code };

    return ProcessTask(*this, { m_stage_code + "@ClickStageName", m_stage_code + "@StageNavigationBegin" })
        .set_retry_times(RetryTimesDefault)
        .run();
}

bool asst::StageNavigationTask::switch_difficulty_after_stage_selection()
{
    LogTraceFunction;

    if (m_difficulty_tasks.empty() || !m_switch_difficulty_after_stage_selection) {
        return true;
    }

    return ProcessTask(*this, m_difficulty_tasks).set_retry_times(RetryTimesDefault).run();
}
