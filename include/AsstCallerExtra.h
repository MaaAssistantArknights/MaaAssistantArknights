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

    struct AsstMapLevelKey ASSTAPI AsstGetMapLevelKey(const char* key);

#ifdef __cplusplus
}
#endif

#endif // ASST_WITH_EXTRA_CALLERS
