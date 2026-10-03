#include "WebGpuDevice.h"

#include <cstdint>
#include <format>
#include <iterator>
#include <memory>
#include <mutex>
#include <vector>

#include "Utils/Logger.hpp"

#if __has_include(<onnxruntime_c_api.h>)
#include <onnxruntime_c_api.h>
#elif __has_include(<onnxruntime/onnxruntime_c_api.h>)
#include <onnxruntime/onnxruntime_c_api.h>
#else
#include <onnxruntime/core/session/onnxruntime_c_api.h>
#endif

#if defined(ASST_WITH_WEBGPU) && defined(_WIN32)
#include <dawn/native/D3DBackend.h>
#include <dawn/native/DawnNative.h>
#define ASST_HAS_WEBGPU_DAWN 1
#endif

namespace asst
{
#ifdef ASST_HAS_WEBGPU_DAWN
namespace
{
// Dawn 的 instance 与 device 都是引用计数的，这里持有一份并活到进程结束：
// ORT 的多个会话、FastDeploy 的 det/rec 会话共用同一份设备。
struct DawnContext
{
    // ORT 的 WebGpuContext::Wait() 是 instance_.WaitAny(future, UINT64_MAX)，而 Dawn 只要
    // timeoutNS > 0 就要求 instance 开了 TimedWaitAny（dawn/native/Instance.cpp: APIWaitAny），
    // 否则直接返回 WaitStatus::Error → ORT 报 "Failed to wait for the operation:3"。
    // ORT 自建默认 instance 时（WebGpuContextFactory::CreateContext）也是这么开的。
    DawnContext() : instance(instance_descriptor()) {}

    static const WGPUInstanceDescriptor* instance_descriptor()
    {
        static const WGPUInstanceFeatureName kInstanceFeatures[] = { WGPUInstanceFeatureName_TimedWaitAny };
        static const WGPUInstanceDescriptor kDescriptor = [] {
            WGPUInstanceDescriptor descriptor {};
            descriptor.requiredFeatureCount = std::size(kInstanceFeatures);
            descriptor.requiredFeatures = kInstanceFeatures;
            return descriptor;
        }();
        return &kDescriptor;
    }

    dawn::native::Instance instance;
    WGPUDevice device = nullptr;
};

std::mutex s_context_mutex;
std::unordered_map<std::string, std::shared_ptr<DawnContext>> s_contexts;

std::string context_key(const GpuDeviceSelector& selector, int device_id)
{
    if (selector.uses_adapter_luid()) {
        return std::format("luid:{:016X}", *selector.adapter_luid());
    }
    return std::format("index:{}", device_id);
}

std::shared_ptr<DawnContext> create_context(const GpuDeviceSelector& selector, int device_id)
{
    auto context = std::make_shared<DawnContext>();

    wgpu::RequestAdapterOptions adapter_options {};
    adapter_options.powerPreference = wgpu::PowerPreference::HighPerformance;

    dawn::native::d3d::RequestAdapterOptionsLUID luid_options;
    if (selector.uses_adapter_luid()) {
        const uint64_t luid = *selector.adapter_luid();
        luid_options.adapterLUID.LowPart = static_cast<DWORD>(luid & 0xFFFF'FFFFULL);
        luid_options.adapterLUID.HighPart = static_cast<LONG>(luid >> 32);
        adapter_options.nextInChain = &luid_options;
    }

    auto adapters = context->instance.EnumerateAdapters(&adapter_options);
    if (adapters.empty()) {
        LogError << "no WebGPU adapter found for" << context_key(selector, device_id);
        return nullptr;
    }

    std::size_t index = 0;
    if (!selector.uses_adapter_luid()) {
        if (device_id < 0 || static_cast<std::size_t>(device_id) >= adapters.size()) {
            LogError << "WebGPU adapter index out of range" << device_id << "of" << adapters.size();
            return nullptr;
        }
        index = static_cast<std::size_t>(device_id);
    }

    WGPUAdapter adapter = adapters.at(index).Get();

    // ORT 自建 device 时会按 adapter 请求一批特性和 limits（WebGpuContext::Initialize），
    // 但 device 由调用方提供时它不会补回来，所以这里照做，免得它的内核
    //（subgroups / subgroup matrix / f16 / timestamp query 等）编译不起来
    WGPUSupportedFeatures supported_features {};
    ::wgpuAdapterGetFeatures(adapter, &supported_features);
    const std::vector<WGPUFeatureName> features(
        supported_features.features,
        supported_features.features + supported_features.featureCount);
    ::wgpuSupportedFeaturesFreeMembers(supported_features);

    WGPULimits adapter_limits {};
    const bool has_limits = ::wgpuAdapterGetLimits(adapter, &adapter_limits) == WGPUStatus_Success;
    WGPULimits required_limits {};
    if (has_limits) {
        required_limits.maxBindGroups = adapter_limits.maxBindGroups;
        required_limits.maxComputeWorkgroupStorageSize = adapter_limits.maxComputeWorkgroupStorageSize;
        required_limits.maxComputeWorkgroupsPerDimension = adapter_limits.maxComputeWorkgroupsPerDimension;
        required_limits.maxStorageBuffersPerShaderStage = adapter_limits.maxStorageBuffersPerShaderStage;
        required_limits.maxStorageBufferBindingSize = adapter_limits.maxStorageBufferBindingSize;
        required_limits.maxBufferSize = adapter_limits.maxBufferSize;
        required_limits.maxComputeInvocationsPerWorkgroup = adapter_limits.maxComputeInvocationsPerWorkgroup;
        required_limits.maxComputeWorkgroupSizeX = adapter_limits.maxComputeWorkgroupSizeX;
        required_limits.maxComputeWorkgroupSizeY = adapter_limits.maxComputeWorkgroupSizeY;
        required_limits.maxComputeWorkgroupSizeZ = adapter_limits.maxComputeWorkgroupSizeZ;
    }

    WGPUDeviceDescriptor device_descriptor {};
    device_descriptor.requiredFeatureCount = features.size();
    device_descriptor.requiredFeatures = features.data();
    device_descriptor.requiredLimits = has_limits ? &required_limits : nullptr;

    context->device = adapters.at(index).CreateDevice(&device_descriptor);
    if (context->device == nullptr) {
        LogWarn << "failed to create WebGPU device with" << features.size() << "feature(s), retrying with the default descriptor";
        context->device = adapters.at(index).CreateDevice();
    }
    if (context->device == nullptr) {
        LogError << "failed to create WebGPU device for" << context_key(selector, device_id);
        return nullptr;
    }

    LogInfo << "created WebGPU device for" << context_key(selector, device_id) << "of" << adapters.size() << "adapter(s),"
            << features.size() << "feature(s)";
    return context;
}

// 返回 nullptr 表示创建失败，调用方应退回 CPU
std::shared_ptr<DawnContext> get_context(const GpuDeviceSelector& selector, int device_id)
{
    std::lock_guard lock(s_context_mutex);

    const auto key = context_key(selector, device_id);
    if (auto iter = s_contexts.find(key); iter != s_contexts.end()) {
        return iter->second;
    }

    auto context = create_context(selector, device_id);
    if (context) {
        s_contexts.emplace(key, context);
    }
    return context;
}
} // namespace
#endif // ASST_HAS_WEBGPU_DAWN

std::optional<std::unordered_map<std::string, std::string>> make_webgpu_provider_options(const GpuDeviceSelector& selector)
{
#ifdef ASST_HAS_WEBGPU_DAWN
    const auto device_id = selector.resolve_device_id();
    if (!device_id) {
        return std::nullopt;
    }

    const auto context = get_context(selector, *device_id);
    if (!context) {
        return std::nullopt;
    }

    const auto instance = context->instance.Get();
    const auto& procs = dawn::native::GetProcs();
    // ORT 自己也会持有传入的 instance/device（会话销毁时释放），这里额外自持一份，
    // 保证会话反复建/毁之后句柄依然有效
    procs.instanceAddRef(instance);
    procs.deviceAddRef(context->device);

    std::unordered_map<std::string, std::string> options;
    options.emplace("deviceId", std::to_string(*device_id));
    options.emplace("webgpuInstance", std::to_string(reinterpret_cast<uintptr_t>(instance)));
    options.emplace("webgpuDevice", std::to_string(reinterpret_cast<uintptr_t>(context->device)));
    return options;
#else
    (void)selector;
    LogError << "WebGPU backend is not available in this build";
    return std::nullopt;
#endif
}

bool configure_webgpu_session(OrtSessionOptions* session_options, void* user_data)
{
    if (session_options == nullptr || user_data == nullptr) {
        LogError << "invalid WebGPU session configure callback arguments";
        return false;
    }

    const auto& options = *static_cast<const std::unordered_map<std::string, std::string>*>(user_data);

    std::vector<const char*> keys;
    std::vector<const char*> values;
    keys.reserve(options.size());
    values.reserve(options.size());
    for (const auto& [key, value] : options) {
        keys.emplace_back(key.c_str());
        values.emplace_back(value.c_str());
    }

    const OrtApi* api = OrtGetApiBase()->GetApi(ORT_API_VERSION);
    if (api == nullptr) {
        LogError << "failed to get the onnxruntime C API";
        return false;
    }

    // 这里的 key 不需要写全 "ep.webgpuexecutionprovider." 前缀，ORT 会自己补
    OrtStatus* status =
        api->SessionOptionsAppendExecutionProvider(session_options, "WebGPU", keys.data(), values.data(), keys.size());
    if (status != nullptr) {
        LogError << "failed to append WebGPU execution provider:" << api->GetErrorMessage(status);
        api->ReleaseStatus(status);
        return false;
    }

    LogInfo << "appended WebGPU execution provider to FastDeploy session options";
    return true;
}
} // namespace asst
