#include "AsstCaller.h"

#include <string.h>
#include <iostream>

#include <meojson/json.hpp>

#include "Assistant.h"
#include "AsstDef.h"
#include "Version.h"

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

AsstCallback _callback = nullptr;

void CallbackTrans(asst::AsstMsg msg, const json::value& json, void* custom_arg)
{
    _callback(static_cast<int>(msg), json.to_string().c_str(), custom_arg);
}

asst::Assistant* AsstCreate(const char* dirname)
{
    if (_callback != nullptr) {
        std::cerr << "An instance of Asst already exists" << std::endl;
        return nullptr;
    }
    try {
        _callback = nullptr;
        return new asst::Assistant(dirname);
    }
    catch (std::exception& e) {
        std::cerr << "create failed: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "create failed: unknown exception" << std::endl;
    }
    return nullptr;
}

asst::Assistant* AsstCreateEx(const char* dirname, AsstCallback callback, void* custom_arg)
{
    if (_callback != nullptr) {
        std::cerr << "An instance of Asst already exists" << std::endl;
        return nullptr;
    }
    try {
        _callback = callback;
        return new asst::Assistant(dirname, CallbackTrans, custom_arg);
    }
    catch (std::exception& e) {
        std::cerr << "create failed: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "create failed: unknown exception" << std::endl;
    }
    return nullptr;
}

void AsstDestroy(asst::Assistant* p_asst)
{
    if (p_asst == nullptr) {
        return;
    }

    delete p_asst;
    p_asst = nullptr;
    _callback = nullptr;
}

bool AsstCatchDefault(asst::Assistant* p_asst)
{
    if (p_asst == nullptr) {
        return false;
    }

    return p_asst->catch_default();
}

bool AsstCatchEmulator(asst::Assistant* p_asst)
{
    if (p_asst == nullptr) {
        return false;
    }

    return p_asst->catch_emulator();
}

bool AsstCatchCustom(asst::Assistant* p_asst, const char* address)
{
    if (p_asst == nullptr) {
        return false;
    }

    return p_asst->catch_custom(address);
}

bool AsstCatchFake(asst::Assistant* p_asst)
{
#ifdef ASST_DEBUG
    if (p_asst == nullptr) {
        return false;
    }

    return p_asst->catch_fake();
#else
    (void*)p_asst;
    return false;
#endif // ASST_DEBUG
}

bool ASSTAPI AsstAppendStartUp(asst::Assistant* p_asst)
{
    if (p_asst == nullptr) {
        return false;
    }

    return p_asst->append_start_up();
}

bool AsstAppendFight(asst::Assistant* p_asst, const char* stage, int max_mecidine, int max_stone, int max_times)
{
    if (p_asst == nullptr) {
        return false;
    }

    return p_asst->append_fight(stage, max_mecidine, max_stone, max_times);
}

bool AsstAppendAward(asst::Assistant* p_asst)
{
    if (p_asst == nullptr) {
        return false;
    }

    return p_asst->append_award();
}

bool AsstAppendVisit(asst::Assistant* p_asst)
{
    if (p_asst == nullptr) {
        return false;
    }

    return p_asst->append_visit();
}

bool AsstAppendMall(asst::Assistant* p_asst, const int black_list[], int list_size)
{
    if (p_asst == nullptr) {
        return false;
    }
    std::vector<int> black_list_vector;
    black_list_vector.assign(black_list, black_list + list_size);
    return p_asst->append_mall(black_list_vector);
}

//bool AsstAppendProcessTask(asst::Assistant* p_asst, const char* task_name)
//{
//    if (p_asst == nullptr) {
//        return false;
//    }
//
//    return p_asst->append_process_task(task_name);
//}

bool AsstStartRecruitCalc(asst::Assistant* p_asst, const int select_level[], int required_len, bool set_time)
{
    if (p_asst == nullptr) {
        return false;
    }
    std::vector<int> level_vector;
    level_vector.assign(select_level, select_level + required_len);
    return p_asst->start_recruit_calc(level_vector, set_time);
}

bool AsstAppendInfrast(asst::Assistant* p_asst, int work_mode, const char** order, int order_size, const char* uses_of_drones, double dorm_threshold)
{
    if (p_asst == nullptr) {
        return false;
    }
    std::vector<std::string> order_vector;
    order_vector.assign(order, order + order_size);

    return p_asst->append_infrast(
        static_cast<asst::infrast::WorkMode>(work_mode),
        order_vector,
        uses_of_drones,
        dorm_threshold);
}

bool AsstAppendRecruit(asst::Assistant* p_asst, int max_times, const int select_level[], int select_len, const int confirm_level[], int confirm_len, bool need_refresh, bool use_expedited)
{
    if (p_asst == nullptr) {
        return false;
    }
    std::vector<int> required_vector;
    required_vector.assign(select_level, select_level + select_len);
    std::vector<int> confirm_vector;
    confirm_vector.assign(confirm_level, confirm_level + confirm_len);

    return p_asst->append_recruit(max_times, required_vector, confirm_vector, need_refresh, use_expedited);
}

bool AsstAppendRoguelike(asst::Assistant* p_asst, int mode)
{
    if (p_asst == nullptr) {
        return false;
    }
    return p_asst->append_roguelike(mode);
}

bool AsstStart(asst::Assistant* p_asst)
{
    if (p_asst == nullptr) {
        return false;
    }

    return p_asst->start();
}

bool AsstStop(asst::Assistant* p_asst)
{
    if (p_asst == nullptr) {
        return false;
    }

    return p_asst->stop();
}

bool AsstSetPenguinId(asst::Assistant* p_asst, const char* id)
{
    if (p_asst == nullptr) {
        return false;
    }

    p_asst->set_penguin_id(id);
    return true;
}

unsigned long long AsstGetImage(asst::Assistant* p_asst, void* buff, unsigned long long buff_size)
{
    if (p_asst == nullptr || buff == nullptr) {
        return 0;
    }
    auto img_data = p_asst->get_image();
    size_t data_size = img_data.size();
    if (buff_size < data_size) {
        return 0;
    }
    memcpy(buff, img_data.data(), data_size);
    return data_size;
}

bool AsstCtrlerClick(asst::Assistant* p_asst, int x, int y, bool block)
{
    if (p_asst == nullptr) {
        return false;
    }
    return p_asst->ctrler_click(x, y, block);
}

//bool AsstSetParam(asst::Assistant* p_asst, const char* type, const char* param, const char* value)
//{
//    if (p_asst == nullptr) {
//        return false;
//    }
//
//    return p_asst->set_param(type, param, value);
//}

const char* AsstGetVersion()
{
    return asst::Version;
}

bool AsstAppendDebug(asst::Assistant* p_asst)
{
    if (p_asst == nullptr) {
        return false;
    }
#ifdef ASST_DEBUG
    return p_asst->append_debug();
#else
    return false;
#endif // ASST_DEBUG
}
