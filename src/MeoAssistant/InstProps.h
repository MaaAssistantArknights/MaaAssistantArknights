#pragma once

#include <memory>

namespace asst
{
    class Assistant;
    class Controller;
    class Status;

    class InstProps
    {
    protected:
        InstProps() = default;
        InstProps(const InstProps&) = default;
        InstProps(InstProps&&) noexcept = default;
        InstProps(Assistant* inst);
        virtual ~InstProps() noexcept = default;

        std::shared_ptr<Controller> ctrler() const;
        std::shared_ptr<Status> status() const;
        bool need_exit() const;

        InstProps& operator=(const InstProps&) = default;
        InstProps& operator=(InstProps&&) noexcept = default;

    protected:
        Assistant* m_inst = nullptr;
    };
}
