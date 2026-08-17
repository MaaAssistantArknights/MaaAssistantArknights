#pragma once

#include "AbstractResource.h"

#include <mutex>
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
    bool use_cpu();
    bool use_gpu(int device_id);

private:
    Ort::Session& get_or_create(const std::string& name);

    Ort::Env m_env;
    Ort::SessionOptions m_options;
    std::unordered_map<std::string, Ort::Session> m_sessions;
    std::unordered_map<std::string, std::filesystem::path> m_model_paths;
    std::unordered_map<std::string, std::size_t> m_session_users;
    std::unordered_set<std::string> m_pending_reload; // 持有期间路径变化、待归零后销毁的会话
    std::mutex m_mutex;
    bool gpu_enabled = false;
};
}
