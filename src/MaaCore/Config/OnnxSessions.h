#pragma once

#include "AbstractResource.h"

#include <unordered_map>

#if __has_include(<onnxruntime_cxx_api.h>)
#include <onnxruntime_cxx_api.h>
#elif __has_include(<onnxruntime/onnxruntime_cxx_api.h>)
#include <onnxruntime/onnxruntime_cxx_api.h>
#else
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#endif

namespace asst
{
    class OnnxSessions final : public SingletonHolder<OnnxSessions>, public AbstractResource
    {
    public:
        virtual ~OnnxSessions();
        virtual bool load(const std::filesystem::path& path) override;

        Ort::Session& get(const std::string& name);
        bool use_cpu();
        bool use_gpu(int device_id);

    private:
        Ort::Env m_env;
        Ort::SessionOptions m_options;
        std::unordered_map<std::string, Ort::Session> m_sessions;
        std::unordered_map<std::string, std::filesystem::path> m_model_paths;
        bool gpu_enabled;
    };
}
