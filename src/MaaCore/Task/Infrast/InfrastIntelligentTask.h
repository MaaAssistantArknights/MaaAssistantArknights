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
    void swipe_overview_down();
    void swipe_overview_up();
    void remove_room_duplicates();
    void remove_dorm_duplicates();
    int process_dormitory_capacity();
    std::vector<infrast::InfrastRoomInfo> process_workspace_priority();
    bool find_and_do_room(int target_index);
    std::vector<infrast::InfrastRoomInfo> m_room_infos;
    std::vector<infrast::InfrastDormInfo> m_dorm_infos;
};
}
