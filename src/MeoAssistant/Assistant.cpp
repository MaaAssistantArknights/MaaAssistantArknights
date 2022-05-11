#include "Assistant.h"

#include <time.h>

#include <meojson/json.hpp>
#include <opencv2/opencv.hpp>

#include "AsstUtils.hpp"
#include "Controller.h"
#include "Logger.hpp"
#include "Resource.h"
#include "RuntimeStatus.h"

#include "FightTask.h"
#include "StartUpTask.h"
#include "AwardTask.h"
#include "VisitTask.h"
#include "MallTask.h"
#include "InfrastTask.h"
#include "RecruitTask.h"
#include "RoguelikeTask.h"
#include "CopilotTask.h"
#ifdef ASST_DEBUG
#include "DebugTask.h"
#endif

using namespace asst;

Assistant::Assistant(AsstApiCallback callback, void* callback_arg)
    : m_callback(callback),
    m_callback_arg(callback_arg)
{
    LogTraceFunction;

    m_status = std::make_shared<RuntimeStatus>();
    m_ctrler = std::make_shared<Controller>(task_callback, (void*)this);

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

    m_inited = false;
    std::unique_lock<std::mutex> lock(m_mutex);

    stop(false);

    bool ret = m_ctrler->connect(adb_path, address, config.empty() ? "General" : config);
    if (ret) {
        m_uuid = m_ctrler->get_uuid();
    }
    m_inited = ret;
    return ret;
}

asst::Assistant::TaskId asst::Assistant::append_task(const std::string& type, const std::string& params)
{
    Log.info(__FUNCTION__, type, params);

    auto ret = json::parse(params);
    if (!ret) {
        return 0;
    }

    std::shared_ptr<PackageTask> ptr = nullptr;

    if (type == FightTask::TaskType) {
        ptr = std::make_shared<FightTask>(task_callback, (void*)this);
    }
    else if (type == StartUpTask::TaskType) {
        ptr = std::make_shared<StartUpTask>(task_callback, (void*)this);
    }
    else if (type == AwardTask::TaskType) {
        ptr = std::make_shared<AwardTask>(task_callback, (void*)this);
    }
    else if (type == VisitTask::TaskType) {
        ptr = std::make_shared<VisitTask>(task_callback, (void*)this);
    }
    else if (type == MallTask::TaskType) {
        ptr = std::make_shared<MallTask>(task_callback, (void*)this);
    }
    else if (type == InfrastTask::TaskType) {
        ptr = std::make_shared<InfrastTask>(task_callback, (void*)this);
    }
    else if (type == RecruitTask::TaskType) {
        ptr = std::make_shared<RecruitTask>(task_callback, (void*)this);
    }
    else if (type == RoguelikeTask::TaskType) {
        ptr = std::make_shared<RoguelikeTask>(task_callback, (void*)this);
    }
    else if (type == CopilotTask::TaskType) {
        ptr = std::make_shared<CopilotTask>(task_callback, (void*)this);
    }
#ifdef ASST_DEBUG
    else if (type == DebugTask::TaskType) {
        ptr = std::make_shared<DebugTask>(task_callback, (void*)this);
    }
#endif
    else {
        Log.error(__FUNCTION__, "| invalid type:", type);
        return 0;
    }

    bool params_ret = ptr->set_params(ret.value());
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

    auto ret = json::parse(params);
    if (!ret) {
        return false;
    }

    bool setted = false;
    std::unique_lock<std::mutex> lock(m_mutex);
    for (auto&& [id, ptr] : m_tasks_list) {
        if (id == task_id) {
            setted = ptr->set_params(ret.value());
            break;
        }
    }

    return setted;
}

std::vector<uchar> asst::Assistant::get_image() const
{
    if (!m_inited) {
        return std::vector<uchar>();
    }
    return m_ctrler->get_image_encode();
}

bool asst::Assistant::ctrler_click(int x, int y, bool block)
{
    if (!m_inited) {
        return false;
    }
    m_ctrler->click(Point(x, y), block);
    return true;
}

bool asst::Assistant::start(bool block)
{
    LogTraceFunction;
    Log.trace("Start |", block ? "block" : "non block");

    if (!m_thread_idle || !m_inited) {
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

void Assistant::working_proc()
{
    LogTraceFunction;

    while (!m_thread_exit) {
        //LogTraceScope("Assistant::working_proc Loop");

        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_thread_idle && !m_tasks_list.empty()) {
            const auto [id, task_ptr] = m_tasks_list.front();
            lock.unlock();

            json::value callback_json = json::object{
                { "taskchain", task_ptr->get_task_chain() },
                { "taskid", task_ptr->get_task_id() }
            };
            task_callback(AsstMsg::TaskChainStart, callback_json, this);

            task_ptr->set_exit_flag(&m_thread_idle)
                .set_ctrler(m_ctrler)
                .set_status(m_status);

            bool ret = task_ptr->run();

            lock.lock();
            if (!m_tasks_list.empty()) {
                m_tasks_list.pop_front();
            }
            lock.unlock();

            auto run_msg = AsstMsg::TaskChainCompleted;
            if (!ret) {
                run_msg = AsstMsg::TaskChainError;
            }
            task_callback(run_msg, callback_json, this);

            if (m_tasks_list.empty()) {
                task_callback(AsstMsg::AllTasksCompleted, callback_json, this);
            }

            auto delay = Resrc.cfg().get_options().task_delay;
            lock.lock();
            m_condvar.wait_for(lock, std::chrono::milliseconds(delay),
                [&]() -> bool { return m_thread_idle; });
        }
        else {
            m_thread_idle = true;
            Log.flush();
            m_condvar.wait(lock);
        }
    }
}

void Assistant::msg_proc()
{
    LogTraceFunction;

    while (!m_thread_exit) {
        //LogTraceScope("Assistant::msg_proc Loop");
        std::unique_lock<std::mutex> lock(m_msg_mutex);
        if (!m_msg_queue.empty()) {
            // 结构化绑定只能是引用，后续的pop会使引用失效，所以需要重新构造一份，这里采用了move的方式
            auto&& [temp_msg, temp_detail] = m_msg_queue.front();
            AsstMsg msg = std::move(temp_msg);
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
    Assistant* p_this = (Assistant*)custom_arg;
    json::value more_detail = detail;
    more_detail["uuid"] = p_this->m_uuid;

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
    m_status->clear_data();
    //Task.clear_cache();
}
