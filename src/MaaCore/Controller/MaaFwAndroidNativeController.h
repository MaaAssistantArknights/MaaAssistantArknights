#pragma once

#ifdef __ANDROID__

#include "MaaFwController.h"

namespace asst
{
class MaaFwAndroidNativeController final : public MaaFwController
{
public:
    MaaFwAndroidNativeController(const AsstCallback& callback, Assistant* inst) :
        MaaFwController(callback, inst, PlatformType::Native)
    {
    }

    bool connect(const std::string& adb_path, const std::string& address, const std::string& config) override;
};
} // namespace asst

#endif // __ANDROID__
