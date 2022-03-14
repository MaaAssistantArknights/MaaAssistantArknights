#include "AsstCaller.h"

#include <string.h>
#include <iostream>

#include <meojson/json.hpp>

#include "Assistant.h"
#include "AsstTypes.h"
#include "Version.h"
#include "Logger.hpp"
#include "Resource.h"

#if 0
#if _MSC_VER
// Win32平台下Dll的入口
BOOL APIENTRY DllMain(HINSTANCE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
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

static bool inited = false;

bool AsstLoadResource(const char* path)
{
    bool log_inited = asst::Logger::set_dirname(std::string(path) + "/");
    bool resrc_inited = asst::Resrc.load(std::string(path) + "/resource/");
    if (!resrc_inited) {
        std::cerr << asst::Resrc.get_last_error() << std::endl;
    }
    inited = log_inited && resrc_inited;
    return inited;
}

AsstHandle AsstCreate()
{
    if (!inited) {
        return nullptr;
    }
    return new asst::Assistant();
}

AsstHandle AsstCreateEx(AsstApiCallback callback, void* custom_arg)
{
    if (!inited) {
        return nullptr;
    }
    return new asst::Assistant(callback, custom_arg);
}

void AsstDestroy(AsstHandle handle)
{
    if (handle == nullptr) {
        return;
    }

    delete handle;
    handle = nullptr;
}

bool AsstConnect(AsstHandle handle, const char* adb_path, const char* address, const char* config)
{
    if (!inited || handle == nullptr) {
        return false;
    }

    return handle->connect(adb_path, address, config ? config : std::string());
}

bool AsstStart(AsstHandle handle)
{
    if (!inited || handle == nullptr) {
        return false;
    }

    return handle->start();
}

bool AsstStop(AsstHandle handle)
{
    if (!inited || handle == nullptr) {
        return false;
    }

    return handle->stop();
}

TaskId AsstAppendTask(AsstHandle handle, const char* type, const char* params)
{
    if (!inited || handle == nullptr) {
        return 0;
    }

    return handle->append_task(type, params);
}

bool AsstSetTaskParams(AsstHandle handle, TaskId id, const char* params)
{
    if (!inited || handle == nullptr) {
        return 0;
    }

    return handle->set_task_params(id, params);
}

unsigned long long AsstGetImage(AsstHandle handle, void* buff, unsigned long long buff_size)
{
    if (!inited || handle == nullptr || buff == nullptr) {
        return 0;
    }
    auto img_data = handle->get_image();
    size_t data_size = img_data.size();
    if (buff_size < data_size) {
        return 0;
    }
    memcpy(buff, img_data.data(), data_size);
    return data_size;
}

bool AsstCtrlerClick(AsstHandle handle, int x, int y, bool block)
{
    if (!inited || handle == nullptr) {
        return false;
    }
    return handle->ctrler_click(x, y, block);
}

//bool AsstSetParam(AsstHandle handle, const char* type, const char* param, const char* value)
//{
//    if (handle == nullptr) {
//        return false;
//    }
//
//    return handle->set_param(type, param, value);
//}

const char* AsstGetVersion()
{
    return asst::Version;
}

void AsstLog(const char* level, const char* message)
{
    if (!inited) {
        std::cerr << "Not inited" << std::endl;
        return;
    }
    asst::Log.log_with_custom_level(level, message);
}
