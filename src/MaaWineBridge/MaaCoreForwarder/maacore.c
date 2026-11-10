#include "dl_maacore.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// clang-format off
// don't sort includes
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
// clang-format on

#include "task_queue.h"
#include "task_queue_wine.h"

#define AsstMsg_Destroyed (5)

static char* translate_utf8_path(const char* utf8_win_path)
{
    if (!utf8_win_path) {
        return NULL;
    }
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8_win_path, -1, NULL, 0);
    if (len <= 0) {
        return NULL;
    }
    wchar_t* wpath = malloc((size_t)len * sizeof(wchar_t));
    if (!wpath) {
        return NULL;
    }
    if (MultiByteToWideChar(CP_UTF8, 0, utf8_win_path, -1, wpath, len) != len) {
        free(wpath);
        return NULL;
    }
    char* unix_path = wine_get_unix_file_name(wpath);
    free(wpath);
    return unix_path;
}

static int win32_free(void* ptr)
{
    return HeapFree(GetProcessHeap(), 0, ptr);
}

// AsstLog 使用 Logger 的短等级标签；核心未加载时退回 stderr。
static void log_warn(const char* message)
{
    if (dl_AsstLog) {
        dl_AsstLog("WRN", message);
    }
    else {
        fprintf(stderr, "MaaWineBridge: %s\n", message);
    }
}

typedef void(
    __stdcall* AsstApiCallbackMsAbi)(AsstMsgId msg, const char* details_json, void* custom_arg);

typedef struct
{
    AsstApiCallbackMsAbi x64abi_callback;
    void* custom_arg;
    TaskQueue* queue;
} CallbackState;

typedef struct
{
    AsstApiCallbackMsAbi x64abi_callback;
    AsstMsgId msg;
    const char* details_json;
    void* custom_arg;
} CallbackInvocation;

static CallbackState* cb_state_new()
{
    CallbackState* fwd_state = calloc(1, sizeof(CallbackState));
    if (!fwd_state) {
        return NULL;
    }
    TaskQueue* queue = task_queue_new(task_queue_worker_wine_new);
    if (!queue) {
        free(fwd_state);
        return NULL;
    }
    fwd_state->queue = queue;
    return fwd_state;
}

static void cb_state_free(CallbackState* fwd_state)
{
    if (!fwd_state) {
        return;
    }
    task_queue_free(fwd_state->queue);
    free(fwd_state);
}

static void invoke_helper(void* arg)
{
    CallbackInvocation* invocation = arg;
    if (invocation->x64abi_callback) {
        invocation->x64abi_callback(invocation->msg, invocation->details_json, invocation->custom_arg);
    }
}

// details_json 仅在回调期间有效，必须等待 Wine worker 执行完再返回。
static void UnixCallbackShim(AsstMsgId msg, const char* details_json, void* custom_arg)
{
    CallbackState* state = (CallbackState*)custom_arg;
    CallbackInvocation invocation = {
        .x64abi_callback = state->x64abi_callback,
        .msg = msg,
        .details_json = details_json,
        .custom_arg = state->custom_arg,
    };
    Task* task = task_queue_push(state->queue, invoke_helper, &invocation);
    if (task) {
        task_wait(task);
        task_release(task);
    }
    else {
        log_warn("callback dropped: could not queue callback");
    }
    // MaaCore 保证 Destroyed 是最后回调且消息线程已 join，可在此释放 state。
    if (msg == AsstMsg_Destroyed) {
        cb_state_free(state);
    }
}

AsstBool __stdcall WineShimAsstSetUserDir(const char* path)
{
    char* unix_path = translate_utf8_path(path);
    if (!unix_path) {
        return 0;
    }
    AsstBool result = dl_AsstSetUserDir(unix_path);
    win32_free(unix_path);
    return result;
}

AsstBool __stdcall WineShimAsstLoadResource(const char* path)
{
    char* unix_path = translate_utf8_path(path);
    if (!unix_path) {
        return 0;
    }
    AsstBool result = dl_AsstLoadResource(unix_path);
    win32_free(unix_path);
    return result;
}

AsstBool __stdcall WineShimAsstSetStaticOption(AsstStaticOptionKey key, const char* value)
{
    return dl_AsstSetStaticOption(key, value);
}

AsstHandle __stdcall WineShimAsstCreate()
{
    return dl_AsstCreate();
}

AsstHandle __stdcall WineShimAsstCreateEx(AsstApiCallbackMsAbi callback, void* custom_arg)
{
    CallbackState* fwd_state = cb_state_new();
    if (!fwd_state) {
        return NULL;
    }
    fwd_state->x64abi_callback = callback;
    fwd_state->custom_arg = custom_arg;
    AsstHandle result = dl_AsstCreateEx(UnixCallbackShim, fwd_state);
    if (!result) {
        cb_state_free(fwd_state);
    }
    return result;
}

void __stdcall WineShimAsstDestroy(AsstHandle handle)
{
    // 同步销毁会 join 正在等待本次回调的消息线程，所以在回调内销毁同一实例会死锁。
    dl_AsstDestroy(handle);
}

AsstBool __stdcall WineShimAsstSetInstanceOption(
    AsstHandle handle,
    AsstInstanceOptionKey key,
    const char* value)
{
    return dl_AsstSetInstanceOption(handle, key, value);
}

AsstBool __stdcall WineShimAsstConnect(
    AsstHandle handle,
    const char* adb_path,
    const char* address,
    const char* config)
{
    if (!address) {
        return 0;
    }
    char* unix_adb_path = translate_utf8_path(adb_path);
    if (!unix_adb_path) {
        return 0;
    }
    AsstBool result = dl_AsstConnect(handle, unix_adb_path, address, config);
    win32_free(unix_adb_path);
    return result;
}

AsstTaskId __stdcall WineShimAsstAppendTask(AsstHandle handle, const char* type, const char* params)
{
    return dl_AsstAppendTask(handle, type, params);
}

AsstBool __stdcall WineShimAsstSetTaskParams(AsstHandle handle, AsstTaskId id, const char* params)
{
    return dl_AsstSetTaskParams(handle, id, params);
}

AsstBool __stdcall WineShimAsstStart(AsstHandle handle)
{
    return dl_AsstStart(handle);
}

AsstBool __stdcall WineShimAsstStop(AsstHandle handle)
{
    return dl_AsstStop(handle);
}

AsstBool __stdcall WineShimAsstRunning(AsstHandle handle)
{
    return dl_AsstRunning(handle);
}

AsstBool __stdcall WineShimAsstConnected(AsstHandle handle)
{
    return dl_AsstConnected(handle);
}

AsstBool __stdcall WineShimAsstBackToHome(AsstHandle handle)
{
    return dl_AsstBackToHome(handle);
}

// Linux 核心无 Win32 窗口绑定；保留失败存根以避免 EntryPointNotFoundException。
AsstBool __stdcall WineShimAsstAttachWindow(
    AsstHandle handle,
    void* hwnd,
    uint64_t screencap_method,
    uint64_t mouse_method,
    uint64_t keyboard_method)
{
    return 0;
}

AsstAsyncCallId __stdcall WineShimAsstAsyncAttachWindow(
    AsstHandle handle,
    void* hwnd,
    uint64_t screencap_method,
    uint64_t mouse_method,
    uint64_t keyboard_method,
    AsstBool block)
{
    return 0;
}

AsstAsyncCallId __stdcall WineShimAsstAsyncConnect(
    AsstHandle handle,
    const char* adb_path,
    const char* address,
    const char* config,
    AsstBool block)
{
    if (!adb_path || !address) {
        return 0;
    }
    char* unix_adb_path;
    if (strchr(adb_path, '/') != NULL || strchr(adb_path, '\\') != NULL) {
        unix_adb_path = translate_utf8_path(adb_path);
        if (!unix_adb_path) {
            return 0;
        }
    }
    else {
        unix_adb_path = (char*)adb_path;
    }
    AsstAsyncCallId result = dl_AsstAsyncConnect(handle, unix_adb_path, address, config, block);
    if (unix_adb_path != adb_path) {
        win32_free(unix_adb_path);
    }
    return result;
}

void __stdcall WineShimAsstSetConnectionExtras(const char* name, const char* extras) {
    dl_AsstSetConnectionExtras(name, extras);
}

AsstAsyncCallId __stdcall WineShimAsstAsyncClick(
    AsstHandle handle,
    int32_t x,
    int32_t y,
    AsstBool block)
{
    return dl_AsstAsyncClick(handle, x, y, block);
}

AsstAsyncCallId __stdcall WineShimAsstAsyncScreencap(AsstHandle handle, AsstBool block)
{
    return dl_AsstAsyncScreencap(handle, block);
}

AsstSize __stdcall WineShimAsstGetImage(AsstHandle handle, void* buff, AsstSize buff_size)
{
    return dl_AsstGetImage(handle, buff, buff_size);
}

AsstSize __stdcall WineShimAsstGetImageBgr(AsstHandle handle, void* buff, AsstSize buff_size)
{
    return dl_AsstGetImageBgr(handle, buff, buff_size);
}

AsstSize __stdcall WineShimAsstGetUUID(AsstHandle handle, char* buff, AsstSize buff_size)
{
    return dl_AsstGetUUID(handle, buff, buff_size);
}

AsstSize __stdcall WineShimAsstGetTasksList(AsstHandle handle, AsstTaskId* buff, AsstSize buff_size)
{
    return dl_AsstGetTasksList(handle, buff, buff_size);
}

AsstSize __stdcall WineShimAsstGetNullSize()
{
    return dl_AsstGetNullSize();
}

const char* __stdcall WineShimAsstGetVersion()
{
    return dl_AsstGetVersion();
}

void __stdcall WineShimAsstLog(const char* level, const char* message)
{
    dl_AsstLog(level, message);
}
