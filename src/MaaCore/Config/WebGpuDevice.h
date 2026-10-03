#pragma once

#include "GpuDeviceSelector.h"

#include <optional>
#include <string>
#include <unordered_map>

// onnxruntime_c_api.h，这里只用到指针，避免在头文件里引入 ORT
struct OrtSessionOptions;

namespace asst
{
// ONNX Runtime 的 WebGPU EP 在 deviceId > 0 时要求调用方自带 WebGPU(Dawn) 的 instance/device
// （onnxruntime/core/providers/webgpu/webgpu_context.cc: "custom context (contextId>0) must have
// custom WebGPU instance and device"），只给 deviceId 会直接失败。
// 这里按 GpuDeviceSelector（LUID 或索引）建好 Dawn 设备并在进程内复用，
// 返回可直接喂给 AppendExecutionProvider("WebGPU", options) 的 provider options。
std::optional<std::unordered_map<std::string, std::string>> make_webgpu_provider_options(const GpuDeviceSelector& selector);

// FastDeploy `OrtBackendOption::configure_session_callback` 的实现。
// user_data 指向 make_webgpu_provider_options 的返回值，生命周期需覆盖会话创建。
bool configure_webgpu_session(OrtSessionOptions* session_options, void* user_data);
} // namespace asst
