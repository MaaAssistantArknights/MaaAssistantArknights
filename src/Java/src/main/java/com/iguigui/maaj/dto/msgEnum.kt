package com.iguigui.maaj.dto

/* Global Info */
val INTERNAL_ERROR = 0          // 内部错误
val INIT_FAILED = 1                    // 初始化失败
val CONNECTION_INFO = 2                // 连接相关信息
val ALL_TASKS_COMPLETED = 3         // 全部任务完成

/* TaskChain Info */
val TASK_CHAIN_ERROR = 10000     // 任务链执行/识别错误
val TASK_CHAIN_START = 10001           // 任务链开始
val TASK_CHAIN_COMPLETED = 10002       // 任务链完成
val TASK_CHAIN_EXTRA_INFO = 10003       // 任务链额外信息

/* SubTask Info */
val SUB_TASK_ERROR = 20000       // 原子任务执行/识别错误
val SUB_TASK_START = 20001             // 原子任务开始
val SUB_TASK_COMPLETED = 20002          // 原子任务完成
val SUB_TASK_EXTRA_INFO = 20003          // 原子任务额外信息
