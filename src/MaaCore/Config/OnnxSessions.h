#pragma once

#include "AbstractResource.h"
#include "GpuDeviceSelector.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#if __has_include(<onnxruntime_cxx_api.h>)
#include <onnxruntime_cxx_api.h>
#elif __has_include(<onnxruntime/onnxruntime_cxx_api.h>)
#include <onnxruntime/onnxruntime_cxx_api.h>
#else
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#endif

namespace asst
{
class OnnxSessions final : public MAA_NS::SingletonHolder<OnnxSessions>, public AbstractResource
{
public:
    virtual ~OnnxSessions();
    virtual bool load(const std::filesystem::path& path) override;

    Ort::Session& get(const std::string& name);

    // 长期持有会话（跨多次推理）时使用：持有期间 load() 不会销毁对应会话，
    // 模型路径变化会被挂起，待引用归零时才销毁旧会话、由下次 acquire 按新路径重建。
    Ort::Session& acquire(const std::string& name);
    void release(const std::string& name);

    // RAII 版本：持有期间会话不会被加速器回退销毁，析构时自动 release。
    class SessionLease
    {
    public:
        SessionLease(OnnxSessions* owner, std::string name, Ort::Session& session) noexcept :
            m_owner(owner),
            m_name(std::move(name)),
            m_session(&session)
        {
        }

        SessionLease(SessionLease&& other) noexcept :
            m_owner(other.m_owner),
            m_name(std::move(other.m_name)),
            m_session(other.m_session)
        {
            other.m_owner = nullptr;
            other.m_session = nullptr;
        }

        SessionLease(const SessionLease&) = delete;
        SessionLease& operator=(const SessionLease&) = delete;
        SessionLease& operator=(SessionLease&&) = delete;

        ~SessionLease()
        {
            if (m_owner != nullptr) {
                m_owner->release(m_name);
            }
        }

        [[nodiscard]] Ort::Session& session() const noexcept { return *m_session; }

    private:
        OnnxSessions* m_owner = nullptr;
        std::string m_name;
        Ort::Session* m_session = nullptr;
    };

    SessionLease acquire_lease(const std::string& name);
    bool use_cpu();
    bool use_gpu(GpuDeviceSelector selector);

    // Pure capability query: never changes the current device. Returns false
    // when the loaded onnxruntime build lacks the OpenVINO provider or the
    // requested OpenVINO device cannot be initialized.
    static bool probe_openvino(OpenVINODevice device);

    // True while sessions are configured to run on the OpenVINO NPU. Callers
    // use this to take the shared NPU inference lock around Ort::Session::Run.
    static bool npu_inference_enabled() noexcept;
    // 探针卡死或丢弃的 NPU 对象过多时，本进程不再使用 NPU，避免继续阻塞或累积。
    static bool npu_unavailable() noexcept;
    static void mark_npu_unavailable(const char* reason) noexcept;

    // 不需要 NPU 锁时 lock 为空且 timed_out 为 false；获取超时时 timed_out 为 true，
    // 调用方应放弃本次推理并回退，而不是无锁运行。
    struct NpuInferenceLock
    {
        std::unique_lock<std::timed_mutex> lock;
        bool timed_out = false;
    };

    static NpuInferenceLock acquire_npu_inference_lock();

private:
    // 以下内部函数均要求调用方已持有 m_mutex
    Ort::Session& get_or_create(const std::string& name);
    void warmup_locked(const std::string& name);
    // OpenVINO NPU 会话析构可能崩溃，需要泄漏保活；CPU 及其它后端直接释放，
    // 仍被调用方持有时延迟到 release 归零后释放。
    void retire_session_locked(const std::string& name, std::shared_ptr<Ort::Session> session, bool in_use);
    // 关闭加速器、清空缓存并改用 CPU 重建指定会话。
    void fallback_to_cpu_locked(const std::string& name);
    bool use_cpu_locked();
    bool use_gpu_locked(GpuDeviceSelector selector);

    int reset_session_options();
    bool initialize_gpu_options();

    Ort::Env m_env;
    Ort::SessionOptions m_options;
    // 会话用 shared_ptr 保持地址稳定：调用方持有 get()/acquire() 返回的引用期间，
    // 缓存重建（模型路径变化或加速器回退）不能销毁对象。
    std::unordered_map<std::string, std::shared_ptr<Ort::Session>> m_sessions;
    std::unordered_map<std::string, std::filesystem::path> m_model_paths;
    std::unordered_map<std::string, std::size_t> m_session_users;
    std::unordered_set<std::string> m_pending_reload; // 持有期间路径变化、待归零后销毁的会话
    std::unordered_set<std::string> m_npu_sessions; // 创建于 OpenVINO NPU 的会话名
    // 已移出缓存但调用方仍持有引用的会话；同名会话归零释放时一并销毁。
    std::unordered_multimap<std::string, std::shared_ptr<Ort::Session>> m_retired_sessions;
    std::optional<GpuDeviceSelector> m_gpu_selector;
    std::mutex m_mutex;
    bool gpu_enabled = false;
    bool gpu_options_initialized = false;
    std::atomic<bool> m_npu_inference_enabled { false };
};
}
