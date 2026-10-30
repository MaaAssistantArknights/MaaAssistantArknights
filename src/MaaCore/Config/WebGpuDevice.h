#pragma once

#include "GpuDeviceSelector.h"

#include <optional>
#include <string>
#include <unordered_map>

// onnxruntime_c_api.h，这里只用到指针，避免在头文件里引入 ORT
struct OrtSessionOptions;

namespace asst
{
// ONNX Runtime 的 WebGPU `deviceId` 是 context 缓存键，不是 DXGI 适配器序号。
// 0 保留给 ORT 自建的默认 context，并且禁止再带自定义 instance/device；
// 大于 0 才表示自定义 context，此时必须自带 Dawn 的 instance/device
// （onnxruntime/core/providers/webgpu/webgpu_context.cc）。
// 显卡仍按 GpuDeviceSelector（LUID 或索引）选择。返回的 options 里 deviceId 固定为
// 非 0 的 context 键，可直接喂给 AppendExecutionProvider("WebGPU", options)。
std::optional<std::unordered_map<std::string, std::string>> make_webgpu_provider_options(const GpuDeviceSelector& selector);

// FastDeploy `OrtBackendOption::configure_session_callback` 的实现。
// user_data 指向 make_webgpu_provider_options 的返回值，生命周期需覆盖会话创建。
bool configure_webgpu_session(OrtSessionOptions* session_options, void* user_data);
} // namespace asst
