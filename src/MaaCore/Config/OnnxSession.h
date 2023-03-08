#pragma once

#include "AbstractResource.h"

#include <unordered_map>

#include <onnxruntime/core/session/onnxruntime_cxx_api.h>

namespace asst
{
    class OnnxSession final : public SingletonHolder<OnnxSession>, public AbstractResource
    {
    public:
        virtual ~OnnxSession() override = default;
        virtual bool load(const std::filesystem::path& path) override;

        Ort::Session& get(const std::string& key) { return m_sessions.at(key); }

    private:
        Ort::Env m_env;
        Ort::SessionOptions m_options;
        std::unordered_map<std::string, Ort::Session> m_sessions;
    };
}
