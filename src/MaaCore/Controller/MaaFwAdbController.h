#pragma once

#include "MaaFwController.h"

namespace asst
{
class MaaFwAdbController final : public MaaFwController
{
public:
    MaaFwAdbController(const AsstCallback& callback, Assistant* inst, PlatformType type) :
        MaaFwController(callback, inst, type)
    {
    }

    bool connect(const std::string& adb_path, const std::string& address, const std::string& config) override;
};
} // namespace asst
