#pragma once

#include <memory>

#include "AbstractTask.h"
#include "Common/AsstMsg.h"
#include "Common/AsstTypes.h"
#include "MaaUtils/NoWarningCVMat.hpp"
#include "ProcessTask.h"

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

    std::strong_ordering operator<=>(const AbstractTaskPlugin& rhs) const;
    bool operator==(const AbstractTaskPlugin& rhs) const;

protected:
    std::shared_ptr<cv::Mat> get_hit_image() const;

    template <typename T>
    requires std::derived_from<T, asst::AnalyzerResult>
    std::shared_ptr<T> get_hit_detail() const
    {
        if (auto ptr = dynamic_cast<ProcessTask*>(m_task_ptr); !ptr) {
        }
        else if (auto last_hit = ptr->get_last_hit(); !last_hit || !last_hit->reco_detail) {
        }
        else if (auto detail = std::dynamic_pointer_cast<T>(last_hit->reco_detail)) {
            return detail;
        }
        Log.error(__FUNCTION__, "| Unable to get hit detail of type:", typeid(T).name());
        return nullptr;
    }

    AbstractTask* m_task_ptr = nullptr;
    int m_priority = 0;
    bool m_block = false;
};
}
