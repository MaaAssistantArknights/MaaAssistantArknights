#include "AsstCaller.h"

#include <stdio.h>
#include <string>

#include <Windows.h>

std::string get_cur_dir()
{
    char exepath_buff[_MAX_PATH] = { 0 };
    ::GetModuleFileNameA(NULL, exepath_buff, _MAX_PATH);
    std::string exepath(exepath_buff);
    std::string cur_dir = exepath.substr(0, exepath.find_last_of('\\') + 1);
    return cur_dir;
}

int main(int argc, char** argv)
{
    auto ptr = AsstCreate(get_cur_dir().c_str());
    auto ret = AsstCatchEmulator(ptr);
    if (!ret) {
        getchar();
        if (ptr) {
            AsstDestroy(ptr);
            ptr = nullptr;
        }
        return -1;
    }

    char ch = 0;
    while (ch != 'q') {
        //AsstAppendStartUp(ptr);
        AsstAppendFight(ptr, "CE-5", 0, 0, 99999);
        //AsstAppendVisit(ptr, true);
        //{
        //    const int required[] = { 3, 4, 5, 6 };
        //    AsstStartRecruitCalc(ptr, required, sizeof(required) / sizeof(int), true);
        //}
        //AsstAppendDebug(ptr);
        //{
        //    const char* order[] = { "Trade", "Mfg", "Dorm" };
        //    AsstAppendInfrast(ptr, 1, order, 3, 0, 0);
        //}
        //AsstAppendProcessTask(ptr, "AwardBegin");
        //{
        //    const int required[] = { 4 };
        //    const int confirm[] = { 3, 4 };
        //    AsstAppendRecruit(ptr, 2, required, sizeof(required) / sizeof(int), confirm, sizeof(confirm) / sizeof(int));
        //}
        AsstStart(ptr);

        ch = getchar();
    }

    if (ptr) {
        AsstDestroy(ptr);
        ptr = nullptr;
    }

    return 0;
}