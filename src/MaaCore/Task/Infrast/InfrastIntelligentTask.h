#pragma once
#include "Common/AsstInfrastDef.h"
#include "InfrastAbstractTask.h"

namespace asst
{
class InfrastIntelligentTask final : public InfrastAbstractTask
{
public:
    using InfrastAbstractTask::InfrastAbstractTask;
    virtual ~InfrastIntelligentTask() override = default;

    void set_facility_allow(std::string facility)
    {
        if (facility == "Dorm") {
            dorm_allow = true;
        }
        else if (facility == "Mfg") {
            mfg_allow = true;
        }
        else if (facility == "Trade") {
            trade_allow = true;
        }
        else if (facility == "Power") {
            power_allow = true;
        }
        else if (facility == "Office") {
            office_allow = true;
        }
        else if (facility == "Reception") {
            reception_allow = true;
        }
        else if (facility == "Control") {
            control_allow = true;
        }
        else if (facility == "Processing") {
            processing_allow = true;
        }
        else if (facility == "Training") {
            training_allow = true;
        }
    }

    void set_continue_training(bool _continue_training) { this->continue_training = _continue_training; }

    void reset_allow_flags();

protected:
    virtual bool _run() override;
    bool dorm_allow = false;
    bool mfg_allow = false;
    bool trade_allow = false;
    bool power_allow = false;
    bool office_allow = false;
    bool reception_allow = false;
    bool control_allow = false;
    bool processing_allow = false;
    bool training_allow = false;
    bool continue_training = false;
    bool scan_overview_workspace();
    bool scan_overview_dormitory();
    void swipe_overview_down();
    void swipe_overview_up();
    void remove_room_duplicates();
    void remove_dorm_duplicates();
    int process_dormitory_capacity();
    std::vector<infrast::InfrastRoomInfo> process_workspace_priority();
    bool find_and_do_room(int target_index);
    bool find_and_do_special(int target_index);
    std::vector<infrast::InfrastRoomInfo> m_room_infos;
    std::vector<infrast::InfrastDormInfo> m_dorm_infos;
};
}
