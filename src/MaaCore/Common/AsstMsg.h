#pragma once

#include <functional>
#include <ostream>
#include <unordered_map>

#include <meojson/json.hpp>

namespace asst
{
enum class AsstMsg
{
    /* Global Info */
    InternalError = 0, // 内部错误
    InitFailed,        // 初始化失败
    ConnectionInfo,    // 连接相关错误
    AllTasksCompleted, // 全部任务完成
    AsyncCallInfo,     // 外部异步调用信息
    Destroyed,         // 实例已销毁
    /* TaskChain Info */
    TaskChainError = 10000, // 任务链执行/识别错误
    TaskChainStart,         // 任务链开始
    TaskChainCompleted,     // 任务链完成
    TaskChainExtraInfo,     // 任务链额外信息
    TaskChainStopped,       // 任务链停止（手动停止）
    /* SubTask Info */
    SubTaskError = 20000, // 原子任务执行/识别错误
    SubTaskStart,         // 原子任务开始
    SubTaskCompleted,     // 原子任务完成
    SubTaskExtraInfo,     // 原子任务额外信息
    SubTaskStopped,       // 原子任务停止（手动停止）
};

inline std::ostream& operator<<(std::ostream& os, const AsstMsg& type)
{
    static const std::unordered_map<AsstMsg, std::string> _type_name = {
        /* Global Info */
        { AsstMsg::InternalError, "InternalError" },
        { AsstMsg::InitFailed, "InitFailed" },
        { AsstMsg::ConnectionInfo, "ConnectionInfo" },
        { AsstMsg::AllTasksCompleted, "AllTasksCompleted" },
        { AsstMsg::AsyncCallInfo, "AsyncCallInfo" },
        { AsstMsg::Destroyed, "Destroyed" },
        /* TaskChain Info */
        { AsstMsg::TaskChainError, "TaskChainError" },
        { AsstMsg::TaskChainStart, "TaskChainStart" },
        { AsstMsg::TaskChainCompleted, "TaskChainCompleted" },
        { AsstMsg::TaskChainExtraInfo, "TaskChainExtraInfo" },
        { AsstMsg::TaskChainStopped, "TaskChainStopped" },
        /* SubTask Info */
        { AsstMsg::SubTaskError, "SubTaskError" },
        { AsstMsg::SubTaskStart, "SubTaskStart" },
        { AsstMsg::SubTaskCompleted, "SubTaskCompleted" },
        { AsstMsg::SubTaskExtraInfo, "SubTaskExtraInfo" },
        { AsstMsg::SubTaskStopped, "SubTaskStopped" },
    };
    return os << _type_name.at(type);
}

// 对外的回调接口
using AsstMsgId = int32_t;
using ApiCallback = void (*)(AsstMsgId msg, const char* details_json, void* custom_arg);

// 内部使用的回调
class Assistant;
using AsstCallback = std::function<void(AsstMsg msg, const json::value& details, Assistant* inst)>;
}
