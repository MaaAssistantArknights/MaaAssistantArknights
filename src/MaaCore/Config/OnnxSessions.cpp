#include "OnnxSessions.h"

#include <array>
#include <filesystem>
#include <string_view>

#include "Utils/Logger.hpp"

#if __has_include(<onnxruntime/dml_provider_factory.h>)
#define WITH_DML
#include <onnxruntime/dml_provider_factory.h>
#endif

#if __has_include(<onnxruntime/coreml_provider_factory.h>)
#define WITH_COREML
#include <onnxruntime/coreml_provider_factory.h>
#endif

bool asst::OnnxSessions::load(const std::filesystem::path& path)
{
    LogTraceFunction;
    Log.info("record path", path.lexically_relative(UserDir.get()));

    std::string name = utils::path_to_utf8_string(path.stem());

    if (auto iter = m_model_paths.find(name); iter == m_model_paths.end() || iter->second != path) {
        m_sessions.erase(name);
        m_model_paths.insert_or_assign(name, path);
    }

    return true;
}

Ort::Session& asst::OnnxSessions::get(const std::string& name)
{
    if (m_sessions.find(name) == m_sessions.end()) {
        Log.info(__FUNCTION__, "lazy load", name);
        Ort::Session session(m_env, m_model_paths.at(name).c_str(), m_options);
        m_sessions.emplace(name, std::move(session));
    }
    return m_sessions.at(name);
}

bool asst::OnnxSessions::use_cpu()
{
    if (m_sessions.size() != 0) return false;
    m_options = Ort::SessionOptions();
    gpu_enabled = false;
    return true;
}

bool asst::OnnxSessions::use_gpu(int device_id)
{
    if (gpu_enabled) return true;
    if (m_sessions.size() != 0) return false;
    auto all_providers = Ort::GetAvailableProviders();
    bool support_cuda = false;
    bool support_dml = false;
    bool support_coreml = false;
    for (const auto& provider : all_providers) {
        if (provider == "CUDAExecutionProvider") {
            support_cuda = true;
        }
        if (provider == "DmlExecutionProvider") {
            support_dml = true;
        }
        if (provider == "CoreMLExecutionProvider") {
            support_coreml = true;
        }
    }

    bool any_gpu = support_cuda || support_dml || support_coreml;

    if (support_cuda) {
        OrtCUDAProviderOptions cuda_options;
        cuda_options.device_id = device_id;
        m_options.AppendExecutionProvider_CUDA(cuda_options);
    }
#ifdef WITH_DML
    else if (support_dml) {
        if (!Ort::Status(OrtSessionOptionsAppendExecutionProvider_DML(m_options, device_id)).IsOK()) {
            return false;
        }
    }
#endif
#ifdef WITH_COREML
    else if (support_coreml) {
        if (!Ort::Status(OrtSessionOptionsAppendExecutionProvider_CoreML((OrtSessionOptions*)m_options, 0)).IsOK()) {
            return false;
        }
    }
#endif
    if (!any_gpu) {
        Log.error(__FUNCTION__, "No GPU execution provider available");
        return false;
    }

    gpu_enabled = true;
    return true;
}

asst::OnnxSessions::~OnnxSessions()
{
    // FIXME: intentionally leak ort objects to avoid crash (double free?)
    // https://github.com/microsoft/onnxruntime/issues/15174
    auto leak_sessions = new decltype(m_sessions);
    *leak_sessions = std::move(m_sessions);

    auto leak_options = new Ort::SessionOptions(nullptr);
    *leak_options = std::move(m_options);
}
