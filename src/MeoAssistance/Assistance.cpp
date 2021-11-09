#include "Assistance.h"

#include <filesystem>
#include <time.h>

#include <json.h>
#include <opencv2/opencv.hpp>

#include "AsstUtils.hpp"
#include "Controller.h"
#include "Logger.hpp"
#include "Resource.h"

#include "CreditShoppingTask.h"
#include "InfrastDormTask.h"
#include "InfrastInfoTask.h"
#include "InfrastMfgTask.h"
#include "InfrastOfficeTask.h"
#include "InfrastPowerTask.h"
#include "InfrastReceptionTask.h"
#include "InfrastTradeTask.h"
#include "ProcessTask.h"
#include "RecruitTask.h"
#include "ScreenCaptureTask.h"

using namespace asst;

Assistance::Assistance(AsstCallback callback, void* callback_arg)
    : m_callback(callback), m_callback_arg(callback_arg)
{
    LogTraceFunction;

    bool resource_ret = resource.load(utils::get_resource_dir());
    if (!resource_ret) {
        const std::string& error = resource.get_last_error();
        log.error("resource broken", error);
        if (m_callback == nullptr) {
            return;
        }
        json::value callback_json;
        callback_json["type"] = "resource broken";
        callback_json["what"] = error;
        m_callback(AsstMsg::InitFaild, callback_json, m_callback_arg);
    }

    m_working_thread = std::thread(std::bind(&Assistance::working_proc, this));
    m_msg_thread = std::thread(std::bind(&Assistance::msg_proc, this));
}

Assistance::~Assistance()
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

bool asst::Assistance::catch_default()
{
    LogTraceFunction;

    auto& opt = resource.cfg().get_options();
    switch (opt.connect_type) {
    case ConnectType::Emulator:
        return catch_emulator();
    case ConnectType::USB:
        return catch_usb();
    case ConnectType::Remote:
        return catch_remote(opt.connect_remote_address);
    default:
        return false;
    }
}

bool Assistance::catch_emulator(const std::string& emulator_name)
{
    LogTraceFunction;

    stop();

    bool ret = false;
    //std::string cor_name = emulator_name;
    auto& cfg = resource.cfg();

    std::unique_lock<std::mutex> lock(m_mutex);

    // 自动匹配模拟器，逐个找
    if (emulator_name.empty()) {
        for (const auto& [name, info] : cfg.get_emulators_info()) {
            ret = ctrler.try_capture(info);
            if (ret) {
                //cor_name = name;
                break;
            }
        }
    }
    else { // 指定的模拟器
        auto& info = cfg.get_emulators_info().at(emulator_name);
        ret = ctrler.try_capture(info);
    }

    m_inited = ret;
    return ret;
}

bool asst::Assistance::catch_usb()
{
    LogTraceFunction;

    stop();

    bool ret = false;
    auto& cfg = resource.cfg();

    std::unique_lock<std::mutex> lock(m_mutex);

    EmulatorInfo remote_info = cfg.get_emulators_info().at("USB");

    ret = ctrler.try_capture(remote_info, true);

    m_inited = ret;
    return ret;
}

bool asst::Assistance::catch_remote(const std::string& address)
{
    LogTraceFunction;

    stop();

    bool ret = false;
    auto& cfg = resource.cfg();

    std::unique_lock<std::mutex> lock(m_mutex);

    EmulatorInfo remote_info = cfg.get_emulators_info().at("Remote");
    remote_info.adb.connect = utils::string_replace_all(remote_info.adb.connect, "[Address]", address);
    remote_info.adb.click = utils::string_replace_all(remote_info.adb.click, "[Address]", address);
    remote_info.adb.swipe = utils::string_replace_all(remote_info.adb.swipe, "[Address]", address);
    remote_info.adb.display = utils::string_replace_all(remote_info.adb.display, "[Address]", address);
    remote_info.adb.screencap = utils::string_replace_all(remote_info.adb.screencap, "[Address]", address);

    ret = ctrler.try_capture(remote_info, true);

    m_inited = ret;
    return ret;
}

bool asst::Assistance::catch_fake()
{
    LogTraceFunction;

    stop();

    m_inited = true;
    return true;
}

bool asst::Assistance::start_sanity()
{
    return start_process_task("SanityBegin", ProcessTaskRetryTimesDefault);
}

bool asst::Assistance::start_visit(bool with_shopping)
{
    LogTraceFunction;
    if (!m_thread_idle || !m_inited) {
        return false;
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    resource.templ().clear_hists();

    append_match_task("VisitBegin", { "VisitBegin" }, ProcessTaskRetryTimesDefault);

    if (with_shopping) {
        auto shopping_task_ptr = std::make_shared<CreditShoppingTask>(task_callback, (void*)this);
        shopping_task_ptr->set_task_chain("VisitBegin");
        m_tasks_deque.emplace_back(shopping_task_ptr);
    }

    m_thread_idle = false;
    m_condvar.notify_one();

    return true;
}

bool Assistance::start_process_task(const std::string& task, int retry_times, bool block)
{
    LogTraceFunction;
    log.trace("Start |", task, block ? "block" : "non block");
    if (!m_thread_idle || !m_inited) {
        return false;
    }

    std::unique_lock<std::mutex> lock;
    if (block) {
        lock = std::unique_lock<std::mutex>(m_mutex);
        //clear_exec_times();
        resource.templ().clear_hists();
    }

    append_match_task(task, { task }, retry_times);

    m_thread_idle = false;
    m_condvar.notify_one();

    return true;
}

#ifdef LOG_TRACE
bool Assistance::start_debug_task()
{
    LogTraceFunction;
    if (!m_thread_idle || !m_inited) {
        return false;
    }

    std::unique_lock<std::mutex> lock(m_mutex);

    {
        constexpr static const char* DebugTaskChain = "Debug";
        auto shift_task_ptr = std::make_shared<InfrastReceptionTask>(task_callback, (void*)this);
        //shift_task_ptr->set_facility("Mfg");
        //shift_task_ptr->set_product("CombatRecord");
        shift_task_ptr->set_task_chain(DebugTaskChain);
        m_tasks_deque.emplace_back(shift_task_ptr);
    }

    m_thread_idle = false;
    m_condvar.notify_one();

    return true;
}
#endif

bool Assistance::start_recruiting(const std::vector<int>& required_level, bool set_time)
{
    LogTraceFunction;
    if (!m_thread_idle || !m_inited) {
        return false;
    }

    std::unique_lock<std::mutex> lock(m_mutex);

    auto task_ptr = std::make_shared<RecruitTask>(task_callback, (void*)this);
    task_ptr->set_param(required_level, set_time);
    task_ptr->set_retry_times(OpenRecruitTaskRetyrTimesDefault);
    task_ptr->set_task_chain("OpenRecruit");
    m_tasks_deque.emplace_back(task_ptr);

    m_thread_idle = false;
    m_condvar.notify_one();

    return true;
}

bool asst::Assistance::start_infrast_shift(InfrastWorkMode work_mode, const std::vector<std::string>& order, UsesOfDrones uses_of_drones, double dorm_threshold)
{
    LogTraceFunction;
    if (!m_thread_idle || !m_inited) {
        return false;
    }

    // 偏激模式还没来得及做，保留用户接口，内部先按激进模式走
    if (work_mode == InfrastWorkMode::Extreme) {
        work_mode = InfrastWorkMode::Aggressive;
    }

    constexpr static const char* InfrastTaskCahin = "Infrast";

    // 这个流程任务，结束的时候是处于基建主界面的。既可以用于进入基建，也可以用于从设施里返回基建主界面
    append_match_task(InfrastTaskCahin, { "InfrastBegin" });

    auto info_task_ptr = std::make_shared<InfrastInfoTask>(task_callback, (void*)this);
    info_task_ptr->set_work_mode(work_mode);
    m_tasks_deque.emplace_back(info_task_ptr);

    // 因为后期要考虑多任务间的联动等，所以这些任务的声明暂时不妨到for循环中
    auto dorm_task_ptr = std::make_shared<InfrastDormTask>(task_callback, (void*)this);
    dorm_task_ptr->set_work_mode(work_mode);
    dorm_task_ptr->set_task_chain(InfrastTaskCahin);
    dorm_task_ptr->set_mood_threshold(dorm_threshold);
    auto mfg_task_ptr = std::make_shared<InfrastMfgTask>(task_callback, (void*)this);
    mfg_task_ptr->set_work_mode(work_mode);
    mfg_task_ptr->set_task_chain(InfrastTaskCahin);
    auto trade_task_ptr = std::make_shared<InfrastTradeTask>(task_callback, (void*)this);
    trade_task_ptr->set_work_mode(work_mode);
    trade_task_ptr->set_task_chain(InfrastTaskCahin);
    auto power_task_ptr = std::make_shared<InfrastPowerTask>(task_callback, (void*)this);
    power_task_ptr->set_work_mode(work_mode);
    power_task_ptr->set_task_chain(InfrastTaskCahin);
    auto office_task_ptr = std::make_shared<InfrastOfficeTask>(task_callback, (void*)this);
    office_task_ptr->set_work_mode(work_mode);
    office_task_ptr->set_task_chain(InfrastTaskCahin);
    auto recpt_task_ptr = std::make_shared<InfrastReceptionTask>(task_callback, (void*)this);
    recpt_task_ptr->set_work_mode(work_mode);
    recpt_task_ptr->set_task_chain(InfrastTaskCahin);

    for (const auto& facility : order) {
        if (facility == "Dorm") {
            m_tasks_deque.emplace_back(dorm_task_ptr);
        }
        else if (facility == "Mfg") {
            m_tasks_deque.emplace_back(mfg_task_ptr);
            if (UsesOfDrones::DronesMfg & uses_of_drones) {
                append_match_task(InfrastTaskCahin, { "DroneAssist-MFG" });
            }
        }
        else if (facility == "Trade") {
            m_tasks_deque.emplace_back(trade_task_ptr);
            if (UsesOfDrones::DronesTrade & uses_of_drones) {
                append_match_task(InfrastTaskCahin, { "DroneAssist-Trade" });
            }
        }
        else if (facility == "Power") {
            m_tasks_deque.emplace_back(power_task_ptr);
        }
        else if (facility == "Office") {
            m_tasks_deque.emplace_back(office_task_ptr);
        }
        else if (facility == "Reception") {
            m_tasks_deque.emplace_back(recpt_task_ptr);
        }
        else {
            log.error("start_infrast_shift | Unknown facility", facility);
        }
        append_match_task(InfrastTaskCahin, { "InfrastBegin" });
    }

    m_thread_idle = false;
    m_condvar.notify_one();

    return true;
}

void Assistance::stop(bool block)
{
    LogTraceFunction;
    log.trace("Stop |", block ? "block" : "non block");

    m_thread_idle = true;

    std::unique_lock<std::mutex> lock;
    if (block) { // 外部调用
        lock = std::unique_lock<std::mutex>(m_mutex);
    }
    decltype(m_tasks_deque) empty;
    m_tasks_deque.swap(empty);
    clear_exec_times();
    resource.templ().clear_hists();
}

bool Assistance::set_param(const std::string& type, const std::string& param, const std::string& value)
{
    LogTraceFunction;
    log.trace("SetParam |", type, param, value);

    return resource.task().set_param(type, param, value);
}

void Assistance::working_proc()
{
    LogTraceFunction;
    auto p_this = this;

    int retry_times = 0;
    while (!m_thread_exit) {
        //LogTraceScope("Assistance::working_proc Loop");
        std::unique_lock<std::mutex> lock(m_mutex);

        if (!m_thread_idle && !m_tasks_deque.empty()) {
            //controller.set_idle(false);

            auto start_time = std::chrono::system_clock::now();
            auto task_ptr = m_tasks_deque.front();
            // 先pop出来，如果执行失败再还原回去
            m_tasks_deque.pop_front();

            task_ptr->set_exit_flag(&m_thread_idle);
            bool ret = task_ptr->run();
            if (ret) {
                retry_times = 0;
                if (m_tasks_deque.empty()) {
                    json::value task_all_completed_json;
                    task_all_completed_json["task_chain"] = task_ptr->get_task_chain();
                    task_callback(AsstMsg::TaskChainCompleted, task_all_completed_json, p_this);
                }
            }
            else {
                // 失败了累加失败次数，超限了再pop
                if (retry_times >= task_ptr->get_retry_times()) {
                    json::value task_error_json;
                    task_error_json["retry_limit"] = task_ptr->get_retry_times();
                    task_error_json["retry_times"] = retry_times;
                    task_error_json["task_chain"] = task_ptr->get_task_chain();
                    task_callback(AsstMsg::TaskError, task_error_json, p_this);

                    retry_times = 0;
                    continue;
                }
                else {
                    ++retry_times;
                    // 执行失败再还原回去
                    m_tasks_deque.emplace_front(task_ptr);
                    task_ptr->on_run_fails(retry_times);
                }
            }

            auto& delay = resource.cfg().get_options().task_delay;
            m_condvar.wait_until(lock, start_time + std::chrono::milliseconds(delay),
                                 [&]() -> bool { return m_thread_idle; });
        }
        else {
            m_thread_idle = true;
            //controller.set_idle(true);
            m_condvar.wait(lock);
        }
    }
}

void Assistance::msg_proc()
{
    LogTraceFunction;

    while (!m_thread_exit) {
        //LogTraceScope("Assistance::msg_proc Loop");
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

void Assistance::task_callback(AsstMsg msg, const json::value& detail, void* custom_arg)
{
    log.trace("Assistance::task_callback |", msg, detail.to_string());

    Assistance* p_this = (Assistance*)custom_arg;
    json::value more_detail = detail;
    switch (msg) {
    case AsstMsg::PtrIsNull:
    case AsstMsg::ImageIsEmpty:
        p_this->stop(false);
        break;
        //case AsstMsg::WindowMinimized:
        //	p_this->controller.show_window();
        //	break;
    case AsstMsg::AppendProcessTask:
        more_detail["type"] = "ProcessTask";
        [[fallthrough]];
    case AsstMsg::AppendTask:
        p_this->append_task(more_detail, true);
        return; // 这俩消息Assistance会新增任务，外部不需要处理，直接拦掉
        break;
    case AsstMsg::StageDrops:
        more_detail = p_this->organize_stage_drop(more_detail);
        break;
    default:
        break;
    }

    // Todo: 有些不需要回调给外部的消息，得在这里给拦截掉
    // 加入回调消息队列，由回调消息线程外抛给外部
    p_this->append_callback(msg, std::move(more_detail));
}

void asst::Assistance::append_match_task(
    const std::string& task_chain, const std::vector<std::string>& tasks, int retry_times, bool front)
{
    auto task_ptr = std::make_shared<ProcessTask>(task_callback, (void*)this);
    task_ptr->set_task_chain(task_chain);
    task_ptr->set_tasks(tasks);
    task_ptr->set_retry_times(retry_times);
    if (front) {
        m_tasks_deque.emplace_front(task_ptr);
    }
    else {
        m_tasks_deque.emplace_back(task_ptr);
    }
}

void asst::Assistance::append_task(const json::value& detail, bool front)
{
    std::string task_type = detail.at("type").as_string();
    std::string task_chain = detail.get("task_chain", "");
    int retry_times = detail.get("retry_times", INT_MAX);

    if (task_type == "ProcessTask") {
        std::vector<std::string> next_vec;
        if (detail.exist("tasks")) {
            json::array next_arr = detail.at("tasks").as_array();
            for (const json::value& next_json : next_arr) {
                next_vec.emplace_back(next_json.as_string());
            }
        }
        else if (detail.exist("task")) {
            next_vec.emplace_back(detail.at("task").as_string());
        }
        append_match_task(task_chain, next_vec, retry_times, front);
    }
    // else if  // TODO
}

void asst::Assistance::append_callback(AsstMsg msg, json::value detail)
{
    std::unique_lock<std::mutex> lock(m_msg_mutex);
    m_msg_queue.emplace(msg, std::move(detail));
    m_msg_condvar.notify_one();
}

void Assistance::clear_exec_times()
{
    resource.task().clear_exec_times();
    resource.item().clear_drop_count();
}

json::value asst::Assistance::organize_stage_drop(const json::value& rec)
{
    json::value dst = rec;
    auto& item = resource.item();
    for (json::value& drop : dst["drops"].as_array()) {
        std::string id = drop["itemId"].as_string();
        int quantity = drop["quantity"].as_integer();
        item.increase_drop_count(id, quantity);
        const std::string& name = item.get_item_name(id);
        drop["itemName"] = name.empty() ? "未知材料" : name;
    }
    std::vector<json::value> statistics_vec;
    for (auto&& [id, count] : item.get_drop_count()) {
        json::value info;
        info["itemId"] = id;
        const std::string& name = item.get_item_name(id);
        info["itemName"] = name.empty() ? "未知材料" : name;
        info["count"] = count;
        statistics_vec.emplace_back(std::move(info));
    }
    // 排个序，数量多的放前面
    std::sort(statistics_vec.begin(), statistics_vec.end(),
              [](const json::value& lhs, const json::value& rhs) -> bool {
                  return lhs.at("count").as_integer() > rhs.at("count").as_integer();
              });

    dst["statistics"] = json::array(std::move(statistics_vec));

    log.trace("organize_stage_drop | ", dst.to_string());

    return dst;
}
