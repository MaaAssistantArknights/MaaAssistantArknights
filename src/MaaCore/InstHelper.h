#pragma once

#include <memory>

namespace asst
{
    class Assistant;
    class Controller;
    class Status;

    class InstHelper
    {
    protected:
        InstHelper() = default;
        InstHelper(const InstHelper&) = default;
        InstHelper(InstHelper&&) noexcept = default;
        InstHelper(Assistant* inst);
        virtual ~InstHelper() noexcept = default;

        std::shared_ptr<Controller> ctrler() const;
        std::shared_ptr<Status> status() const;
        bool need_exit() const;

        InstHelper& operator=(const InstHelper&) = default;
        InstHelper& operator=(InstHelper&&) noexcept = default;

    protected:
        Assistant* m_inst = nullptr;
    };
}
