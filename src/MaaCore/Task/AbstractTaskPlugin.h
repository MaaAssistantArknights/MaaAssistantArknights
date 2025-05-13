#pragma once

#include <memory>

#include "AbstractTask.h"
#include "Common/AsstMsg.h"
#include "Utils/NoWarningCVMat.h"

namespace asst
{
class AbstractTaskPlugin : public AbstractTask
{
public:
    using AbstractTask::AbstractTask;
    virtual ~AbstractTaskPlugin() override = default;

    int priority() const;
    bool block() const;

    void set_priority(int priority);
    void set_block(bool block);

    virtual void set_task_ptr(AbstractTask* ptr);

    virtual bool verify(AsstMsg msg, const json::value& details) const = 0;

    std::strong_ordering operator<=>(const asst::AbstractTaskPlugin& rhs) const;
    bool operator==(const AbstractTaskPlugin& rhs) const;

protected:
    cv::Mat get_process_image() const;

    AbstractTask* m_task_ptr = nullptr;
    int m_priority = 0;
    bool m_block = false;
};
}
