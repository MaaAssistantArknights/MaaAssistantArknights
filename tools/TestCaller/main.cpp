#include "AsstCaller.h"

#include <filesystem>
#include <iostream>
#include <stdio.h>
#include <string>

std::string get_cur_dir()
{
    return std::filesystem::current_path().u8string();
}

int main(/*int argc, char** argv*/)
{
    // 若使用 VS，请先设置 TestCaller 属性-调试-工作目录为 $(TargetDir)
    AsstLoadResource(get_cur_dir().c_str());

    // 增量更新国际服的资源
    //AsstLoadResource((get_cur_dir() + R"(/resource/international/US)").c_str());

    auto ptr = AsstCreate();
    if (ptr == nullptr) {
        std::cerr << "create failed" << std::endl;
        return -1;
    }

    bool connected = AsstConnect(ptr, "adb", "127.0.0.1:5555", "");
    if (!connected) {
        std::cerr << "connect failed" << std::endl;
        AsstDestroy(ptr);
        ptr = nullptr;

        return -1;
    }

    /* 详细参数可参考 docs / 集成文档.md */

    //AsstAppendTask(ptr, "StartUp", R"({})");

//    AsstAppendTask(ptr, "Fight", R"(
//{
//    "stage": "CE-5"
//}
//    )");

    //    AsstAppendTask(ptr, "Recruit", R"(
    //{
    //    "select":[4],
    //    "confirm":[3,4],
    //    "times":4
    //}
    //    )");
    //
    //    AsstAppendTask(ptr, "Infrast", R"(
    //{
    //    "facility": ["Mfg", "Trade", "Power", "Control", "Reception", "Office", "Dorm"],
    //    "drones": "Money"
    //}
    //)");
    //
    //    AsstAppendTask(ptr, "Visit", R"({})");
    //
    //    AsstAppendTask(ptr, "Mall", R"(
    //{
    //    "shopping": true,
    //    "is_black_list": false,
    //    "shopping_list": [
    //        "家具",
    //        "碳"
    //    ]
    //}
    //)");
    //    AsstAppendTask(ptr, "Award", R"({})");
    //
    //AsstAppendTask(ptr, "Roguelike", R"(
    //{
    //    "opers": [
    //        {
    //            "name": "山",
    //            "skill": 2,
    //            "skill_usage": 1
    //        },
    //        {
    //            "name": "梓兰"
    //        },
    //        {
    //            "name": "芙蓉"
    //        }
    //    ]
    //}
    //)");

    //AsstAppendTask(ptr, "Debug", R"({})");

    AsstAppendTask(ptr, "Copilot", R"(
{
    "stage_name": "如帝国之影",
    "formation": true
}
)");

    AsstStart(ptr);

    std::ignore = getchar();

    AsstStop(ptr);
    AsstDestroy(ptr);
    ptr = nullptr;

    return 0;
}
