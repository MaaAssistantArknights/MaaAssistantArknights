#pragma once

#include "AsstPort.h"
#include <stdint.h>

struct AsstExtAPI;
typedef struct AsstExtAPI* AsstHandle;

typedef uint8_t AsstBool;
typedef uint64_t AsstSize;
typedef int32_t AsstMsgId;
typedef int32_t AsstTaskId;
typedef int32_t AsstAsyncCallId;
typedef int32_t AsstStaticOptionKey;
typedef int32_t AsstInstanceOptionKey;

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

    AsstBool ASSTAPI AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, const char* value);
    /* deprecated */ AsstBool ASSTAPI AsstConnect(AsstHandle handle, const char* adb_path, const char* address,
                                                  const char* config);

    AsstTaskId ASSTAPI AsstAppendTask(AsstHandle handle, const char* type, const char* params);
    AsstBool ASSTAPI AsstSetTaskParams(AsstHandle handle, AsstTaskId id, const char* params);

    AsstBool ASSTAPI AsstStart(AsstHandle handle);
    AsstBool ASSTAPI AsstStop(AsstHandle handle);
    AsstBool ASSTAPI AsstRunning(AsstHandle handle);

    /* Aysnc with AsstMsg::AsyncCallInfo Callback*/
    AsstAsyncCallId ASSTAPI AsstAsyncConnect(AsstHandle handle, const char* adb_path, const char* address,
                                             const char* config, AsstBool block);
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
