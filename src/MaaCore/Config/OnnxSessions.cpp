#include "OnnxSessions.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <string_view>
#include <thread>

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
    std::lock_guard lock(m_mutex);
    if (auto iter = m_model_paths.find(name); iter == m_model_paths.end() || iter->second != path) {
        m_sessions.erase(name);
        m_model_paths.insert_or_assign(name, path);
    }

    return true;
}

Ort::Session& asst::OnnxSessions::get_or_create(const std::string& name)
{
    if (!m_sessions.contains(name)) {
        Log.info(__FUNCTION__, "lazy load", name);
        Ort::Session session(m_env, m_model_paths.at(name).c_str(), m_options);
        m_sessions.emplace(name, std::move(session));
    }
    return m_sessions.at(name);
}

Ort::Session& asst::OnnxSessions::get(const std::string& name)
{
    std::lock_guard lock(m_mutex);
    return get_or_create(name);
}

Ort::Session& asst::OnnxSessions::acquire(const std::string& name)
{
    std::lock_guard lock(m_mutex);
    Ort::Session& session = get_or_create(name);
    ++m_session_users[name];
    return session;
}

void asst::OnnxSessions::release(const std::string& name)
{
    std::lock_guard lock(m_mutex);
    const auto found = m_session_users.find(name);
    if (found == m_session_users.end()) {
        Log.error(__FUNCTION__, "session was not acquired", name);
        return;
    }
    if (--found->second == 0) {
        m_session_users.erase(found);
        Log.info(__FUNCTION__, "released", name);
    }
}

bool asst::OnnxSessions::use_cpu()
{
    std::lock_guard lock(m_mutex);
    if (!m_sessions.empty()) {
        return false;
    }
    m_options = Ort::SessionOptions();

    int logical = std::max(1u, std::thread::hardware_concurrency());
    int cpu_threads;
    if (logical <= 2) {
        cpu_threads = 1;
    }
    else if (logical <= 4) {
        cpu_threads = 2;
    }
    else if (logical <= 12) {
        cpu_threads = 3;
    }
    else {
        cpu_threads = 4;
    }

    // 设置执行模式为顺序执行，减少线程竞争
    m_options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
    m_options.SetIntraOpNumThreads(cpu_threads);

    Log.info("CPU OCR enabled with", cpu_threads, "threads");

    gpu_enabled = false;
    return true;
}

bool asst::OnnxSessions::use_gpu(int device_id)
{
    std::lock_guard lock(m_mutex);
    if (gpu_enabled) {
        return true;
    }
    if (!m_sessions.empty()) {
        return false;
    }
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
