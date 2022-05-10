#pragma once

#include "AbstractTask.h"

#include <vector>

#include "AsstDef.h"

namespace asst
{
    class CreditShoppingTask final : public AbstractTask
    {        
    public:        
        using AbstractTask::AbstractTask;
        virtual ~CreditShoppingTask() = default;

        CreditShoppingTask& set_black_list(std::vector<int> black_list) noexcept;
    protected:
        virtual bool _run() override;
        std::vector<int> m_black_list;
    };
}
