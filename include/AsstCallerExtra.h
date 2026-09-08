#pragma once

// Experimental C API.
//
// This interface is currently developed primarily for use by MaaMacGui.
// It is not part of the supported public API and may be changed or removed
// without notice or compatibility guarantees.

#if ASST_WITH_EXTRA_CALLERS

#include "AsstCaller.h"

struct AsstMapLevelKey
{
    const char* stage_id;
    const char* code;
    const char* level_id;
    const char* name;
};

#ifdef __cplusplus
extern "C"
{
#endif

    // 根据关卡名、code、stageId 或 levelId 查找匹配的关卡。
    //
    // 如果找不到匹配的关卡，返回的结构体中所有字段均为 NULL。
    //
    // 结构体内的字符串指针由 MaaCore 持有，调用方不应释放它们。
    // 调用方如果需要在多次调用间保留内容，应自行复制。
    struct AsstMapLevelKey ASSTAPI AsstGetMapLevelKey(const char* key);

    // 根据物品 Id 查找物品名称。
    //
    // 对于空 Id，返回 "Unknown"。
    // 对于找不到的 Id，返回空字符串。
    //
    // 返回的字符串指针由 MaaCore 持有，调用方不应释放它。
    // 调用方如果需要在多次调用间保留内容，应自行复制。
    const char* ASSTAPI AsstGetItemName(const char* id);

#ifdef __cplusplus
}
#endif

#endif // ASST_WITH_EXTRA_CALLERS
