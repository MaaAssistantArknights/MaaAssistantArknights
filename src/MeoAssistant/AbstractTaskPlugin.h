#pragma once

#include <memory>

#include "AsstMsg.h"
#include "AbstractTask.h"

namespace asst
{
    class AbstractTaskPlugin : protected AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~AbstractTaskPlugin() = default;

        int priority() const;
        bool block() const;

        void set_priority(int priority);
        void set_block(bool block);

        virtual bool verify(AsstMsg msg, const json::value& details) const = 0;
        virtual bool run(AbstractTask* ptr) = 0;

        bool operator<(const AbstractTaskPlugin& rhs) const
        {
            return priority() < rhs.priority();
        }
        void set_plugin_exit_flag(bool* exit_flag) noexcept;

    protected:
        virtual bool _run() override { return false; }

        int m_priority = 0;
        bool m_block = false;
    };
}
