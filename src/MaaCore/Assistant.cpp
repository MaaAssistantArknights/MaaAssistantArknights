#include "Assistant.h"

#include "Utils/NoWarningCV.h"
#include "Utils/Ranges.hpp"
#include <meojson/json.hpp>

#include "Config/GeneralConfig.h"
#include "Controller/Controller.h"
#include "Status.h"
#include "Task/Interface/AwardTask.h"
#include "Task/Interface/CloseDownTask.h"
#include "Task/Interface/CopilotTask.h"
#include "Task/Interface/CustomTask.h"
#include "Task/Interface/DepotTask.h"
#include "Task/Interface/FightTask.h"
#include "Task/Interface/InfrastTask.h"
#include "Task/Interface/MallTask.h"
#include "Task/Interface/ReclamationTask.h"
#include "Task/Interface/RecruitTask.h"
#include "Task/Interface/RoguelikeTask.h"
#include "Task/Interface/SSSCopilotTask.h"
#include "Task/Interface/SingleStepTask.h"
#include "Task/Interface/StartUpTask.h"
#include "Task/Interface/VideoRecognitionTask.h"
#include "Utils/Logger.hpp"
#ifdef ASST_DEBUG
#include "Task/Interface/DebugTask.h"
#endif

using namespace asst;

bool ::AsstExtAPI::set_static_option(StaticOptionKey key, const std::string& value)
{
    Log.info(__FUNCTION__, "| key", static_cast<int>(key), "value", value);
    return false;
}

Assistant::Assistant(ApiCallback callback, void* callback_arg) : m_callback(callback), m_callback_arg(callback_arg)
{
    LogTraceFunction;

    m_status = std::make_shared<Status>();
    m_ctrler = std::make_shared<Controller>(append_callback_for_inst, this);

    m_msg_thread = std::thread(&Assistant::msg_proc, this);
    m_call_thread = std::thread(&Assistant::call_proc, this);
    m_working_thread = std::thread(&Assistant::working_proc, this);
}

Assistant::~Assistant()
{
    LogTraceFunction;

    m_thread_exit = true;
    m_thread_idle = true;

    {
        std::unique_lock<std::mutex> lock(m_call_mutex);
        m_call_condvar.notify_all();
    }
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condvar.notify_all();
    }
    {
        std::unique_lock<std::mutex> lock(m_msg_mutex);
        m_msg_condvar.notify_all();
    }

    if (m_working_thread.joinable()) {
        m_working_thread.join();
    }
    if (m_call_thread.joinable()) {
        m_call_thread.join();
    }
    if (m_msg_thread.joinable()) {
        m_msg_thread.join();
    }
}

bool asst::Assistant::set_instance_option(InstanceOptionKey key, const std::string& value)
{
    Log.info(__FUNCTION__, "| key", static_cast<int>(key), "value", value);
    switch (key) {
    case InstanceOptionKey::TouchMode:
        if (constexpr std::string_view Adb = "adb"; value == Adb) {
            m_ctrler->set_touch_mode(TouchMode::Adb);
            return true;
        }
        else if (constexpr std::string_view Minitouch = "minitouch"; value == Minitouch) {
            m_ctrler->set_touch_mode(TouchMode::Minitouch);
            return true;
        }
        else if (constexpr std::string_view MaaTouch = "maatouch"; value == MaaTouch) {
            m_ctrler->set_touch_mode(TouchMode::Maatouch);
            return true;
        }
        else if (constexpr std::string_view MacPlayTools = "MacPlayTools"; value == MacPlayTools) {
            m_ctrler->set_touch_mode(TouchMode::MacPlayTools);
            return true;
        }
        break;
    case InstanceOptionKey::DeploymentWithPause:
        if (constexpr std::string_view Enable = "1"; value == Enable) {
            m_ctrler->set_swipe_with_pause(true);
            return true;
        }
        else if (constexpr std::string_view Disable = "0"; value == Disable) {
            m_ctrler->set_swipe_with_pause(false);
            return true;
        }
        break;
    case InstanceOptionKey::AdbLiteEnabled:
        if (constexpr std::string_view Enable = "1"; value == Enable) {
            m_ctrler->set_adb_lite_enabled(true);
            return true;
        }
        else if (constexpr std::string_view Disable = "0"; value == Disable) {
            m_ctrler->set_adb_lite_enabled(false);
            return true;
        }
        break;
    case InstanceOptionKey::KillAdbOnExit:
        if (constexpr std::string_view Enable = "1"; value == Enable) {
            m_ctrler->set_kill_adb_on_exit(true);
            return true;
        }
        else if (constexpr std::string_view Disable = "0"; value == Disable) {
            m_ctrler->set_kill_adb_on_exit(false);
            return true;
        }
        break;
    default:
        break;
    }
    Log.error("Unknown key or value", value);
    return false;
}

bool asst::Assistant::ctrl_connect(const std::string& adb_path, const std::string& address, const std::string& config)
{
    LogTraceFunction;

    std::unique_lock<std::mutex> lock(m_mutex);

    // 仍有任务进行，connect 前需要 stop
    if (!m_thread_idle) {
        return false;
    }

    m_thread_idle = false;

    bool ret = m_ctrler->connect(adb_path, address, config.empty() ? "General" : config);
    if (ret) {
        m_uuid = m_ctrler->get_uuid();
    }

    m_thread_idle = true;
    return ret;
}

asst::Assistant::TaskId asst::Assistant::append_task(const std::string& type, const std::string& params)
{
    Log.info(__FUNCTION__, type, params);

    auto ret = json::parse(params.empty() ? "{}" : params);
    if (!ret) {
        return 0;
    }

    std::shared_ptr<InterfaceTask> ptr = nullptr;

#define ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(TASK)        \
    else if (type == TASK::TaskType)                                  \
    {                                                                 \
        ptr = std::make_shared<TASK>(append_callback_for_inst, this); \
    }

    if constexpr (false) {}
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(FightTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(StartUpTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(CloseDownTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(AwardTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(MallTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(InfrastTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(RecruitTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(RoguelikeTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(CopilotTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(SSSCopilotTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(SingleStepTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(VideoRecognitionTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(DepotTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(ReclamationTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(CustomTask)
#ifdef ASST_DEBUG
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(DebugTask)
#endif
    else
    {
        Log.error(__FUNCTION__, "| invalid type:", type);
        return 0;
    }

#undef ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH

    auto& json = ret.value();
    ptr->set_enable(json.get("enable", true));

    bool params_ret = ptr->set_params(json);
    if (!params_ret) {
        return 0;
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    ++m_task_id;
    ptr->set_task_id(m_task_id);
    m_tasks_list.emplace_back(m_task_id, ptr);
    return m_task_id;
}

bool asst::Assistant::set_task_params(TaskId task_id, const std::string& params)
{
    Log.info(__FUNCTION__, task_id, params);

    if (task_id <= 0) {
        return false;
    }

    auto ret = json::parse(params.empty() ? json::value().to_string() : params);
    if (!ret) {
        return false;
    }
    auto& json = ret.value();

    bool is_set = false;
    std::unique_lock<std::mutex> lock(m_mutex);
    for (auto&& [id, ptr] : m_tasks_list) {
        if (id != task_id) {
            continue;
        }
        bool enable = json.get("enable", true);
        ptr->set_enable(enable);
        is_set = ptr->set_params(json);
        break;
    }

    return is_set;
}

std::vector<uchar> asst::Assistant::get_image() const
{
    if (!inited()) {
        return {};
    }
    cv::Mat img = m_ctrler->get_image_cache();
    std::vector<uchar> buf;
    cv::imencode(".png", img, buf);
    return buf;
}

bool asst::Assistant::connect(const std::string& adb_path, const std::string& address, const std::string& config)
{
    LogTraceFunction;

    return ctrl_connect(adb_path, address, config);
}

asst::Assistant::AsyncCallId asst::Assistant::async_connect(const std::string& adb_path, const std::string& address,
                                                            const std::string& config, bool block)
{
    LogTraceFunction;

    return append_async_call(
        block, "Connect",
        [](Assistant* this_a, const std::string& adb_path, const std::string& address, const std::string& config) {
            return this_a->ctrl_connect(adb_path, address, config);
        },
        this, adb_path, address, config);
}

asst::Assistant::AsyncCallId asst::Assistant::async_click(int x, int y, bool block)
{
    LogTraceFunction;

    return append_async_call(
        block, "Click", [](Assistant* this_a, int x, int y) { return this_a->m_ctrler->click(Point(x, y)); }, this, x,
        y);
}

asst::Assistant::AsyncCallId asst::Assistant::async_screencap(bool block)
{
    LogTraceFunction;

    return append_async_call(
        block, "Screencap", [](Assistant* this_a) { return this_a->m_ctrler->screencap(); }, this);
}

bool asst::Assistant::connected() const
{
    return inited();
}

std::string asst::Assistant::get_uuid() const
{
    return m_uuid;
}

std::vector<Assistant::TaskId> asst::Assistant::get_tasks_list() const
{
    std::unique_lock<std::mutex> lock(m_mutex);
    std::vector<TaskId> result(m_tasks_list.size());
    ranges::copy(m_tasks_list | views::keys, result.begin());
    return result;
}

bool asst::Assistant::start(bool block)
{
    LogTraceFunction;
    Log.trace("Start |", block ? "block" : "non block");

    if (!m_thread_idle) {
        return false;
    }
    std::unique_lock<std::mutex> lock;
    if (block) { // 外部调用
        lock = std::unique_lock<std::mutex>(m_mutex);
    }

    m_thread_idle = false;
    m_condvar.notify_one();

    return true;
}

bool Assistant::stop(bool block)
{
    LogTraceFunction;
    Log.trace("Stop |", block ? "block" : "non block");

    m_thread_idle = true;

    std::unique_lock<std::mutex> lock;
    if (block) { // 外部调用
        lock = std::unique_lock<std::mutex>(m_mutex);
    }
    m_tasks_list.clear();

    clear_cache();

    return true;
}

bool asst::Assistant::running() const
{
    return !m_thread_idle;
}

void Assistant::working_proc()
{
    LogTraceFunction;

    std::vector<TaskId> finished_tasks;
    while (!m_thread_exit) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_thread_idle || m_tasks_list.empty()) {
            m_thread_idle = true;
            finished_tasks.clear();
            Log.flush();
            m_condvar.wait(lock);
            continue;
        }

        const auto [id, task_ptr] = m_tasks_list.front();
        lock.unlock();
        // only one instance of working_proc running, unlock here to allow set_task_param to the running task

        json::value callback_json = json::object {
            { "taskchain", std::string(task_ptr->get_task_chain()) },
            { "taskid", id },
        };
        append_callback(AsstMsg::TaskChainStart, callback_json);

        bool ret = task_ptr->run();
        finished_tasks.emplace_back(id);

        lock.lock();
        if (!m_tasks_list.empty()) {
            m_tasks_list.pop_front();
        }
        lock.unlock();

        auto msg =
            m_thread_idle ? AsstMsg::TaskChainStopped : (ret ? AsstMsg::TaskChainCompleted : AsstMsg::TaskChainError);
        append_callback(msg, callback_json);

        if (m_thread_idle) {
            finished_tasks.clear();
            continue;
        }

        if (m_tasks_list.empty()) {
            callback_json["finished_tasks"] = json::array(finished_tasks);
            append_callback(AsstMsg::AllTasksCompleted, callback_json);
            finished_tasks.clear();
        }

        const int delay = Config.get_options().task_delay;
        lock.lock();
        m_condvar.wait_for(lock, std::chrono::milliseconds(delay), [&]() -> bool { return m_thread_idle; });

        if (m_thread_idle) {
            append_callback(AsstMsg::TaskChainStopped, callback_json);
        }
    }
}

void Assistant::msg_proc()
{
    LogTraceFunction;

    while (!m_thread_exit) {
        std::unique_lock<std::mutex> lock(m_msg_mutex);

        if (m_msg_queue.empty()) {
            m_msg_condvar.wait(lock);
            continue;
        }

        // 结构化绑定只能是引用，后续的pop会使引用失效，所以需要重新构造一份，这里采用了move的方式
        auto&& [temp_msg, temp_detail] = m_msg_queue.front();
        AsstMsg msg = temp_msg;
        json::value detail = std::move(temp_detail);
        m_msg_queue.pop();
        lock.unlock();

        if (m_callback) {
            m_callback(static_cast<AsstMsgId>(msg), detail.to_string().c_str(), m_callback_arg);
        }
    }
}

template <typename Func, typename... Args>
asst::Assistant::AsyncCallId asst::Assistant::append_async_call(bool block, std::string what, Func&&, Args&&... args)
{
    LogTraceFunction;

    //    std::function<bool(void)> mf = [... args = std::forward<Args>(args)]() mutable {
    //        return std::invoke(Func {}, std::forward<Args>(args)...);
    //    };
    std::function<bool(void)> mf = [args...]() mutable { // type erasure, force capture by value
        return std::invoke(Func {}, args...);
    };
    AsyncCallId id;
    {
        std::unique_lock<std::mutex> lock(m_call_mutex);
        id = ++m_call_id;
        AsyncCallItem item = { id, std::move(mf), std::move(what) };
        m_call_queue.emplace(std::move(item));
        m_call_condvar.notify_one();
    }

    if (block) {
        // 需要保证队列中id一定是有序的
        wait_async_id(id);
    }

    return id;
}

bool asst::Assistant::wait_async_id(AsyncCallId id)
{
    while (!m_thread_exit) {
        auto old = m_completed_call.load();
        if (old >= id) return true;
        m_completed_call.wait(old);
    }
    return false;
}

void asst::Assistant::call_proc()
{
    LogTraceFunction;

    while (!m_thread_exit) {

        AsyncCallItem old_call_item;

        {
            std::unique_lock<std::mutex> lock(m_call_mutex);
            if (m_call_queue.empty()) {
                m_call_condvar.wait(lock);
                continue;
            }
            old_call_item = std::move(m_call_queue.front());
            m_call_queue.pop();
        }

        auto start = std::chrono::steady_clock::now();
        bool ret = old_call_item.function();
        m_completed_call = old_call_item.id;
        m_completed_call.notify_all();

        auto cost =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        json::value cb_info = json::object {
            { "uuid", m_uuid },
            { "what", old_call_item.what },
            { "async_call_id", old_call_item.id },
            {
                "details",
                json::object {
                    { "ret", ret },
                    { "cost", cost },
                },
            },
        };
        append_callback(AsstMsg::AsyncCallInfo, cb_info);
    }
}

void Assistant::append_callback(AsstMsg msg, const json::value& detail)
{
    json::value more_detail = detail;
    if (!more_detail.contains("uuid")) {
        more_detail["uuid"] = m_uuid;
    }
    switch (msg) {
    case AsstMsg::InternalError:
    case AsstMsg::InitFailed:
        stop(false);
        break;
    default:
        break;
    }

    // 加入回调消息队列，由回调消息线程外抛给外部
    Log.info("Assistant::append_callback |", msg, more_detail.to_string());

    std::unique_lock<std::mutex> lock(m_msg_mutex);
    m_msg_queue.emplace(msg, std::move(more_detail));
    m_msg_condvar.notify_one();
}

void asst::Assistant::append_callback_for_inst(AsstMsg msg, const json::value& detail, Assistant* inst)
{
    if (!inst) {
        return;
    }
    inst->append_callback(msg, detail);
}

void Assistant::clear_cache()
{
    m_status->clear_number();
    m_status->clear_rect();
    m_status->clear_str();
}

bool asst::Assistant::inited() const noexcept
{
    return m_ctrler && m_ctrler->inited();
}
