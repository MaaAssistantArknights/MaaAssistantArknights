#pragma once

#include <string>

namespace asst
{
// 同一次现场观测中,完成当前培养动作或当前配方需要的库存数量。
// 仅在 item_id 已确认且 0 <= owned < required 时交给补料流程。
struct MissingMaterial
{
    std::string item_id;
    int owned = 0;
    int required = 0;
};
}
