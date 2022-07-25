#pragma once

#include "AsstPort.h"

#ifdef __cplusplus
extern "C" {
#endif
    typedef void* AsstHandle;
    typedef int TaskId;
    typedef void(ASST_CALL* AsstApiCallback)(int msg, const char* detail_json, void* custom_arg);

    int ASSTAPI AsstLoadResource(const char* path);

    AsstHandle ASSTAPI AsstCreate();
    AsstHandle ASSTAPI AsstCreateEx(AsstApiCallback callback, void* custom_arg);
    void ASSTAPI AsstDestroy(AsstHandle handle);

    int ASSTAPI AsstConnect(AsstHandle handle, const char* adb_path, const char* address, const char* config);

    TaskId ASSTAPI AsstAppendTask(AsstHandle handle, const char* type, const char* params);
    int ASSTAPI AsstSetTaskParams(AsstHandle handle, TaskId id, const char* params);

    int ASSTAPI AsstStart(AsstHandle handle);
    int ASSTAPI AsstStop(AsstHandle handle);

    int ASSTAPI AsstCtrlerClick(AsstHandle handle, int x, int y, int block);
    unsigned long long ASSTAPI AsstGetImage(AsstHandle handle, void* buff, unsigned long long buff_size);
    unsigned long long ASSTAPI AsstGetUUID(AsstHandle handle, char* buff, unsigned long long buff_size);
    unsigned long long ASSTAPI AsstGetTasksList(AsstHandle handle, TaskId* buff, unsigned long long buff_size);
    unsigned long long ASSTAPI AsstGetNullSize();

    ASSTAPI_PORT const char* ASST_CALL AsstGetVersion();
    void ASSTAPI AsstLog(const char* level, const char* message);

#ifdef __cplusplus
}
#endif
