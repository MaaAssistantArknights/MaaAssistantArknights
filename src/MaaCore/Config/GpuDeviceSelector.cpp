#include "GpuDeviceSelector.h"

#include <format>

#include "MaaUtils/Encoding.h"
#include "Utils/Logger.hpp"

#ifdef _WIN32
#include <dxgi.h>
#include <wrl/client.h>

#endif

std::optional<int> asst::GpuDeviceSelector::resolve_device_id() const
{
    if (!m_adapter_luid) {
        return m_device_id;
    }

#ifdef _WIN32
    Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
    const auto factory_hr = CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf()));
    if (FAILED(factory_hr)) {
        Log.error(__FUNCTION__, "CreateDXGIFactory1 failed", std::format("0x{:08X}", factory_hr));
        return std::nullopt;
    }

    for (UINT index = 0;; ++index) {
        Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
        const auto enum_hr = factory->EnumAdapters(index, adapter.GetAddressOf());
        if (enum_hr == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        if (FAILED(enum_hr)) {
            Log.error(__FUNCTION__, "EnumAdapters failed", index, std::format("0x{:08X}", enum_hr));
            return std::nullopt;
        }

        DXGI_ADAPTER_DESC desc {};
        if (FAILED(adapter->GetDesc(&desc))) {
            continue;
        }

        const auto adapter_luid = (static_cast<uint64_t>(static_cast<uint32_t>(desc.AdapterLuid.HighPart)) << 32) |
                                  static_cast<uint64_t>(desc.AdapterLuid.LowPart);
        if (adapter_luid != *m_adapter_luid) {
            continue;
        }

        Log.info(
            __FUNCTION__,
            "resolved adapter LUID",
            std::format("{:016X}", adapter_luid),
            "to device id",
            index,
            MAA_NS::from_u16(std::wstring_view(desc.Description)));
        return static_cast<int>(index);
    }

    Log.error(__FUNCTION__, "adapter LUID not found", std::format("{:016X}", *m_adapter_luid));
#else
    Log.error(__FUNCTION__, "adapter LUID selectors are only supported on Windows");
#endif

    return std::nullopt;
}
