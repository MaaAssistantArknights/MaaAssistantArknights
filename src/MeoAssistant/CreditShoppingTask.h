#pragma once

#include "AbstractTask.h"

#include <vector>

#include "AsstTypes.h"

namespace asst
{
    class CreditShoppingTask final : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~CreditShoppingTask() override = default;

        void set_black_list(std::vector<std::string> black_list);
        void set_white_list(std::vector<std::string> white_list);

    protected:
        virtual bool _run() override;

        std::vector<std::string> m_shopping_list;
        bool m_is_white_list = false;
    };
}
