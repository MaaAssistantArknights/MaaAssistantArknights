#pragma once

#include "AsstPort.h"

namespace asst
{
    class Assistant;
}

#ifdef __cplusplus
extern "C" {
#endif
    typedef asst::Assistant* AsstHandle;
    typedef void(ASST_CALL* AsstApiCallback)(int msg, const char* detail_json, void* custom_arg);

    bool ASSTAPI AsstLoadResource(const char* path);
    ASSTAPI_PORT AsstHandle ASST_CALL AsstCreate();
    ASSTAPI_PORT AsstHandle ASST_CALL AsstCreateEx(AsstApiCallback callback, void* custom_arg);
    void ASSTAPI AsstDestroy(AsstHandle handle);

    bool ASSTAPI AsstConnect(AsstHandle handle, const char* adb_path, const char* address, const char* config);

    bool ASSTAPI AsstAppendStartUp(AsstHandle handle);
    bool ASSTAPI AsstAppendFight(AsstHandle handle, const char* stage, int max_mecidine, int max_stone, int max_times);
    bool ASSTAPI AsstAppendAward(AsstHandle handle);
    bool ASSTAPI AsstAppendVisit(AsstHandle handle);
    bool ASSTAPI AsstAppendMall(AsstHandle handle, bool with_shopping);
    // bool ASSTAPI AsstAppendProcessTask(AsstHandle handle, const char* task_name);
    bool ASSTAPI AsstAppendInfrast(AsstHandle handle, int work_mode, const char** order, int order_size, const char* uses_of_drones, double dorm_threshold);
    bool ASSTAPI AsstAppendRecruit(AsstHandle handle, int max_times, const int select_level[], int select_len, const int confirm_level[], int confirm_len, bool need_refresh, bool use_expedited);
    bool ASSTAPI AsstAppendRoguelike(AsstHandle handle, int mode);

    bool ASSTAPI AsstAppendDebug(AsstHandle handle);

    bool ASSTAPI AsstStartRecruitCalc(AsstHandle handle, const int select_level[], int required_len, bool set_time);
    bool ASSTAPI AsstStart(AsstHandle handle);
    bool ASSTAPI AsstStop(AsstHandle handle);

    bool ASSTAPI AsstSetPenguinId(AsstHandle handle, const char* id);
    // bool ASSTAPI AsstSetParam(AsstHandle handle, const char* type, const char* param, const char* value);

    unsigned long long ASSTAPI AsstGetImage(AsstHandle handle, void* buff, unsigned long long buff_size);
    bool ASSTAPI AsstCtrlerClick(AsstHandle handle, int x, int y, bool block);

    ASSTAPI_PORT const char* ASST_CALL AsstGetVersion();

    void ASSTAPI AsstLog(AsstHandle handle, const char* level, const char* message);

#ifdef __cplusplus
}
#endif
