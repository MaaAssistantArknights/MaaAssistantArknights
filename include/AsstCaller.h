#pragma once

#include "AsstPort.h"
#include <stdint.h>

struct AsstExtAPI;
typedef struct AsstExtAPI* AsstHandle;

typedef uint8_t AsstBool;
typedef uint64_t AsstSize;

typedef int32_t AsstId;
typedef AsstId AsstMsgId;
typedef AsstId AsstTaskId;
typedef AsstId AsstAsyncCallId;

typedef int32_t AsstOptionKey;
typedef AsstOptionKey AsstStaticOptionKey;
typedef AsstOptionKey AsstInstanceOptionKey;

typedef void(ASST_CALL* AsstApiCallback)(AsstMsgId msg, const char* details_json, void* custom_arg);

#ifdef __cplusplus
extern "C"
{
#endif
    AsstBool ASSTAPI AsstSetUserDir(const char* path);
    AsstBool ASSTAPI AsstLoadResource(const char* path);
    AsstBool ASSTAPI AsstSetStaticOption(AsstStaticOptionKey key, const char* value);

    AsstHandle ASSTAPI AsstCreate();
    AsstHandle ASSTAPI AsstCreateEx(AsstApiCallback callback, void* custom_arg);
    void ASSTAPI AsstDestroy(AsstHandle handle);

    AsstBool ASSTAPI
        AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, const char* value);

    // 同步连接，功能已完全被异步连接取代
    // FIXME: 5.0 版本将废弃此接口
    /* deprecated */ AsstBool ASSTAPI AsstConnect(
        AsstHandle handle,
        const char* adb_path,
        const char* address,
        const char* config);

    AsstTaskId ASSTAPI AsstAppendTask(AsstHandle handle, const char* type, const char* params);
    AsstBool ASSTAPI AsstSetTaskParams(AsstHandle handle, AsstTaskId id, const char* params);

    AsstBool ASSTAPI AsstStart(AsstHandle handle);
    AsstBool ASSTAPI AsstStop(AsstHandle handle);
    AsstBool ASSTAPI AsstRunning(AsstHandle handle);
    AsstBool ASSTAPI AsstConnected(AsstHandle handle);
    AsstBool ASSTAPI AsstBackToHome(AsstHandle handle);

    /* Async with AsstMsg::AsyncCallInfo Callback*/
    AsstAsyncCallId ASSTAPI AsstAsyncConnect(
        AsstHandle handle,
        const char* adb_path,
        const char* address,
        const char* config,
        AsstBool block);
    void ASSTAPI AsstSetConnectionExtras(const char* name, const char* extras);

    AsstAsyncCallId ASSTAPI AsstAsyncClick(AsstHandle handle, int32_t x, int32_t y, AsstBool block);
    AsstAsyncCallId ASSTAPI AsstAsyncScreencap(AsstHandle handle, AsstBool block);

    AsstSize ASSTAPI AsstGetImage(AsstHandle handle, void* buff, AsstSize buff_size);
    AsstSize ASSTAPI AsstGetUUID(AsstHandle handle, char* buff, AsstSize buff_size);
    AsstSize ASSTAPI AsstGetTasksList(AsstHandle handle, AsstTaskId* buff, AsstSize buff_size);
    AsstSize ASSTAPI AsstGetNullSize();

    ASSTAPI_PORT const char* ASST_CALL AsstGetVersion();
    void ASSTAPI AsstLog(const char* level, const char* message);

#ifdef __cplusplus
}
#endif
