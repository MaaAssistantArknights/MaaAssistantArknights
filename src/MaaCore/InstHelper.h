#pragma once

#include <memory>
#include <string>

namespace asst
{
    class Assistant;
    class Controller;
    class Status;

    class InstHelper
    {
    public:
        InstHelper() = default;
        InstHelper(const InstHelper&) = default;
        InstHelper(InstHelper&&) noexcept = default;
        InstHelper(Assistant* inst);
        virtual ~InstHelper() noexcept = default;

        std::shared_ptr<Controller> ctrler() const;
        std::shared_ptr<Status> status() const;
        bool need_exit() const;
        bool sleep(unsigned millisecond) const;

        Assistant* inst() noexcept;
        std::string inst_string() const;

        InstHelper& operator=(const InstHelper&) = default;
        InstHelper& operator=(InstHelper&&) noexcept = default;

    protected:
        Assistant* m_inst = nullptr;
    };
}
