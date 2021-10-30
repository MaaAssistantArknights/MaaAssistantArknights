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
        //AsstStartSanity(ptr);
        //AsstStartVisit(ptr, true);
        //{
        //    const int required[] = { 3, 4, 5, 6 };
        //    AsstStartRecruiting(ptr, required, sizeof(required) / sizeof(int), true);
        //}
        AsstStartDebugTask(ptr);
        //AsstStartInfrastShift(ptr);

        ch = getchar();
    }

    if (ptr) {
        AsstDestory(ptr);
        ptr = nullptr;
    }

    return 0;
}