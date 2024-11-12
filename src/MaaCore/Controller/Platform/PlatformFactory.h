#pragma once

#include "AdbLiteIO.h"
#ifdef _WIN32
#include "Win32IO.h"
#else
#include "PosixIO.h"
#endif

namespace asst
{
class PlatformFactory
{
public:
    static std::shared_ptr<PlatformIO> create_platform(Assistant* inst, PlatformType type)
    {
        switch (type) {
        case PlatformType::AdbLite:
            return std::make_shared<AdbLiteIO>(inst);
        case PlatformType::Native:
            return std::make_shared<NativeIO>(inst);
        default:
            return nullptr;
        }
    }
};
}
