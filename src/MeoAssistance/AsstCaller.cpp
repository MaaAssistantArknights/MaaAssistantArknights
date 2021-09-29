#include "AsstCaller.h"

#include <string.h>

#include <json_value.h>

#include "Version.h"
#include "AsstAux.h"
#include "Assistance.h"

#if 0
#if _MSC_VER
// Win32平台下Dll的入口
BOOL APIENTRY DllMain(HANDLE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
) {
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(lpReserved);
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
#elif VA_GNUC

#endif
#endif

AsstCallback _callback = nullptr;

void CallbackTrans(asst::AsstMsg msg, const json::value& json, void* custom_arg)
{
    _callback(static_cast<int>(msg), asst::Utf8ToGbk(json.to_string()).c_str(), custom_arg);
}

void* AsstCreate()
{
    try {
        return new asst::Assistance();
    }
    catch (...) {
        return nullptr;
    }
}

void* AsstCreateEx(AsstCallback callback, void* custom_arg)
{
    try {
        // 创建多实例回调会有问题，有空再慢慢整
        _callback = callback;
        return new asst::Assistance(CallbackTrans, custom_arg);
    }
    catch (...) {
        return nullptr;
    }
}

void AsstDestory(void* p_asst)
{
    if (p_asst == nullptr) {
        return;
    }

    delete p_asst;
    p_asst = nullptr;
}

bool AsstCatchDefault(void* p_asst)
{
    if (p_asst == nullptr) {
        return false;
    }

    return ((asst::Assistance*)p_asst)->catch_default();
}

bool AsstCatchEmulator(void* p_asst)
{
    if (p_asst == nullptr) {
        return false;
    }

    return ((asst::Assistance*)p_asst)->catch_emulator();
}

bool AsstCatchUSB(void* p_asst)
{
    if (p_asst == nullptr) {
        return false;
    }

    return ((asst::Assistance*)p_asst)->catch_usb();
}

bool AsstCatchRemote(void* p_asst, const char* address)
{
    if (p_asst == nullptr) {
        return false;
    }

    return ((asst::Assistance*)p_asst)->catch_remote(address);
}

bool AsstStartSanity(void* p_asst)
{
    if (p_asst == nullptr) {
        return false;
    }

    return ((asst::Assistance*)p_asst)->start_sanity();
}

bool AsstStartVisit(void* p_asst, bool with_shopping)
{
    if (p_asst == nullptr) {
        return false;
    }

    return ((asst::Assistance*)p_asst)->start_visit(with_shopping);
}

bool AsstStartProcessTask(void* p_asst, const char* task)
{
    if (p_asst == nullptr) {
        return false;
    }

    return ((asst::Assistance*)p_asst)->start_process_task(task, asst::Assistance::ProcessTaskRetryTimesDefault);
}

void AsstStop(void* p_asst)
{
    if (p_asst == nullptr) {
        return;
    }

    ((asst::Assistance*)p_asst)->stop();
}

bool AsstSetParam(void* p_asst, const char* type, const char* param, const char* value)
{
    if (p_asst == nullptr) {
        return false;
    }

    return ((asst::Assistance*)p_asst)->set_param(type, param, value);
}

const char* AsstGetVersion()
{
    return asst::Version;
}

bool AsstStartOpenRecruit(void* p_asst, const int required_level[], int required_len, bool set_time)
{
    if (p_asst == nullptr) {
        return false;
    }
    std::vector<int> level_vector;
    level_vector.assign(required_level, required_level + required_len);
    return ((asst::Assistance*)p_asst)->start_open_recruit(level_vector, set_time);
}

bool AsstStartIndertifyOpers(void* p_asst)
{
    if (p_asst == nullptr) {
        return false;
    }
    return ((asst::Assistance*)p_asst)->start_to_identify_opers();
}

bool AsstStartInfrast(void* p_asst)
{
    if (p_asst == nullptr) {
        return false;
    }
    return ((asst::Assistance*)p_asst)->start_infrast();
}

bool AsstStartDebugTask(void* p_asst)
{
    if (p_asst == nullptr) {
        return false;
    }
#if LOG_TRACE
    return ((asst::Assistance*)p_asst)->start_debug_task();
#else
    return false;
#endif // LOG_TRACE
}