#include "PackageTask.h"

#include "Resource.h"
#include "Logger.hpp"

bool asst::PackageTask::run()
{
    if (!m_enable) {
        Log.info("task disabled, pass", basic_info().to_string());
        return true;
    }
    m_finished = true;

    const auto task_delay = Resrc.cfg().get_options().task_delay;

    for (size_t i = 0; i != m_subtasks.size(); ++i) {
        if (need_exit()) {
            return false;
        }

        auto task_ptr = m_subtasks.at(i);
        if (!task_ptr->get_enable()) {
            continue;
        }

        task_ptr->set_exit_flag(m_exit_flag)
            .set_ctrler(m_ctrler)
            .set_status(m_status)
            .set_task_id(m_task_id);

        if (!task_ptr->run() && !task_ptr->get_ignore_error()) {
            return false;
        }

        if (i != m_subtasks.size() - 1) {
            sleep(task_delay);
        }
    }
    return true;
}

asst::AbstractTask& asst::PackageTask::set_exit_flag(bool* exit_flag) noexcept
{
    AbstractTask::set_exit_flag(exit_flag);
    for (auto&& sub : m_subtasks) {
        sub->set_exit_flag(exit_flag);
    }
    return *this;
}

asst::AbstractTask& asst::PackageTask::set_retry_times(int times) noexcept
{
    AbstractTask::set_retry_times(times);
    for (auto&& sub : m_subtasks) {
        sub->set_retry_times(times);
    }
    return *this;
}

asst::AbstractTask& asst::PackageTask::set_ctrler(std::shared_ptr<Controller> ctrler) noexcept
{
    AbstractTask::set_ctrler(ctrler);
    for (auto&& sub : m_subtasks) {
        sub->set_ctrler(ctrler);
    }
    return *this;
}

asst::AbstractTask& asst::PackageTask::set_status(std::shared_ptr<RuntimeStatus> status) noexcept
{
    AbstractTask::set_status(status);
    for (auto&& sub : m_subtasks) {
        sub->set_status(status);
    }
    return *this;
}

asst::AbstractTask& asst::PackageTask::set_task_id(int task_id) noexcept
{
    AbstractTask::set_task_id(task_id);
    for (auto&& sub : m_subtasks) {
        sub->set_task_id(task_id);
    }
    return *this;
}
