#pragma once
#include "InfrastAbstractTask.h"
#include "Common/AsstInfrastDef.h"

namespace asst
{
class InfrastIntelligentTask final : public InfrastAbstractTask
{
public:
    using InfrastAbstractTask::InfrastAbstractTask;
    virtual ~InfrastIntelligentTask() override = default;

protected:
    virtual bool _run() override;
    bool scan_overview_workspace();
    bool scan_overview_dormitory();
    void swipe_overview_up();
    void remove_room_duplicates();
    void remove_dorm_duplicates();
    std::vector<infrast::InfrastRoomInfo> m_room_infos;
    std::vector<infrast::InfrastDormInfo> m_dorm_infos;
};
}
