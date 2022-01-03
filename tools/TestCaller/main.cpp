#include "AsstCaller.h"

#include <filesystem>
#include <iostream>
#include <stdio.h>
#include <string>

std::string get_cur_dir()
{
    return std::filesystem::current_path().u8string();
}

int main(int argc, char** argv)
{
    // 若使用 VS，请先设置 TestCaller 属性-调试-工作目录为 $(TargetDir)
    auto ptr = AsstCreate(get_cur_dir().c_str());
    if (ptr == nullptr) {
        return -1;
    }
    //auto ret = AsstCatchCustom(ptr, "127.0.0.1:5555");
    auto ret = AsstCatchDefault(ptr);
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
        //    const char* order[] = { "Trade", "Dorm" };
        //    AsstAppendInfrast(ptr, 1, order, 2, "Money", 0.3);
        //}
        // AsstAppendProcessTask(ptr, "AwardBegin");
        //{
        //    const int required[] = { 4 };
        //    const int confirm[] = { 3, 4 };
        //    AsstAppendRecruit(ptr, 3, required, sizeof(required) / sizeof(int), confirm, sizeof(confirm) / sizeof(int), true, true);
        //}
        AsstStart(ptr);

        ch = static_cast<char>(getchar());
    }

    if (ptr) {
        AsstDestroy(ptr);
        ptr = nullptr;
    }

    return 0;
}
