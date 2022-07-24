#include "AsstCaller.h"

#include <cstring>
#include <iostream>

#include <meojson/json.hpp>

#include "Assistant.h"
#include "AsstTypes.h"
#include "Version.h"
#include "Logger.hpp"
#include "Resource.h"

static constexpr unsigned long long NullSize = static_cast<unsigned long long>(-1);

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
#ifdef _WIN32
    std::string working_path = asst::utils::utf8_to_ansi(path);
#else
    std::string working_path = std::string(path);
#endif
    bool log_inited = asst::Logger::set_dirname(working_path + "/");
    bool resrc_inited = asst::Resrc.load(working_path + "/resource/");
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

#ifdef _WIN32
    return handle->connect(asst::utils::utf8_to_ansi(adb_path), address, config ? config : std::string());
#else
    return handle->connect(adb_path, address, config ? config : std::string());
#endif
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

    return handle->append_task(type, params ? params : "");
}

bool AsstSetTaskParams(AsstHandle handle, TaskId id, const char* params)
{
    if (!inited || handle == nullptr) {
        return false;
    }

    return handle->set_task_params(id, params ? params : "");
}

bool AsstCtrlerClick(AsstHandle handle, int x, int y, bool block)
{
    if (!inited || handle == nullptr) {
        return false;
    }
    return handle->ctrler_click(x, y, block);
}

unsigned long long AsstGetImage(AsstHandle handle, void* buff, unsigned long long buff_size)
{
    if (!inited || handle == nullptr || buff == nullptr) {
        return NullSize;
    }
    auto img_data = handle->get_image();
    size_t data_size = img_data.size();
    if (buff_size < data_size) {
        return NullSize;
    }
    memcpy(buff, img_data.data(), data_size * sizeof(decltype(img_data)::value_type));
    return data_size;
}

unsigned long long AsstGetUUID(AsstHandle handle, char* buff, unsigned long long buff_size)
{
    if (!inited || handle == nullptr || buff == nullptr) {
        return NullSize;
    }
    auto uuid = handle->get_uuid();
    size_t data_size = uuid.size();
    if (buff_size < data_size) {
        return NullSize;
    }
    memcpy(buff, uuid.data(), data_size * sizeof(decltype(uuid)::value_type));
    return data_size;
}

unsigned long long AsstGetTasksList(AsstHandle handle, TaskId* buff, unsigned long long buff_size)
{
    if (!inited || handle == nullptr || buff == nullptr) {
        return NullSize;
    }
    auto tasks = handle->get_tasks_list();
    size_t data_size = tasks.size();
    if (buff_size < data_size) {
        return NullSize;
    }
    memcpy(buff, tasks.data(), data_size * sizeof(decltype(tasks)::value_type));
    return data_size;
}

unsigned long long AsstGetNullSize()
{
    return NullSize;
}

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
