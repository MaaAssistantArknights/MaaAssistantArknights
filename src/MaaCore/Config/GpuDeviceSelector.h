#pragma once

#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>

namespace asst
{
class GpuDeviceSelector
{
public:
    static constexpr std::string_view LuidPrefix = "luid:";

    static std::optional<GpuDeviceSelector> parse(std::string_view value)
    {
        GpuDeviceSelector result;

        if (value.starts_with(LuidPrefix)) {
            const auto luid_value = value.substr(LuidPrefix.size());
            if (luid_value.empty() || luid_value.size() > 16) {
                return std::nullopt;
            }

            uint64_t adapter_luid = 0;
            const auto [ptr, ec] =
                std::from_chars(luid_value.data(), luid_value.data() + luid_value.size(), adapter_luid, 16);
            if (ec != std::errc {} || ptr != luid_value.data() + luid_value.size()) {
                return std::nullopt;
            }

            result.m_adapter_luid = adapter_luid;
            return result;
        }

        int device_id = 0;
        const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), device_id);
        if (value.empty() || ec != std::errc {} || ptr != value.data() + value.size() || device_id < 0) {
            return std::nullopt;
        }

        result.m_device_id = device_id;
        return result;
    }

    [[nodiscard]] bool uses_adapter_luid() const noexcept { return m_adapter_luid.has_value(); }

    [[nodiscard]] std::optional<uint64_t> adapter_luid() const noexcept { return m_adapter_luid; }

    [[nodiscard]] int device_id() const noexcept { return m_device_id; }

    [[nodiscard]] std::optional<int> resolve_device_id() const;

    bool operator==(const GpuDeviceSelector&) const = default;

private:
    int m_device_id = 0;
    std::optional<uint64_t> m_adapter_luid;
};
} // namespace asst
