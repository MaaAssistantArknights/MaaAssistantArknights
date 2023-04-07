#include "OnnxSessions.h"

#include <array>
#include <filesystem>
#include <string_view>

#include "Utils/Logger.hpp"

bool asst::OnnxSessions::load(const std::filesystem::path& path)
{
    LogTraceFunction;
    Log.info("record path", path);

    if (!std::filesystem::exists(path)) {
        Log.error("file not exist:", path);
        return false;
    }

    std::string name = utils::path_to_utf8_string(path.stem());
    m_model_paths.insert_or_assign(name, path);

    return true;
}

Ort::Session& asst::OnnxSessions::get(const std::string& key)
{
    if (m_sessions.find(key) == m_sessions.end()) {
        const auto& path = m_model_paths.at(key);
        LogTraceScope(std::string(__FUNCTION__) + " | lazy load: " + utils::path_to_utf8_string(path) + " as " + key);
        Ort::Session session(m_env, m_model_paths.at(key).c_str(), m_options);
        m_sessions.emplace(key, std::move(session));
    }
    return m_sessions.at(key);
}
