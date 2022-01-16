#include "Assistant.h"

#include <filesystem>
#include <time.h>

#include <meojson/json.h>
#include <opencv2/opencv.hpp>

#include "AsstUtils.hpp"
#include "Controller.h"
#include "Logger.hpp"
#include "Resource.h"

#include "CreditShoppingTask.h"
#include "BattleTask.h"
#include "InfrastDormTask.h"
#include "InfrastInfoTask.h"
#include "InfrastMfgTask.h"
#include "InfrastOfficeTask.h"
#include "InfrastPowerTask.h"
#include "InfrastReceptionTask.h"
#include "InfrastTradeTask.h"
#include "ProcessTask.h"
#include "RecruitTask.h"
#include "AutoRecruitTask.h"
#include "InfrastControlTask.h"
#include "RuntimeStatus.h"
#include "StageDropsTaskPlugin.h"
#include "DronesForShamareTaskPlugin.h"

using namespace asst;

Assistant::Assistant(std::string dirname, AsstCallback callback, void* callback_arg)
    : m_dirname(std::move(dirname) + "/"),
    m_callback(callback),
    m_callback_arg(callback_arg)
{
    Logger::set_dirname(m_dirname);
    Controller::set_dirname(m_dirname);

    LogTraceFunction;

    bool resource_ret = Resrc.load(m_dirname + "resource/");
    if (!resource_ret) {
        const std::string& error = Resrc.get_last_error();
        Log.error("resource broken:", error);
        if (m_callback == nullptr) {
            throw error;
        }
        json::value callback_json;
        callback_json["what"] = "resource broken";
        callback_json["details"] = json::object{
            { "error", error }
        };
        m_callback(AsstMsg::InitFailed, callback_json, m_callback_arg);
        throw error;
    }

    m_working_thread = std::thread(std::bind(&Assistant::working_proc, this));
    m_msg_thread = std::thread(std::bind(&Assistant::msg_proc, this));
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

bool asst::Assistant::catch_default()
{
    LogTraceFunction;

    auto& opt = Resrc.cfg().get_options();
    switch (opt.connect_type) {
    case ConnectType::Emulator:
        return catch_emulator();
    case ConnectType::Custom:
        return catch_custom();
    default:
        return false;
    }
}

bool Assistant::catch_emulator(const std::string& emulator_name)
{
    LogTraceFunction;

    stop();
#ifdef _WIN32
    bool ret = false;
    //std::string cor_name = emulator_name;
    auto& cfg = Resrc.cfg();

    std::unique_lock<std::mutex> lock(m_mutex);

    // 自动匹配模拟器，逐个找
    if (emulator_name.empty()) {
        for (const auto& [name, info] : cfg.get_emulators_info()) {
            ret = Ctrler.try_capture(info);
            if (ret) {
                //cor_name = name;
                break;
            }
        }
    }
    else { // 指定的模拟器
        auto& info = cfg.get_emulators_info().at(emulator_name);
        ret = Ctrler.try_capture(info);
    }

    m_inited = ret;
    return ret;
#else   // Not supported catch emulator in Linux
    return false;
#endif
}

bool asst::Assistant::catch_custom(const std::string& address)
{
    LogTraceFunction;

    stop();

    bool ret = false;
    auto& cfg = Resrc.cfg();

    std::unique_lock<std::mutex> lock(m_mutex);

    EmulatorInfo remote_info = cfg.get_emulators_info().at("Custom");
    if (!address.empty()) {
        remote_info.adb.addresses.push_back(address);
    }

    ret = Ctrler.try_capture(remote_info, true);

    m_inited = ret;
    return ret;
}

bool asst::Assistant::catch_fake()
{
    LogTraceFunction;

    stop();

    m_inited = true;
    return true;
}

bool asst::Assistant::append_start_up(bool only_append)
{
    LogTraceFunction;
    if (!m_inited) {
        return false;
    }

    std::unique_lock<std::mutex> lock(m_mutex);

    auto task_ptr = std::make_shared<ProcessTask>(task_callback, (void*)this, "StartUp");
    task_ptr->set_tasks({ "StartUp" })
        .set_times_limit("ReturnToTerminal", 0)
        .set_times_limit("Terminal", 0);

    m_tasks_queue.emplace(task_ptr);

    if (!only_append) {
        return start(false);
    }

    return true;
}

bool asst::Assistant::append_fight(const std::string& stage, int mecidine, int stone, int times, bool only_append)
{
    LogTraceFunction;
    if (!m_inited) {
        return false;
    }

    constexpr const char* TaskChain = "Fight";

    // 进入选关界面（主界面的“终端”点进去）
    auto terminal_task_ptr = std::make_shared<ProcessTask>(task_callback, (void*)this, TaskChain);
    terminal_task_ptr->set_tasks({ "StageBegin" })
        .set_times_limit("LastBattle", 0)
        .set_times_limit("StartButton1", 0)
        .set_times_limit("StartButton2", 0)
        .set_times_limit("MedicineConfirm", 0)
        .set_times_limit("StoneConfirm", 0);

    // 进入对应的关卡
    auto stage_task_ptr = std::make_shared<ProcessTask>(task_callback, (void*)this, TaskChain);
    stage_task_ptr->set_tasks({ stage })
        .set_times_limit("StartButton1", 0)
        .set_times_limit("StartButton2", 0)
        .set_times_limit("MedicineConfirm", 0)
        .set_times_limit("StoneConfirm", 0);

    // 开始战斗任务
    auto fight_task_ptr = std::make_shared<ProcessTask>(task_callback, (void*)this, TaskChain);
    fight_task_ptr->set_tasks({ "FightBegin" })
        .set_times_limit("MedicineConfirm", mecidine)
        .set_times_limit("StoneConfirm", stone)
        .set_times_limit("StartButton1", times)
        .set_times_limit("StartButton2", times);
    fight_task_ptr->regiseter_plugin<StageDropsTaskPlugin>()
        ->set_retry_times(0);

    std::unique_lock<std::mutex> lock(m_mutex);

    if (!stage.empty()) {
        m_tasks_queue.emplace(terminal_task_ptr);
        m_tasks_queue.emplace(stage_task_ptr);
    }
    m_tasks_queue.emplace(fight_task_ptr);

    if (!only_append) {
        return start(false);
    }

    return true;
}

bool asst::Assistant::append_award(bool only_append)
{
    append_process_task("AwardBegin", "Award");

    if (!only_append) {
        return start(false);
    }

    return true;
}

bool asst::Assistant::append_visit(bool only_append)
{
    append_process_task("VisitBegin", "Visit");

    if (!only_append) {
        return start(false);
    }

    return true;
}

bool asst::Assistant::append_mall(bool with_shopping, bool only_append)
{
    LogTraceFunction;
    if (!m_inited) {
        return false;
    }

    std::unique_lock<std::mutex> lock(m_mutex);

    const std::string TaskChain = "Mall";

    append_process_task("MallBegin", TaskChain);

    if (with_shopping) {
        auto shopping_task_ptr = std::make_shared<CreditShoppingTask>(task_callback, (void*)this, TaskChain);
        m_tasks_queue.emplace(shopping_task_ptr);
    }

    if (!only_append) {
        return start(false);
    }

    return true;
}

bool Assistant::append_process_task(const std::string& task_name, std::string task_chain, int retry_times)
{
    LogTraceFunction;
    if (!m_inited) {
        return false;
    }

    //std::unique_lock<std::mutex> lock(m_mutex);

    if (task_chain.empty()) {
        task_chain = task_name;
    }

    auto task_ptr = std::make_shared<ProcessTask>(task_callback, (void*)this, task_chain);
    task_ptr->set_tasks({ task_name })
        .set_retry_times(retry_times);

    m_tasks_queue.emplace(task_ptr);

    //if (!only_append) {
    //    return start(false);
    //}

    return true;
}

bool asst::Assistant::append_recruit(unsigned max_times, const std::vector<int>& select_level, const std::vector<int>& confirm_level, bool need_refresh, bool use_expedited)
{
    LogTraceFunction;
    if (!m_inited) {
        return false;
    }
    static const std::string TaskChain = "Recruit";

    append_process_task("RecruitBegin", TaskChain);

    auto recruit_task_ptr = std::make_shared<AutoRecruitTask>(task_callback, (void*)this, TaskChain);
    recruit_task_ptr->set_max_times(max_times)
        .set_need_refresh(need_refresh)
        .set_use_expedited(use_expedited)
        .set_select_level(select_level)
        .set_confirm_level(confirm_level)
        .set_retry_times(AutoRecruitTaskRetryTimesDefault);

    m_tasks_queue.emplace(recruit_task_ptr);

    return true;
}

#ifdef ASST_DEBUG
bool Assistant::append_debug()
{
    LogTraceFunction;
    if (!m_inited) {
        return false;
    }

    std::unique_lock<std::mutex> lock(m_mutex);

    {
        constexpr static const char* DebugTaskChain = "Debug";
        auto debug_task_ptr = std::make_shared<BattleTask>(task_callback, (void*)this, DebugTaskChain);
        m_tasks_queue.emplace(debug_task_ptr);
    }

    return true;
}
#endif

bool Assistant::start_recruit_calc(const std::vector<int>& select_level, bool set_time)
{
    LogTraceFunction;
    if (!m_inited) {
        return false;
    }

    std::unique_lock<std::mutex> lock(m_mutex);

    auto task_ptr = std::make_shared<RecruitTask>(task_callback, (void*)this, "RecruitCalc");
    task_ptr->set_retry_times(OpenRecruitTaskRetryTimesDefault);
    task_ptr->set_param(select_level, set_time);
    m_tasks_queue.emplace(task_ptr);

    return start(false);
}

bool asst::Assistant::append_infrast(infrast::WorkMode work_mode, const std::vector<std::string>& order, const std::string& uses_of_drones, double dorm_threshold, bool only_append)
{
    LogTraceFunction;
    if (!m_inited) {
        return false;
    }

    // 保留接口，目前强制按激进模式进行换班
    work_mode = infrast::WorkMode::Aggressive;

    constexpr static const char* InfrastTaskCahin = "Infrast";

    // 这个流程任务，结束的时候是处于基建主界面的。既可以用于进入基建，也可以用于从设施里返回基建主界面

    auto append_infrast_begin = [&]() {
        append_process_task("InfrastBegin", InfrastTaskCahin);
    };

    append_infrast_begin();

    auto info_task_ptr = std::make_shared<InfrastInfoTask>(task_callback, (void*)this, InfrastTaskCahin);
    info_task_ptr->set_work_mode(work_mode)
        .set_mood_threshold(dorm_threshold);

    m_tasks_queue.emplace(info_task_ptr);

    // 因为后期要考虑多任务间的联动等，所以这些任务的声明暂时不放到for循环中
    auto mfg_task_ptr = std::make_shared<InfrastMfgTask>(task_callback, (void*)this, InfrastTaskCahin);
    mfg_task_ptr->set_uses_of_drone(uses_of_drones)
        .set_work_mode(work_mode)
        .set_mood_threshold(dorm_threshold);
    auto trade_task_ptr = std::make_shared<InfrastTradeTask>(task_callback, (void*)this, InfrastTaskCahin);
    trade_task_ptr->set_uses_of_drone(uses_of_drones)
        .set_work_mode(work_mode)
        .set_mood_threshold(dorm_threshold);
    trade_task_ptr->regiseter_plugin<DronesForShamareTaskPlugin>();
    auto power_task_ptr = std::make_shared<InfrastPowerTask>(task_callback, (void*)this, InfrastTaskCahin);
    power_task_ptr->set_work_mode(work_mode)
        .set_mood_threshold(dorm_threshold);
    auto office_task_ptr = std::make_shared<InfrastOfficeTask>(task_callback, (void*)this, InfrastTaskCahin);
    office_task_ptr->set_work_mode(work_mode)
        .set_mood_threshold(dorm_threshold);
    auto recpt_task_ptr = std::make_shared<InfrastReceptionTask>(task_callback, (void*)this, InfrastTaskCahin);
    recpt_task_ptr->set_work_mode(work_mode)
        .set_mood_threshold(dorm_threshold);
    auto control_task_ptr = std::make_shared<InfrastControlTask>(task_callback, (void*)this, InfrastTaskCahin);
    control_task_ptr->set_work_mode(work_mode)
        .set_mood_threshold(dorm_threshold);

    auto dorm_task_ptr = std::make_shared<InfrastDormTask>(task_callback, (void*)this, InfrastTaskCahin);
    dorm_task_ptr->set_work_mode(work_mode)
        .set_mood_threshold(dorm_threshold);

    for (const auto& facility : order) {
        if (facility == "Dorm") {
            m_tasks_queue.emplace(dorm_task_ptr);
        }
        else if (facility == "Mfg") {
            m_tasks_queue.emplace(mfg_task_ptr);
        }
        else if (facility == "Trade") {
            m_tasks_queue.emplace(trade_task_ptr);
        }
        else if (facility == "Power") {
            m_tasks_queue.emplace(power_task_ptr);
        }
        else if (facility == "Office") {
            m_tasks_queue.emplace(office_task_ptr);
        }
        else if (facility == "Reception") {
            m_tasks_queue.emplace(recpt_task_ptr);
        }
        else if (facility == "Control") {
            m_tasks_queue.emplace(control_task_ptr);
        }
        else {
            Log.error("append_infrast | Unknown facility", facility);
        }
        append_infrast_begin();
    }

    if (!only_append) {
        return start(false);
    }

    return true;
}

void asst::Assistant::set_penguin_id(const std::string& id)
{
    auto& opt = Resrc.cfg().get_options();
    if (id.empty()) {
        opt.penguin_report.extra_param.clear();
    }
    else {
        opt.penguin_report.extra_param = "-H \"authorization: PenguinID " + id + "\"";
    }
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
    decltype(m_tasks_queue) empty;
    m_tasks_queue.swap(empty);

    clear_cache();

    return true;
}

void Assistant::working_proc()
{
    LogTraceFunction;

    std::string pre_taskchain;
    while (!m_thread_exit) {
        //LogTraceScope("Assistant::working_proc Loop");
        std::unique_lock<std::mutex> lock(m_mutex);

        if (!m_thread_idle && !m_tasks_queue.empty()) {
            auto task_ptr = m_tasks_queue.front();
            m_tasks_queue.pop();

            std::string cur_taskchain = task_ptr->get_task_chain();
            std::string next_taskchain = m_tasks_queue.empty() ? std::string() : m_tasks_queue.front()->get_task_chain();
            json::value callback_json = json::object{
                { "taskchain", cur_taskchain },
                { "pre_taskchain", pre_taskchain },
                { "next_taskchain", next_taskchain }
            };
            if (cur_taskchain != pre_taskchain) {
                task_callback(AsstMsg::TaskChainStart, callback_json, this);
            }

            task_ptr->set_exit_flag(&m_thread_idle);
            bool ret = task_ptr->run();

            if (cur_taskchain != next_taskchain) {
                if (ret) {
                    task_callback(AsstMsg::TaskChainCompleted, callback_json, this);
                }
                else {
                    task_callback(AsstMsg::TaskChainError, callback_json, this);
                }
            }
            if (m_tasks_queue.empty()) {
                task_callback(AsstMsg::AllTasksCompleted, callback_json, this);
            }

            pre_taskchain = cur_taskchain;

            auto& delay = Resrc.cfg().get_options().task_delay;
            m_condvar.wait_for(lock, std::chrono::milliseconds(delay),
                [&]() -> bool { return m_thread_idle; });
        }
        else {
            pre_taskchain.clear();
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
                m_callback(msg, detail, m_callback_arg);
            }
        }
        else {
            m_msg_condvar.wait(lock);
        }
    }
}

void Assistant::task_callback(AsstMsg msg, const json::value& detail, void* custom_arg)
{
    Log.trace("Assistant::task_callback |", msg, detail.to_string());

    Assistant* p_this = (Assistant*)custom_arg;
    json::value more_detail = detail;
    switch (msg) {
    case AsstMsg::InternalError:
    case AsstMsg::InitFailed:
    case AsstMsg::ConnectionError:
        p_this->stop(false);
        break;
    default:
        break;
    }

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
    Status.clear();
    //Task.clear_cache();
}
