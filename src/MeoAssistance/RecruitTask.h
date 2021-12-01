#pragma once

#include "AbstractTask.h"

namespace asst
{
    class RecruitTask final : public AbstractTask
    {
        enum class ErrorT
        {
            Ok,
            NotInTagsPage,
            TagsError,
            OtherError
        };
    public:
        using AbstractTask::AbstractTask;
        virtual ~RecruitTask() = default;

        virtual bool run() override;

        void set_param(std::vector<int> required_level, bool set_time = true) noexcept;
        void set_confirm_level(std::vector<int> confirm_level) noexcept;
        void set_max_times(unsigned max_times);

    protected:
        virtual bool _run() override;

        std::vector<int> m_required_level;
        std::vector<int> m_confirm_level;
        bool m_set_time = false;
        unsigned m_max_times = 0;

        ErrorT m_last_error = ErrorT::Ok;
    };
}
