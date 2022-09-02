#pragma once

namespace asst
{
    static constexpr const char* Version =
#ifdef MAA_VERSION
        MAA_VERSION;
#else
        "DEBUG VERSION";
#endif
}
