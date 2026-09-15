#include "RoguelikeDifficultySelectionTaskPlugin.h"

#include "Config/TaskData.h"
#include "Controller/Controller.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Vision/Matcher.h"
#include "Vision/OCRer.h"

bool asst::RoguelikeDifficultySelectionTaskPlugin::load_params([[maybe_unused]] const json::value& params)
{
    const RoguelikeMode mode = m_config->get_mode();

    // 深入调查和月度小队模式不需要选择难度
    if (mode == RoguelikeMode::Exploration || mode == RoguelikeMode::Squad) {
        return false;
    }

    // 为刷开局模式设置专用难度
    if (mode == RoguelikeMode::Collectible) {
        if (m_config->get_difficulty() == -1) {
            m_collectible_difficulty = -1; // 当前难度只能用当前难度烧水
        }
        else {
            const std::string& theme = m_config->get_theme();
            const std::string& squad = params.get("squad", "");
            const std::string& collectible_mode_squad = params.get("collectible_mode_squad", squad);
            if (theme == RoguelikeTheme::JieGarden && collectible_mode_squad == "指挥分队" &&
                // 界园指挥分队可用 3 难快速烧水
                m_config->get_difficulty() >= 3) {
                m_collectible_difficulty = 3;
            }
            else {
                m_collectible_difficulty = 0;
            }
        }
    }

    /*auto opt = params.find<int>("difficulty");
    return opt && *opt != -1;*/
    return true;
}

bool asst::RoguelikeDifficultySelectionTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskStart || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (!RoguelikeConfig::is_valid_theme(m_config->get_theme())) {
        Log.error("Roguelike name doesn't exist!");
        return false;
    }

    const std::string roguelike_name = m_config->get_theme() + "@";
    const std::string& task = details.get("details", "task", "");

    std::string_view task_view = task;
    if (task_view.starts_with(roguelike_name)) {
        task_view.remove_prefix(roguelike_name.length());
    }
    if (task_view.ends_with("Roguelike@GamePass")) {
        m_has_changed = false;
    }
    if (task_view == "Roguelike@StartExplore") { // 烧水时候调来调去的干脆不走
        return m_config->get_mode() == RoguelikeMode::Collectible || !m_has_changed;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeDifficultySelectionTaskPlugin::_run()
{
    LogTraceFunction;

    if (m_config->get_run_for_collectible()) {
        Log.info(__FUNCTION__, "| Running for collectible");
    }

    const int difficulty = m_config->get_run_for_collectible() ? m_collectible_difficulty : m_config->get_difficulty();
    Log.info(__FUNCTION__, "| current_difficulty:", m_current_difficulty, "next difficulty:", difficulty);

    // 仅在插件记录的当前难度与目标难度不一致时重新选择难度
    select_difficulty(difficulty);

    m_has_changed = true;
    return true;
}

int asst::RoguelikeDifficultySelectionTaskPlugin::detect_current_difficulty() const
{
    LogTraceFunction;

    const cv::Mat image = ctrler()->get_image();
    OCRer analyzer(image);
    analyzer.set_task_info("Roguelike@ChooseDifficulty_AnalyzeCurrentDifficulty");
    if (analyzer.analyze()) {
        const std::string text = analyzer.get_result().front().text;
        Log.info("Detected difficulty text:", text);
        int difficulty;
        if (!utils::chars_to_number(text, difficulty)) {
            Log.error("Failed to convert difficulty text to number. Text =", text);
            return -1;
        }
        return difficulty;
    }
    else {
        Log.error("OCR failed. Cannot detect difficulty.");
        return -1;
    }
}

bool asst::RoguelikeDifficultySelectionTaskPlugin::select_difficulty(const int difficulty)
{
    LogTraceFunction;

    const std::string& theme = m_config->get_theme();
    static std::unordered_set<std::string> initialized_themes;

    if (!initialized_themes.contains(theme)) {
        ProcessTask(*this, { theme + "@Roguelike@ChooseDifficultyEnter" }).run();

        // 第一次运行肉鸽或者重装游戏后没有难度按钮，先判断下有没有 Confirm，没有就点一下 StartExplore
        OCRer confirm_matcher(ctrler()->get_image());
        confirm_matcher.set_task_info(theme + "@Roguelike@ChooseDifficultyConfirm");
        if (!confirm_matcher.analyze()) {
            Log.warn("Failed to find difficulty selection UI. Try to click Roguelike@StartExplore.");
            Matcher start_explore_matcher(ctrler()->get_image());
            start_explore_matcher.set_task_info(theme + "@Roguelike@StartExplore");
            if (start_explore_matcher.analyze()) {
                ctrler()->click(start_explore_matcher.get_result().rect);
            }
            else {
                Log.error("Failed to find Roguelike@StartExplore button. Cannot proceed with difficulty selection.");
                return false;
            }
        }

        if (difficulty == m_current_difficulty) {
            Log.info("Current difficulty is already set to the target difficulty:", difficulty);
            ProcessTask(*this, { theme + "@Roguelike@ChooseDifficultyConfirm" }).run();
            initialized_themes.insert(theme);
            return true;
        }
        initialized_themes.insert(theme);
    }
    else {
        if (difficulty == m_current_difficulty) {
            Log.info("Current difficulty is already set to the target difficulty:", difficulty);
            return true;
        }
        ProcessTask(*this, { theme + "@Roguelike@ChooseDifficultyEnter" }).run();
    }

    if (difficulty == INT_MAX) {
        // 最高难度：循环下滑直到识别出的难度不再变化（到达列表底部）。
        // 原实现固定滑动两屏，难度列表较长时到不了底部，会以列表
        // 中途的难度确认（如从低难度出发两次滑动只到 N15）。
        // 收敛只认数值读数：识别失败（-1）既不累计也不重置稳定性
        static constexpr int MaxScrollTimes = 8;
        int last_difficulty = INT_MIN;
        int stable_count = 0; // 连续无变化的滑动次数（需三次相同读数）
        bool converged = false;
        for (int i = 0; i < MaxScrollTimes && !need_exit(); ++i) {
            m_current_difficulty = detect_current_difficulty();
            if (m_current_difficulty >= 0) {
                if (m_current_difficulty == last_difficulty) {
                    if (++stable_count >= 2) {
                        converged = true;
                        break;
                    }
                }
                else {
                    stable_count = 0;
                    last_difficulty = m_current_difficulty;
                }
            }
            if (!ProcessTask(*this, { "SwipeToTheDown" }).run() || !sleep(300)) {
                LogError << "Task stopped during difficulty selection.";
                return false;
            }
        }
        m_current_difficulty = detect_current_difficulty();
        if (converged) {
            LogInfo << "Reached bottom of difficulty list, current: " << m_current_difficulty;
        }
        else {
            LogWarn << "Did not converge to the highest difficulty, using current: " << m_current_difficulty;
        }
    }
    else if (difficulty == 0) {
        // 最低难度：循环上滑直到到达列表顶部，与最高难度对称。
        // 原实现固定滑动两屏，从高难度出发时到不了顶部（如从 N18
        // 出发只到 N7 就确认）。顶部边缘显示列表标题（识别返回 -1），
        // 因此同一读数（数值或 -1）连续重复即视为到达端点
        static constexpr int MaxScrollTimes = 8;
        int last_read = INT_MIN;
        int stable_count = 0;
        bool converged = false;
        for (int i = 0; i < MaxScrollTimes && !need_exit(); ++i) {
            m_current_difficulty = detect_current_difficulty();
            if (m_current_difficulty == last_read) {
                if (++stable_count >= 2) {
                    converged = true;
                    break;
                }
            }
            else {
                stable_count = 0;
                last_read = m_current_difficulty;
            }
            if (!ProcessTask(*this, { "SwipeToTheUp" }).run() || !sleep(300)) {
                LogError << "Task stopped during difficulty selection.";
                return false;
            }
        }
        m_current_difficulty = detect_current_difficulty();
        if (converged) {
            LogInfo << "Reached top of difficulty list, current: " << m_current_difficulty;
        }
        else {
            LogWarn << "Did not converge to the lowest difficulty, using current: " << m_current_difficulty;
        }
    }
    else {
        // 向目标难度方向逐步滑动并尝试点击，直到选中目标难度。
        // 原实现固定滑动两屏且仅尝试点击一次，目标难度（如 N14/N18）
        // 不在识别槽位时会未点击任何难度条目就直接确认，以错误难度
        // 开始探索。整屏滑动一次约移动 5 级，目标不在落点上时用小步
        // 幅滑动微调；目标出现在列表可视区域内时直接点击精确选中
        static constexpr int MaxAdjustTimes = 12;
        int stall_count = 0;
        bool boundary_reached = false;
        for (int i = 0; i < MaxAdjustTimes && !need_exit(); ++i) {
            const int previous = m_current_difficulty = detect_current_difficulty();
            LogInfo << "Target difficulty: " << difficulty << ", current: " << m_current_difficulty;
            if (m_current_difficulty == difficulty) {
                break;
            }

            const bool swipe_down = m_current_difficulty < 0 || m_current_difficulty < difficulty;
            const bool far_from_target = m_current_difficulty < 0 || m_current_difficulty < difficulty - 2 ||
                                         m_current_difficulty > difficulty + 2;
            if (!ProcessTask(
                     *this,
                     { swipe_down
                           ? (far_from_target ? "SwipeToTheDown" : theme + "@Roguelike@ChooseDifficulty_SwipeDownStep")
                           : (far_from_target ? "SwipeToTheUp" : theme + "@Roguelike@ChooseDifficulty_SwipeUpStep") })
                     .run() ||
                !sleep(300)) { // 等列表滚动稳定后再识别，避免动画中的 OCR 误读
                LogError << "Task stopped during difficulty selection.";
                return false;
            }

            // 目标难度在列表可视区域内时直接点击，确保精确选中；
            // 未识别到目标不会产生点击，不影响下面的停滞判定
            OCRer specified_analyzer(ctrler()->get_image());
            specified_analyzer.set_task_info(theme + "@Roguelike@ChooseDifficulty_ClickSpecified");
            specified_analyzer.set_required({ std::to_string(difficulty) });
            if (specified_analyzer.analyze()) {
                LogInfo << "Click target difficulty: " << difficulty;
                ctrler()->click(specified_analyzer.get_result().front().rect);
                if (!sleep(500)) {
                    LogError << "Task stopped during difficulty selection.";
                    return false;
                }
            }

            // 滑动并尝试点击后识别值仍不变，连续两次视为已到列表端点：
            // 目标超出可解锁范围时接受端点处已解锁的难度
            m_current_difficulty = detect_current_difficulty();
            if (m_current_difficulty == previous) {
                if (++stall_count >= 2) {
                    boundary_reached = true;
                    break;
                }
            }
            else {
                stall_count = 0;
            }
        }
        if (m_current_difficulty != difficulty) {
            m_current_difficulty = detect_current_difficulty();
        }
        if (m_current_difficulty != difficulty) {
            if (boundary_reached) {
                LogWarn << "Reached list endpoint without selecting target difficulty: " << difficulty
                        << ", current: " << m_current_difficulty;
            }
            else {
                // 选择失败时以当前难度兜底继续任务：此处返回失败会导致
                // 插件在每次 StartExplore 时被重新触发并反复放弃探索
                LogError << "Failed to select target difficulty: " << difficulty
                         << ", current: " << m_current_difficulty;
            }
        }
    }

    Log.info("Target difficulty:", difficulty);
    Log.info("Current difficulty:", m_current_difficulty);

    ProcessTask(*this, { theme + "@Roguelike@ChooseDifficultyConfirm" }).run();

    return true;
}
