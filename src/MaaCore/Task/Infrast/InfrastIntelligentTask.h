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
    bool scan_overview();
    void swipe_overview_up();
    void remove_duplicates();
    std::vector<infrast::InfrastRoomInfo> m_room_infos;
};
}
