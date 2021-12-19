#pragma once

#include "AsstPort.h"

namespace asst {
    class Assistant;
}

#ifdef __cplusplus
extern "C" {
#endif
    typedef void (ASST_CALL* AsstCallback)(int msg, const char* detail_json, void* custom_arg);

    ASSTAPI_PORT asst::Assistant* ASST_CALL AsstCreate(const char* dirname);
    ASSTAPI_PORT asst::Assistant* ASST_CALL AsstCreateEx(const char* dirname, AsstCallback callback, void* custom_arg);
    void ASSTAPI AsstDestroy(asst::Assistant* p_asst);

    bool ASSTAPI AsstCatchDefault(asst::Assistant* p_asst);
    bool ASSTAPI AsstCatchEmulator(asst::Assistant* p_asst);
    bool ASSTAPI AsstCatchCustom(asst::Assistant* p_asst, const char* address);
    bool ASSTAPI AsstCatchFake(asst::Assistant* p_asst);

    bool ASSTAPI AsstAppendStartUp(asst::Assistant* p_asst);
    bool ASSTAPI AsstAppendFight(asst::Assistant* p_asst, const char* stage, int max_mecidine, int max_stone, int max_times);
    bool ASSTAPI AsstAppendAward(asst::Assistant* p_asst);
    bool ASSTAPI AsstAppendVisit(asst::Assistant* p_asst);
    bool ASSTAPI AsstAppendMall(asst::Assistant* p_asst, bool with_shopping);
    //bool ASSTAPI AsstAppendProcessTask(asst::Assistant* p_asst, const char* task);
    bool ASSTAPI AsstAppendInfrast(asst::Assistant* p_asst, int work_mode, const char** order, int order_size, const char* uses_of_drones, double dorm_threshold);
    bool ASSTAPI AsstAppendRecruit(asst::Assistant* p_asst, int max_times, const int select_level[], int select_len, const int confirm_level[], int confirm_len, bool need_refresh);
    bool ASSTAPI AsstAppendDebug(asst::Assistant* p_asst);

    bool ASSTAPI AsstStartRecruitCalc(asst::Assistant* p_asst, const int select_level[], int required_len, bool set_time);
    bool ASSTAPI AsstStart(asst::Assistant* p_asst);
    bool ASSTAPI AsstStop(asst::Assistant* p_asst);

    bool ASSTAPI AsstSetPenguinId(asst::Assistant* p_asst, const char* id);
    //bool ASSTAPI AsstSetParam(asst::Assistant* p_asst, const char* type, const char* param, const char* value);

    ASSTAPI_PORT const char* ASST_CALL AsstGetVersion();
#ifdef __cplusplus
}
#endif