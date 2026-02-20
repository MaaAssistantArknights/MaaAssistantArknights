#pragma once

#include "AsstPort.h"
#include <stdint.h>

struct AsstExtAPI;
typedef struct AsstExtAPI* AsstHandle;

#ifdef _WIN32
// Win32 截图方式
typedef enum AsstWin32ScreencapMethodEnum
{
    AsstWin32ScreencapMethod_None = 0,
    AsstWin32ScreencapMethod_GDI = 1,
    AsstWin32ScreencapMethod_FramePool = 1 << 1,
    AsstWin32ScreencapMethod_DXGI_DesktopDup = 1 << 2,
    AsstWin32ScreencapMethod_DXGI_DesktopDup_Window = 1 << 3,
    AsstWin32ScreencapMethod_PrintWindow = 1 << 4,
    AsstWin32ScreencapMethod_ScreenDC = 1 << 5,
} AsstWin32ScreencapMethodEnum;

// Win32 输入方式
typedef enum AsstWin32InputMethodEnum
{
    AsstWin32InputMethod_None = 0,
    AsstWin32InputMethod_Seize = 1,
    AsstWin32InputMethod_SendMessage = 1 << 1,
    AsstWin32InputMethod_PostMessage = 1 << 2,
    AsstWin32InputMethod_LegacyEvent = 1 << 3,
    AsstWin32InputMethod_PostThreadMessage = 1 << 4,
    AsstWin32InputMethod_SendMessageWithCursorPos = 1 << 5,
    AsstWin32InputMethod_PostMessageWithCursorPos = 1 << 6,
    AsstWin32InputMethod_SendMessageWithWindowPos = 1 << 7,
    AsstWin32InputMethod_PostMessageWithWindowPos = 1 << 8,
} AsstWin32InputMethodEnum;
#endif

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

#ifdef _WIN32
    // 同步绑定到 Win32 窗口，功能已完全被异步绑定取代
    // FIXME: 5.0 版本将废弃此接口
    // hwnd: 目标窗口句柄
    // screencap_method: 截图方式，参见 Win32ScreencapMethod
    // mouse_method: 鼠标输入方式，参见 Win32InputMethod
    // keyboard_method: 键盘输入方式，参见 Win32InputMethod
    /* deprecated */ AsstBool ASSTAPI AsstAttachWindow(
        AsstHandle handle,
        void* hwnd,
        uint64_t screencap_method,
        uint64_t mouse_method,
        uint64_t keyboard_method);

    // 异步绑定到 Win32 窗口（仅 Windows 平台）
    // hwnd: 目标窗口句柄
    // screencap_method: 截图方式，参见 Win32ScreencapMethod
    // mouse_method: 鼠标输入方式，参见 Win32InputMethod
    // keyboard_method: 键盘输入方式，参见 Win32InputMethod
    AsstAsyncCallId ASSTAPI AsstAsyncAttachWindow(
        AsstHandle handle,
        void* hwnd,
        uint64_t screencap_method,
        uint64_t mouse_method,
        uint64_t keyboard_method,
        AsstBool block);
#endif

    AsstAsyncCallId ASSTAPI AsstAsyncClick(AsstHandle handle, int32_t x, int32_t y, AsstBool block);
    AsstAsyncCallId ASSTAPI AsstAsyncScreencap(AsstHandle handle, AsstBool block);

    AsstSize ASSTAPI AsstGetImage(AsstHandle handle, void* buff, AsstSize buff_size);
    AsstSize ASSTAPI AsstGetImageBgr(AsstHandle handle, void* buff, AsstSize buff_size);
    AsstSize ASSTAPI AsstGetUUID(AsstHandle handle, char* buff, AsstSize buff_size);
    AsstSize ASSTAPI AsstGetTasksList(AsstHandle handle, AsstTaskId* buff, AsstSize buff_size);
    AsstSize ASSTAPI AsstGetNullSize();

    ASSTAPI_PORT const char* ASST_CALL AsstGetVersion();
    void ASSTAPI AsstLog(const char* level, const char* message);

#ifdef __cplusplus
}
#endif
