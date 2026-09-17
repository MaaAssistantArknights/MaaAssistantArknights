#pragma once

#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>
#include <system_error>

namespace asst
{
enum class InferenceBackend
{
    Auto = 0,
    DirectML = 1,
    WebGPU = 2,
};

class GpuDeviceSelector
{
public:
    static constexpr std::string_view LuidPrefix = "luid:";
    static constexpr std::string_view WebGpuPrefix = "webgpu:";
    static constexpr std::string_view DirectMlPrefix = "directml:";

    static std::optional<GpuDeviceSelector> parse(std::string_view value)
    {
        GpuDeviceSelector result;

        if (value.starts_with(WebGpuPrefix)) {
            result.m_backend = InferenceBackend::WebGPU;
            value.remove_prefix(WebGpuPrefix.size());
        }
        else if (value.starts_with(DirectMlPrefix)) {
            result.m_backend = InferenceBackend::DirectML;
            value.remove_prefix(DirectMlPrefix.size());
        }
        else {
            result.m_backend = InferenceBackend::Auto;
        }

        if (value.starts_with(LuidPrefix)) {
#ifndef _WIN32
            return std::nullopt;
#else
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
#endif
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

    [[nodiscard]] InferenceBackend backend() const noexcept { return m_backend; }

    [[nodiscard]] std::optional<int> resolve_device_id() const;

    bool operator==(const GpuDeviceSelector&) const = default;

private:
    int m_device_id = 0;
    std::optional<uint64_t> m_adapter_luid;
    InferenceBackend m_backend = InferenceBackend::Auto;
};
} // namespace asst
