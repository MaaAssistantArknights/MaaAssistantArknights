#pragma once

#include "AbstractResource.h"

#include <unordered_map>

#include <onnxruntime/core/session/onnxruntime_cxx_api.h>

namespace asst
{
    class OnnxSessions final : public SingletonHolder<OnnxSessions>, public AbstractResource
    {
    public:
        virtual ~OnnxSessions() override = default;
        virtual bool load(const std::filesystem::path& path) override;

        Ort::Session& get(const std::string& name);

    private:
        Ort::Env m_env;
        Ort::SessionOptions m_options;
        std::unordered_map<std::string, Ort::Session> m_sessions;
        std::unordered_map<std::string, std::filesystem::path> m_model_paths;
    };
}
