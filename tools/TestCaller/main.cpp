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
    AsstLoadResource(get_cur_dir().c_str());

    auto ptr = AsstCreate();
    if (ptr == nullptr) {
        return -1;
    }
    //auto ret = AsstCatchCustom(ptr, "127.0.0.1:5555");
    auto ret = AsstConnect(ptr, "adb", "127.0.0.1:5555", nullptr);
    if (!ret) {
        std::cout << "connect failed" << std::endl;
        if (ptr) {
            AsstDestroy(ptr);
            ptr = nullptr;
        }
        return -1;
    }

    const char* params = R"(
{
    "stage": "CE-5"
}
)";

    char ch = 0;
    while (ch != 'q') {
        AsstAppendTask(ptr, "Fight", params);
        AsstStart(ptr);

        ch = static_cast<char>(getchar());
    }

    if (ptr) {
        AsstDestroy(ptr);
        ptr = nullptr;
    }

    return 0;
}
