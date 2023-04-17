#pragma once
#pragma once
#include "Common/AsstBattleDef.h"
#include "Task/AbstractTask.h"
#include <unordered_set>

namespace asst
{
    class RoleRecognitionTask : public AbstractTask
    {
    public:
        using AbstractTask::AbstractTask;
        virtual ~RoleRecognitionTask() override = default;

    protected:
        virtual bool _run() override;
        void swipe_page();
        std::vector<TextRect> analyzer_opers();
        std::string lastRole;
        std::string m_support_unit_name;
        std::unordered_set<std::string> roles;
        bool isLastRole(std::string name);
        void callback_analyze_result(bool done);
    };
}
