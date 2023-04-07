#pragma once

#include <stdint.h>

struct AsstExtAPI;
typedef struct AsstExtAPI* AsstHandle;

typedef uint8_t AsstBool;
typedef uint64_t AsstSize;
typedef int32_t AsstMsgId;
typedef int32_t AsstTaskId;
typedef int32_t AsstAsyncCallId;
typedef int32_t AsstStaticOptionKey;
typedef int32_t AsstInstanceOptionKey;
typedef int64_t AsstControlFeat;