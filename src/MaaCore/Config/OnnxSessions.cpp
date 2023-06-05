#include "OnnxSessions.h"

#include <array>
#include <filesystem>
#include <string_view>

#include "Utils/Logger.hpp"

bool asst::OnnxSessions::load(const std::filesystem::path& path)
{
    LogTraceFunction;
    Log.info("record path", path);

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
