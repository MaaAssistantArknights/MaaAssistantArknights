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
    typedef int AsstTaskId;
    typedef int AsstProcessOptionKey;
    typedef int AsstInstanceOptionKey;
    typedef unsigned long long AsstSize;
    typedef int AsstAsyncCallId;

    typedef void(ASST_CALL* AsstApiCallback)(int msg, const char* detail_json, void* custom_arg);

    bool ASSTAPI AsstSetUserDir(const char* path);
    bool ASSTAPI AsstLoadResource(const char* path);
    bool ASSTAPI AsstSetProcessOption(AsstProcessOptionKey key, const char* value);

    AsstHandle ASSTAPI AsstCreate();
    AsstHandle ASSTAPI AsstCreateEx(AsstApiCallback callback, void* custom_arg);
    void ASSTAPI AsstDestroy(AsstHandle handle);

    bool ASSTAPI AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, const char* value);
    /* deprecated */ bool ASSTAPI AsstConnect(AsstHandle handle, const char* adb_path, const char* address,
                                              const char* config);

    AsstTaskId ASSTAPI AsstAppendTask(AsstHandle handle, const char* type, const char* params);
    bool ASSTAPI AsstSetTaskParams(AsstHandle handle, AsstTaskId id, const char* params);

    bool ASSTAPI AsstStart(AsstHandle handle);
    bool ASSTAPI AsstStop(AsstHandle handle);
    bool ASSTAPI AsstRunning(AsstHandle handle);

    /* Aysnc with AsstMsg::AsyncCallInfo Callback*/
    AsstAsyncCallId ASSTAPI AsstAsyncConnect(AsstHandle handle, const char* adb_path, const char* address,
                                             const char* config, bool block);
    AsstAsyncCallId ASSTAPI AsstAsyncClick(AsstHandle handle, int x, int y, bool block);
    AsstAsyncCallId ASSTAPI AsstAsyncScreencap(AsstHandle handle, bool block);

    AsstSize ASSTAPI AsstGetImage(AsstHandle handle, void* buff, AsstSize buff_size);
    AsstSize ASSTAPI AsstGetUUID(AsstHandle handle, char* buff, AsstSize buff_size);
    AsstSize ASSTAPI AsstGetTasksList(AsstHandle handle, AsstTaskId* buff, AsstSize buff_size);
    AsstSize ASSTAPI AsstGetNullSize();

    ASSTAPI_PORT const char* ASST_CALL AsstGetVersion();
    void ASSTAPI AsstLog(const char* level, const char* message);

#ifdef __cplusplus
}
#endif
