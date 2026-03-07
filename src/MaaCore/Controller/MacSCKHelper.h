#pragma once

#ifdef ASST_WITH_MAC_SCK

#include <array>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace asst
{
class MacSCKHelper
{
public:
    MacSCKHelper();
    ~MacSCKHelper();

    bool init(std::string_view bundle_id, std::string_view port, std::pair<int, int> size, std::array<int16_t, 8> rect);
    bool capture(std::vector<uint8_t>& bgrData) const;

    static bool hasPermission();
    static bool requestPermission();

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};
} // namespace asst

#endif // ASST_WITH_MAC_SCK
