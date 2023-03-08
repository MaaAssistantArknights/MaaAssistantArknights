#include "OnnxSession.h"

#include <array>
#include <filesystem>
#include <string_view>

#include "Utils/Logger.hpp"

bool asst::OnnxSession::load(const std::filesystem::path& path)
{
    LogTraceFunction;
    Log.info("load", path);

    std::string name = utils::path_to_utf8_string(path.stem());
#ifdef _WIN32
    Ort::Session session(m_env, path.wstring().c_str(), m_options);
#else
    Ort::Session session(m_env, path.string().c_str(), m_options);
#endif

    m_sessions.insert_or_assign(std::move(name), std::move(session));

    return true;
}
