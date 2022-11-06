#pragma once

#include "AsstPort.h"

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
namespace asst
{
    class Assistant;
}
#endif

#ifdef __cplusplus
extern "C"
{
#endif
#ifdef __cplusplus
    typedef asst::Assistant* AsstHandle;
#else
typedef void* AsstHandle;
#endif
    typedef int TaskId;
    typedef void(ASST_CALL* AsstApiCallback)(int msg, const char* detail_json, void* custom_arg);

    bool ASSTAPI AsstSetUserDir(const char* path);
    bool ASSTAPI AsstLoadResource(const char* path);

    AsstHandle ASSTAPI AsstCreate();
    AsstHandle ASSTAPI AsstCreateEx(AsstApiCallback callback, void* custom_arg);
    void ASSTAPI AsstDestroy(AsstHandle handle);

    bool ASSTAPI AsstConnect(AsstHandle handle, const char* adb_path, const char* address, const char* config);

    TaskId ASSTAPI AsstAppendTask(AsstHandle handle, const char* type, const char* params);
    bool ASSTAPI AsstSetTaskParams(AsstHandle handle, TaskId id, const char* params);

    bool ASSTAPI AsstStart(AsstHandle handle);
    bool ASSTAPI AsstStop(AsstHandle handle);
    bool ASSTAPI AsstRunning(AsstHandle handle);

    bool ASSTAPI AsstClick(AsstHandle handle, int x, int y);
    unsigned long long ASSTAPI AsstGetImage(AsstHandle handle, void* buff, unsigned long long buff_size);
    unsigned long long ASSTAPI AsstGetUUID(AsstHandle handle, char* buff, unsigned long long buff_size);
    unsigned long long ASSTAPI AsstGetTasksList(AsstHandle handle, TaskId* buff, unsigned long long buff_size);
    unsigned long long ASSTAPI AsstGetNullSize();

    ASSTAPI_PORT const char* ASST_CALL AsstGetVersion();
    void ASSTAPI AsstLog(const char* level, const char* message);

#ifdef __cplusplus
}
#endif
