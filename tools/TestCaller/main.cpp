#include "AsstCaller.h"

#include <stdio.h>
#include <string>
#include <filesystem>
#include <iostream>

std::string get_cur_dir()
{
    return std::filesystem::current_path().u8string();
}

int main(int argc, char** argv)
{
    auto ptr = AsstCreate(get_cur_dir().c_str());
    if (ptr == nullptr) {
        return -1;
    }
    auto ret = AsstCatchCustom(ptr, "127.0.0.1:5555");
    if (!ret) {
        std::cout << "connect failed" << std::endl;
        if (ptr) {
            AsstDestroy(ptr);
            ptr = nullptr;
        }
        return -1;
    }

    char ch = 0;
    while (ch != 'q') {
        // AsstAppendStartUp(ptr);
        AsstAppendFight(ptr, "CE-5", 0, 0, 99999);
        // AsstAppendVisit(ptr, true);
        //{
        //     const int required[] = { 3, 4, 5, 6 };
        //     AsstStartRecruitCalc(ptr, required, sizeof(required) / sizeof(int), true);
        // }
        // AsstAppendDebug(ptr);
        //{
        //     const char* order[] = { "Trade", "Mfg", "Dorm" };
        //     AsstAppendInfrast(ptr, 1, order, 3, 0, 0);
        // }
        // AsstAppendProcessTask(ptr, "AwardBegin");
        //{
        //     const int required[] = { 4 };
        //     const int confirm[] = { 3, 4 };
        //     AsstAppendRecruit(ptr, 2, required, sizeof(required) / sizeof(int), confirm, sizeof(confirm) / sizeof(int));
        // }
        AsstStart(ptr);

        ch = getchar();
    }

    if (ptr) {
        AsstDestroy(ptr);
        ptr = nullptr;
    }

    return 0;
}