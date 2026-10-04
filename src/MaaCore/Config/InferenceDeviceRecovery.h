#pragma once

#include <string_view>
#include <utility>

namespace asst
{
enum class InferenceDeviceRecoveryResult
{
    RecoveredToCpu,
    RestartRequired,
};

class InferenceDeviceRecovery
{
public:
    [[nodiscard]] static bool is_directml_device_removed(std::string_view message) noexcept
    {
        return message.find("887A0005") != std::string_view::npos ||
               message.find("887a0005") != std::string_view::npos ||
               message.find("DXGI_ERROR_DEVICE_REMOVED") != std::string_view::npos;
    }

    template <typename... Recoveries>
    [[nodiscard]] static bool recover_all(Recoveries&&... recoveries) noexcept
    {
        bool recovered = true;
        const auto recover = [&](auto&& recovery) {
            try {
                recovered = static_cast<bool>(recovery()) && recovered;
            }
            catch (...) {
                recovered = false;
            }
        };
        (recover(std::forward<Recoveries>(recoveries)), ...);
        return recovered;
    }

    [[nodiscard]] static InferenceDeviceRecoveryResult recover_to_cpu() noexcept;
};
}
