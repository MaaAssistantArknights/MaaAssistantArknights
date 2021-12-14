#pragma once

#include "AsstPort.h"

#ifdef __cplusplus
extern "C" {
#endif
    typedef void (ASST_CALL* AsstCallback)(int msg, const char* detail_json, void* custom_arg);

    ASSTAPI_PORT void* ASST_CALL AsstCreate(const char* dirname);
    ASSTAPI_PORT void* ASST_CALL AsstCreateEx(const char* dirname, AsstCallback callback, void* custom_arg);
    void ASSTAPI AsstDestroy(void* p_asst);

    bool ASSTAPI AsstCatchDefault(void* p_asst);
    bool ASSTAPI AsstCatchEmulator(void* p_asst);
    bool ASSTAPI AsstCatchCustom(void* p_asst, const char* address);
    bool ASSTAPI AsstCatchFake(void* p_asst);

    bool ASSTAPI AsstAppendFight(void* p_asst, int max_mecidine, int max_stone, int max_times);
    bool ASSTAPI AsstAppendAward(void* p_asst);
    bool ASSTAPI AsstAppendVisit(void* p_asst);
    bool ASSTAPI AsstAppendMall(void* p_asst, bool with_shopping);
    //bool ASSTAPI AsstAppendProcessTask(void* p_asst, const char* task);
    bool ASSTAPI AsstAppendInfrast(void* p_asst, int work_mode, const char** order, int order_size, const char* uses_of_drones, double dorm_threshold);
    bool ASSTAPI AsstAppendRecruit(void* p_asst, int max_times, const int select_level[], int select_len, const int confirm_level[], int confirm_len, bool need_refresh);
    bool ASSTAPI AsstAppendDebug(void* p_asst);

    bool ASSTAPI AsstStartRecruitCalc(void* p_asst, const int select_level[], int required_len, bool set_time);
    bool ASSTAPI AsstStart(void* p_asst);
    bool ASSTAPI AsstStop(void* p_asst);

    bool ASSTAPI AsstSetPenguinId(void* p_asst, const char* id);
    //bool ASSTAPI AsstSetParam(void* p_asst, const char* type, const char* param, const char* value);

    ASSTAPI_PORT const char* ASST_CALL AsstGetVersion();
#ifdef __cplusplus
}
#endif