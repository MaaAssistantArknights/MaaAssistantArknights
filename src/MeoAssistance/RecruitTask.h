#pragma once

#include "AbstractTask.h"

namespace asst
{
    class RecruitTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~RecruitTask() = default;

        void set_param(std::vector<int> required_level, bool set_time = true) noexcept;
        void set_confirm_level(std::vector<int> confirm_level) noexcept;
        void set_need_refresh(bool need_refresh) noexcept;

    protected:
        virtual bool _run() override;

        std::vector<int> m_required_level;
        std::vector<int> m_confirm_level;
        bool m_set_time = false;
        bool m_need_refresh = false;

        enum class ErrorT
        {
            Ok,
            NotInTagsPage,
            TagsError,
            NotInConfirm,
            OtherError,
        };
        ErrorT m_last_error = ErrorT::Ok;
    };
}
