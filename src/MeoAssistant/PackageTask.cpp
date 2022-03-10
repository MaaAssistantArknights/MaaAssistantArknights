#include "PackageTask.h"

#include "Resource.h"

bool asst::PackageTask::run()
{
    m_runned = true;

    const auto task_delay = Resrc.cfg().get_options().task_delay;

    for (auto task_ptr : m_subtasks) {
        if (need_exit()) {
            return false;
        }
        if (!task_ptr) {
            continue;
        }

        task_ptr->set_exit_flag(m_exit_flag)
            .set_ctrler(m_ctrler)
            .set_status(m_status);

        if (!task_ptr->run()) {
            return false;
        }

        sleep(task_delay);
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
