#pragma once

namespace asst
{
    constexpr static const char* Version =
#ifdef MAA_VERSION
        MAA_VERSION;
#else
        "DEBUG VERSION";
#endif
}
