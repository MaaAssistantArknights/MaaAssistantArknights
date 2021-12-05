#pragma once

#include <functional>
#include <memory>
#include <ostream>
#include <unordered_map>

#include <json.h>

namespace asst
{
    enum class AsstMsg
    {
        /* Error Msg */
        PtrIsNull,       // 指针为空
        ImageIsEmpty,    // 图像为空
        WindowMinimized, // [已弃用] 窗口被最小化了
        InitFaild,       // 初始化失败
        TaskError,       // 任务错误（任务一直出错，retry次数达到上限）
        OcrResultError,  // Ocr识别结果错误
        /* Info Msg: about Task */
        TaskStart = 1000,      // 任务开始
        TaskMatched,           // 任务匹配成功
        ReachedLimit,          // 单个原子任务达到次数上限
        ReadyToSleep,          // 准备开始睡眠
        EndOfSleep,            // 睡眠结束
        AppendProcessTask,     // [已弃用] 新增流程任务，Assistance内部消息，外部不需要处理
        AppendTask,            // [已弃用] 新增任务，Assistance内部消息，外部不需要处理
        TaskCompleted,         // 单个原子任务完成
        PrintWindow,           // 截图消息
        ProcessTaskStopAction, // 流程任务执行到了Stop的动作
        TaskChainCompleted,    // 任务链完成
        ProcessTaskNotMatched, // 流程任务识别错误
        AllTasksCompleted,     // 所有任务完成
        TaskChainStart,        // 开始任务链
        /* Info Msg: about Identify */
        TextDetected = 2000, // 识别到文字
        ImageFindResult,     // 查找图像的结果
        ImageMatched,        // 图像匹配成功
        StageDrops,          // 关卡掉落信息
        /* Open Recruit Msg */
        RecruitTagsDetected = 3000, // 公招识别到了Tags
        RecruitSpecialTag,          // 公招识别到了特殊的Tag
        RecruitResult,              // 公开招募结果
        RecruitSelected,            // 选择了Tags
        /* Infrast Msg */
        InfrastSkillsDetected = 4000,  // 识别到了基建技能（当前页面）
        InfrastSkillsResult,           // 识别到的所有可用技能
        InfrastComb,                   // 当前房间的最优干员组合
        EnterFacility,                 // 进入某个房间
        FacilityInfo,                  // 当前设施信息
    };

    static std::ostream& operator<<(std::ostream& os, const AsstMsg& type)
    {
        static const std::unordered_map<AsstMsg, std::string> _type_name = {
            { AsstMsg::PtrIsNull, "PtrIsNull" },
            { AsstMsg::ImageIsEmpty, "ImageIsEmpty" },
            { AsstMsg::RecruitSelected, "RecruitSelected" },
            { AsstMsg::WindowMinimized, "WindowMinimized" },
            { AsstMsg::InitFaild, "InitFaild" },
            { AsstMsg::TaskStart, "TaskStart" },
            { AsstMsg::ImageFindResult, "ImageFindResult" },
            { AsstMsg::ImageMatched, "ImageMatched" },
            { AsstMsg::StageDrops, "StageDrops" },
            { AsstMsg::TaskMatched, "TaskMatched" },
            { AsstMsg::ReachedLimit, "ReachedLimit" },
            { AsstMsg::ReadyToSleep, "ReadyToSleep" },
            { AsstMsg::EndOfSleep, "EndOfSleep" },
            { AsstMsg::AppendProcessTask, "AppendProcessTask" },
            { AsstMsg::TaskCompleted, "TaskCompleted" },
            { AsstMsg::TaskChainCompleted, "TaskChainCompleted" },
            { AsstMsg::TaskChainStart, "TaskChainStart" },
            { AsstMsg::AllTasksCompleted, "AllTasksCompleted" },
            { AsstMsg::PrintWindow, "PrintWindow" },
            { AsstMsg::TaskError, "TaskError" },
            { AsstMsg::ProcessTaskNotMatched, "ProcessTaskNotMatched" },
            { AsstMsg::ProcessTaskStopAction, "ProcessTaskStopAction" },
            { AsstMsg::TextDetected, "TextDetected" },
            { AsstMsg::RecruitTagsDetected, "RecruitTagsDetected" },
            { AsstMsg::OcrResultError, "OcrResultError" },
            { AsstMsg::RecruitSpecialTag, "RecruitSpecialTag" },
            { AsstMsg::RecruitResult, "RecruitResult" },
            { AsstMsg::AppendTask, "AppendTask" },
            { AsstMsg::InfrastSkillsDetected, "InfrastSkillsDetected" },
            { AsstMsg::InfrastSkillsResult, "InfrastSkillsResult" },
            { AsstMsg::InfrastComb, "InfrastComb" },
            { AsstMsg::EnterFacility, "EnterFacility" },
            { AsstMsg::FacilityInfo, "FacilityInfo" }
        };
        return os << _type_name.at(type);
    }

    // AsstCallback 消息回调函数
    // 参数：
    // AsstMsg 消息类型
    // const json::value& 消息详情json，每种消息不同，Todo，需要补充个协议文档啥的
    // void* 外部调用者自定义参数，每次回调会带出去，建议传个(void*)this指针进来
    using AsstCallback = std::function<void(AsstMsg, const json::value&, void*)>;
}
