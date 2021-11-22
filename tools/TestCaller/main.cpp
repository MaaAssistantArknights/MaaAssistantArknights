#include "AsstCaller.h"
#include <stdio.h>

int main(int argc, char** argv)
{
    void* ptr = AsstCreate();
    auto ret = AsstCatchEmulator(ptr);
    if (!ret) {
        getchar();
        if (ptr) {
            AsstDestory(ptr);
            ptr = nullptr;
        }
        return -1;
    }

    char ch = 0;
    while (ch != 'q') {
        //AsstAppendSanity(ptr);
        //AsstAppendVisit(ptr, true);
        //{
        //    const int required[] = { 3, 4, 5, 6 };
        //    AsstAppendRecruiting(ptr, required, sizeof(required) / sizeof(int), true);
        //}
        //AsstAppendDebugTask(ptr);
        //{
        //    const char* order[] = { "Trade", "Mfg", "Dorm" };
        //    AsstAppendInfrastShift(ptr, 1, order, 3, 0, 0);
        //}
        AsstAppendProcessTask(ptr, "AwardBegin");
        AsstStart(ptr);
        
        ch = getchar();
    }

    if (ptr) {
        AsstDestory(ptr);
        ptr = nullptr;
    }

    return 0;
}