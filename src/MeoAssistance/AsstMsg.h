#pragma once

#include <ostream>
#include <memory>
#include <unordered_map>
#include <functional>

#include <json.h>

namespace asst {
    enum class AsstMsg {
        /* Error Msg */
        PtrIsNull,							// 指针为空
        ImageIsEmpty,						// 图像为空
        WindowMinimized,					// [已弃用] 窗口被最小化了
        InitFaild,							// 初始化失败
        TaskError,							// 任务错误（任务一直出错，retry次数达到上限）
        OcrResultError,						// Ocr识别结果错误
        /* Info Msg: about Task */
        TaskStart = 1000,					// 任务开始
        TaskMatched,						// 任务匹配成功
        ReachedLimit,						// 单个原子任务达到次数上限
        ReadyToSleep,						// 准备开始睡眠
        EndOfSleep,							// 睡眠结束
        AppendProcessTask,					// 新增流程任务，Assistance内部消息，外部不需要处理
        AppendTask,							// 新增任务，Assistance内部消息，外部不需要处理
        TaskCompleted,						// 单个原子任务完成
        PrintWindow,						// 截图消息
        ProcessTaskStopAction,				// 流程任务执行到了Stop的动作
        TaskChainCompleted,					// 任务链完成
        ProcessTaskNotMatched,				// 流程任务识别错误
        /* Info Msg: about Identify */
        TextDetected = 2000,				// 识别到文字
        ImageFindResult,					// 查找图像的结果
        ImageMatched,						// 图像匹配成功
        /* Open Recruit Msg */
        RecruitTagsDetected = 3000,			// 公招识别到了Tags
        RecruitSpecialTag,					// 公招识别到了特殊的Tag
        RecruitResult,						// 公开招募结果
        /* Infrast Msg */
        OpersDetected = 4000,				// 识别到了干员s
        OpersIdtfResult,					// 干员识别结果（总的）
        InfrastComb,						// 当前房间的最优干员组合
        EnterStation,						// 进入某个房间
        StationInfo,						// 当前房间信息
        ReadyToShift,						// 准备换班
        ShiftCompleted,						// 换班完成（单个房间）
        NoNeedToShift						// 无需换班（单个房间）
    };

    static std::ostream& operator<<(std::ostream& os, const AsstMsg& type)
    {
        static const std::unordered_map<AsstMsg, std::string> _type_name = {
            {AsstMsg::PtrIsNull, "PtrIsNull"},
            {AsstMsg::ImageIsEmpty, "ImageIsEmpty"},
            {AsstMsg::WindowMinimized, "WindowMinimized"},
            {AsstMsg::InitFaild, "InitFaild"},
            {AsstMsg::TaskStart, "TaskStart"},
            {AsstMsg::ImageFindResult, "ImageFindResult"},
            {AsstMsg::ImageMatched, "ImageMatched"},
            {AsstMsg::TaskMatched, "TaskMatched"},
            {AsstMsg::ReachedLimit, "ReachedLimit"},
            {AsstMsg::ReadyToSleep, "ReadyToSleep"},
            {AsstMsg::EndOfSleep, "EndOfSleep"},
            {AsstMsg::AppendProcessTask, "AppendProcessTask"},
            {AsstMsg::TaskCompleted, "TaskCompleted"},
            {AsstMsg::TaskChainCompleted, "TaskChainCompleted"},
            {AsstMsg::PrintWindow, "PrintWindow"},
            {AsstMsg::TaskError, "TaskError"},
            {AsstMsg::ProcessTaskNotMatched, "ProcessTaskNotMatched"},
            {AsstMsg::ProcessTaskStopAction, "ProcessTaskStopAction"},
            {AsstMsg::TextDetected, "TextDetected"},
            {AsstMsg::RecruitTagsDetected, "RecruitTagsDetected"},
            {AsstMsg::OcrResultError, "OcrResultError"},
            {AsstMsg::RecruitSpecialTag, "RecruitSpecialTag"},
            {AsstMsg::RecruitResult, "RecruitResult"},
            {AsstMsg::AppendTask, "AppendTask"},
            {AsstMsg::OpersDetected, "OpersDetected"},
            {AsstMsg::OpersIdtfResult, "OpersIdtfResult"},
            {AsstMsg::InfrastComb, "InfrastComb"},
            {AsstMsg::EnterStation, "EnterStation"},
            {AsstMsg::StationInfo, "StationInfo"},
            {AsstMsg::ReadyToShift, "ReadyToShift"},
            {AsstMsg::ShiftCompleted, "ShiftCompleted"},
            {AsstMsg::NoNeedToShift, "NoNeedToShift"}
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