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
        InternalError = 0,          // 内部错误
        InitFailed,                 // 初始化失败
        ConnectionInfo,             // 连接相关错误
        AllTasksCompleted,          // 全部任务完成
        /* TaskChain Info */
        TaskChainError = 10000,     // 任务链执行/识别错误
        TaskChainStart,             // 任务链开始
        TaskChainCompleted,         // 任务链完成
        TaskChainExtraInfo,         // 任务链额外信息
        /* SubTask Info */
        SubTaskError = 20000,       // 原子任务执行/识别错误
        SubTaskStart,               // 原子任务开始
        SubTaskCompleted,           // 原子任务完成
        SubTaskExtraInfo            // 原子任务额外信息
    };

    inline std::ostream& operator<<(std::ostream& os, const AsstMsg& type)
    {
        static const std::unordered_map<AsstMsg, std::string> _type_name = {
            /* Global Info */
            { AsstMsg::InternalError, "InternalError" },
            { AsstMsg::InitFailed, "InitFailed" },
            { AsstMsg::ConnectionInfo, "ConnectionInfo" },
            { AsstMsg::AllTasksCompleted, "AllTasksCompleted" },
            /* TaskChain Info */
            { AsstMsg::TaskChainError, "TaskChainError" },
            { AsstMsg::TaskChainStart, "TaskChainStart" },
            { AsstMsg::TaskChainCompleted, "TaskChainCompleted" },
            { AsstMsg::TaskChainExtraInfo, "TaskChainExtraInfo" },
            /* SubTask Info */
            { AsstMsg::SubTaskError, "SubTaskError" },
            { AsstMsg::SubTaskStart, "SubTaskStart" },
            { AsstMsg::SubTaskCompleted, "SubTaskCompleted" },
            { AsstMsg::SubTaskExtraInfo, "SubTaskExtraInfo" },
        };
        return os << _type_name.at(type);
    }

    // AsstCallback 消息回调函数
    // 参数：
    // AsstMsg 消息类型
    // const json::value& 消息详情json，每种消息不同，Todo，需要补充个协议文档啥的
    // void* 外部调用者自定义参数，每次回调会带出去，建议传个(void*)this指针进来
    using AsstCallback = std::function<void(AsstMsg, const json::value&, void*)>;

    using AsstApiCallback = void(*)(int msg, const char* detail_json, void* custom_arg);
}
