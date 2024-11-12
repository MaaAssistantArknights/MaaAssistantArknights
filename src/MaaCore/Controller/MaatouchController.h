#pragma once

#include "MinitouchController.h"

namespace asst
{
class MaatouchController : public MinitouchController
{
public:
    MaatouchController(const AsstCallback& callback, Assistant* inst, PlatformType type) :
        MinitouchController(callback, inst, type)
    {
        m_use_maa_touch = true;
    }

    MaatouchController(const MaatouchController&) = delete;
    MaatouchController(MaatouchController&&) = delete;
    virtual ~MaatouchController() = default;
};
}
