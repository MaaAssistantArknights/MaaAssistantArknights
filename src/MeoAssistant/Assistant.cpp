#include "Assistant.h"

#include "Utils/AsstRanges.hpp"

#include <meojson/json.hpp>

#include "Controller.h"
#include "Resource/GeneralConfiger.h"
#include "RuntimeStatus.h"
#include "Utils/Logger.hpp"

#include "Task/AwardTask.h"
#include "Task/CloseDownTask.h"
#include "Task/CopilotTask.h"
#include "Task/DepotTask.h"
#include "Task/FightTask.h"
#include "Task/InfrastTask.h"
#include "Task/MallTask.h"
#include "Task/RecruitTask.h"
#include "Task/RoguelikeTask.h"
#include "Task/StartUpTask.h"
#include "Task/VisitTask.h"
#ifdef ASST_DEBUG
#include "Task/DebugTask.h"
#endif

using namespace asst;

Assistant::Assistant(AsstApiCallback callback, void* callback_arg) : m_callback(callback), m_callback_arg(callback_arg)
{
    LogTraceFunction;

    m_status = std::make_shared<RuntimeStatus>();
    m_ctrler = std::make_shared<Controller>(task_callback, static_cast<void*>(this));
    m_ctrler->set_exit_flag(&m_thread_idle);

    m_working_thread = std::thread(&Assistant::working_proc, this);
    m_msg_thread = std::thread(&Assistant::msg_proc, this);
}

Assistant::~Assistant()
{
    LogTraceFunction;

    m_thread_exit = true;
    m_thread_idle = true;
    m_condvar.notify_all();
    m_msg_condvar.notify_all();

    if (m_working_thread.joinable()) {
        m_working_thread.join();
    }
    if (m_msg_thread.joinable()) {
        m_msg_thread.join();
    }
}

bool asst::Assistant::connect(const std::string& adb_path, const std::string& address, const std::string& config)
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

    std::shared_ptr<PackageTask> ptr = nullptr;

#define ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(TASK)                 \
    else if (type == TASK::TaskType)                                           \
    {                                                                          \
        ptr = std::make_shared<TASK>(task_callback, static_cast<void*>(this)); \
    }

    if constexpr (false) {}
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(FightTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(StartUpTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(CloseDownTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(AwardTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(VisitTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(MallTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(InfrastTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(RecruitTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(RoguelikeTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(CopilotTask)
    ASST_ASSISTANT_APPEND_TASK_FROM_STRING_IF_BRANCH(DepotTask)
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
    ptr->set_exit_flag(&m_thread_idle).set_ctrler(m_ctrler).set_status(m_status).set_enable(json.get("enable", true));

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

    auto ret = json::parse(params.empty() ? "{}" : params);
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
    return m_ctrler->get_encoded_image_cache();
}

bool asst::Assistant::ctrler_click(int x, int y)
{
    if (!inited()) {
        return false;
    }
    m_ctrler->click(Point(x, y));
    return true;
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

    if (!m_thread_idle || !inited()) {
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

bool asst::Assistant::running()
{
    return !m_thread_idle;
}

void Assistant::working_proc()
{
    LogTraceFunction;

    std::vector<TaskId> finished_tasks;
    while (!m_thread_exit) {
        // LogTraceScope("Assistant::working_proc Loop");

        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_thread_idle && !m_tasks_list.empty()) {
            const auto [id, task_ptr] = m_tasks_list.front();
            lock.unlock();
            // only one instance of working_proc running, unlock here to allow set_task_param to the running task

            json::value callback_json = json::object {
                { "taskchain", task_ptr->get_task_chain() },
                { "taskid", id },
            };
            task_callback(AsstMsg::TaskChainStart, callback_json, this);

            bool ret = task_ptr->run();
            finished_tasks.emplace_back(id);

            lock.lock();
            if (!m_tasks_list.empty()) {
                m_tasks_list.pop_front();
            }
            lock.unlock();

            auto msg = m_thread_idle ? AsstMsg::TaskChainStopped
                                     : (ret ? AsstMsg::TaskChainCompleted : AsstMsg::TaskChainError);
            task_callback(msg, callback_json, this);

            if (m_thread_idle) {
                finished_tasks.clear();
                continue;
            }

            if (m_tasks_list.empty()) {
                callback_json["finished_tasks"] = json::array(finished_tasks);
                task_callback(AsstMsg::AllTasksCompleted, callback_json, this);
                finished_tasks.clear();
            }

            const int delay = Configer.get_options().task_delay;
            lock.lock();
            m_condvar.wait_for(lock, std::chrono::milliseconds(delay), [&]() -> bool { return m_thread_idle; });
        }
        else {
            m_thread_idle = true;
            finished_tasks.clear();
            Log.flush();
            m_condvar.wait(lock);
        }
    }
}

void Assistant::msg_proc()
{
    LogTraceFunction;

    while (!m_thread_exit) {
        // LogTraceScope("Assistant::msg_proc Loop");
        std::unique_lock<std::mutex> lock(m_msg_mutex);
        if (!m_msg_queue.empty()) {
            // 结构化绑定只能是引用，后续的pop会使引用失效，所以需要重新构造一份，这里采用了move的方式
            auto&& [temp_msg, temp_detail] = m_msg_queue.front();
            AsstMsg msg = temp_msg;
            json::value detail = std::move(temp_detail);
            m_msg_queue.pop();
            lock.unlock();

            if (m_callback) {
                m_callback(static_cast<int>(msg), detail.to_string().c_str(), m_callback_arg);
            }
        }
        else {
            m_msg_condvar.wait(lock);
        }
    }
}

void Assistant::task_callback(AsstMsg msg, const json::value& detail, void* custom_arg)
{
    auto p_this = static_cast<Assistant*>(custom_arg);
    json::value more_detail = detail;
    if (!more_detail.contains("uuid")) {
        more_detail["uuid"] = p_this->m_uuid;
    }

    switch (msg) {
    case AsstMsg::InternalError:
    case AsstMsg::InitFailed:
        p_this->stop(false);
        break;
    default:
        break;
    }

    Log.trace("Assistant::task_callback |", msg, more_detail.to_string());

    // 加入回调消息队列，由回调消息线程外抛给外部
    p_this->append_callback(msg, std::move(more_detail));
}

void asst::Assistant::append_callback(AsstMsg msg, json::value detail)
{
    std::unique_lock<std::mutex> lock(m_msg_mutex);
    m_msg_queue.emplace(msg, std::move(detail));
    m_msg_condvar.notify_one();
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
