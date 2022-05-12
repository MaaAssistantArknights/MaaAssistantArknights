#pragma once

#include "AsstPort.h"
// 添加dart header
#include "dart_api.h" 
#include "dart_native_api.h"
namespace asst
{
    class Assistant;
}

#ifdef __cplusplus
extern "C" {
#endif
    typedef asst::Assistant* AsstHandle;
    typedef int TaskId;
    typedef void(ASST_CALL* AsstApiCallback)(int msg, const char* detail_json, void* custom_arg);

    bool ASSTAPI AsstLoadResource(const char* path);

    AsstHandle ASSTAPI AsstCreate();
    AsstHandle ASSTAPI AsstCreateEx(AsstApiCallback callback, void* custom_arg);
    DART_EXPORT AsstHandle AsstCreateWithDartPort(Dart_Port *dart_port);
    DART_EXPORT void InitDartVM(void *data);
    
    void ASSTAPI AsstDestroy(AsstHandle handle);

    bool ASSTAPI AsstConnect(AsstHandle handle, const char* adb_path, const char* address, const char* config);

    TaskId ASSTAPI AsstAppendTask(AsstHandle handle, const char* type, const char* params);
    bool ASSTAPI AsstSetTaskParams(AsstHandle handle, TaskId id, const char* params);

    bool ASSTAPI AsstStart(AsstHandle handle);
    bool ASSTAPI AsstStop(AsstHandle handle);

    unsigned long long ASSTAPI AsstGetImage(AsstHandle handle, void* buff, unsigned long long buff_size);
    bool ASSTAPI AsstCtrlerClick(AsstHandle handle, int x, int y, bool block);

    ASSTAPI_PORT const char* ASST_CALL AsstGetVersion();
    void ASSTAPI AsstLog(const char* level, const char* message);

#ifdef __cplusplus
}
#endif
