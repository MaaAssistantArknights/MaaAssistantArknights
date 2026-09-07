#include "SwitchThemeTask.h"

#include <random>

#include "Config/TaskData.h"
#include "Task/ProcessTask.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

asst::SwitchThemeTask::SwitchThemeTask(const AsstCallback& callback, Assistant* inst) :
    InterfaceTask(callback, inst, TaskType)
{
    LogTraceFunction;
}

bool asst::SwitchThemeTask::set_params(const json::value& params)
{
    LogTraceFunction;

    m_candidates.clear();
    auto themes_opt = params.find("themes");
    if (!themes_opt) {
        Log.error("SwitchThemeTask: no themes in params");
        return false;
    }
    if (!themes_opt->is_array()) {
        Log.error("SwitchThemeTask: themes is not an array");
        return false;
    }
    for (const auto& theme : themes_opt->as_array()) {
        if (!theme.is_string()) {
            continue;
        }
        std::string name = theme.as_string();
        utils::string_trim(name);
        if (!name.empty()) {
            m_candidates.emplace_back(std::move(name));
        }
    }
    return true;
}

bool asst::SwitchThemeTask::run()
{
    LogTraceFunction;

    if (!m_enable) {
        Log.info("task disabled, pass", basic_info().to_string());
        return true;
    }

    if (m_candidates.empty()) {
        Log.info("no candidate theme, skip");
        json::value skip_info = basic_info_with_what("SwitchThemeSkipped");
        callback(AsstMsg::SubTaskExtraInfo, skip_info);
        return true;
    }

    std::string target = m_candidates.front();
    if (m_candidates.size() > 1) {
        static std::default_random_engine rand_engine(std::random_device {}());
        std::uniform_int_distribution<size_t> rand_uni(0, m_candidates.size() - 1);
        target = m_candidates[rand_uni(rand_engine)];
    }
    Log.info("target theme:", target);

    Task.get<OcrTaskInfo>("SwitchThemeByNameSelectTheme")->text = { target };

    // 回主界面（小房子快捷优先，返回按钮兜底）→ 打开装扮面板 → 进入主题列表
    if (!ProcessTask(*this, { "SwitchThemeByNameBegin" }).run()) {
        return false;
    }

    constexpr int MaxDragTimes = 20;
    auto try_select = [&]() {
        return ProcessTask(*this, { "SwitchThemeByNameSelectTheme" }).set_retry_times(0).run();
    };

    bool selected = false;

    // 先在当前页找，未命中则像换日间一样快速滑到列表顶部（目标多在靠下的新主题区，途中逐屏识别收益低）
    if (try_select()) {
        selected = true;
    }
    else {
        for (int i = 0; i <= MaxDragTimes; ++i) {
            if (need_exit()) {
                return false;
            }
            if (ProcessTask(*this, { "SwitchThemeByNameListAtTop" }).set_retry_times(0).run()) {
                break;
            }
            if (i == MaxDragTimes) {
                break;
            }
            ProcessTask(*this, { "SwitchThemeByNameDragThemeList" }).run();
        }
    }

    // 从顶向下逐屏查找，先识别当前屏再翻页，顶部第一屏才会被选中
    for (int i = 0; !selected && i <= MaxDragTimes; ++i) {
        if (need_exit()) {
            return false;
        }
        if (try_select()) {
            selected = true;
            break;
        }
        if (i == MaxDragTimes) {
            break;
        }
        ProcessTask(*this, { "SwitchThemeByNameDragDownList" }).run();
    }

    if (!selected) {
        // 整个列表都没有目标，取消退出并报失败
        ProcessTask(*this, { "SwitchThemeByNameCancelTheme" }).run();
        Log.error("theme not found:", target);
        json::value fail_info = basic_info_with_what("SwitchThemeNotFound");
        fail_info["details"]["theme"] = target;
        callback(AsstMsg::SubTaskExtraInfo, fail_info);
        return false;
    }

    // 选中后按界面状态互斥分流（候选按序取首个命中），灰确认按钮颜色不敏感可被模板命中故未解锁先拦，
    // 已是当前主题次之，最后才点确认完成切换；命中分支由 GUI 按子任务回调区分日志
    if (!ProcessTask(*this, { "SwitchThemeByNameLockedTheme", "SwitchThemeByNameAlreadySet", "SwitchThemeByNameConfirmTheme" }).run()) {
        // 三种状态都不满足属异常，点取消退出并按失败处理
        ProcessTask(*this, { "SwitchThemeByNameCancelTheme" }).run();
        return false;
    }
    Log.info("theme switch flow done:", target);
    return true;
}
