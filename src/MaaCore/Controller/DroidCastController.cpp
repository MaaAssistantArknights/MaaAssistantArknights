#include "DroidCastController.h"

#include "Config/GeneralConfig.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"
#include <curl/curl.h>

// 复制自MinitouchController

asst::DroidCastController::~DroidCastController()
{
    LogTraceFunction;

    release_droidcast();
}

void asst::DroidCastController::release_droidcast([[maybe_unused]] bool force)
{
    curl_global_cleanup();
    m_droidcast_handler.reset();
}

bool asst::DroidCastController::call_and_hup_droidcast()
{
    LogTraceFunction;
    release_droidcast(true);

    std::string cmd = m_adb.call_droidcast;
    Log.info(cmd);

    std::string pipe_str;

    m_droidcast_handler = m_platform_io->interactive_shell(cmd);
    if (!m_droidcast_handler) {
        Log.error("unable to start droidcast");
        return false;
    }

    auto check_timeout = [&](const auto& start_time) -> bool {
        using namespace std::chrono_literals;
        return std::chrono::steady_clock::now() - start_time < 3s;
    };

    const auto start_time = std::chrono::steady_clock::now();
    while (true) {
        if (need_exit()) {
            release_droidcast(true);
            return false;
        }

        pipe_str += m_droidcast_handler->read(3);

        if (!check_timeout(start_time)) {
            Log.info("unable to find $ from pipe_str:", Logger::separator::newline, pipe_str);
            release_droidcast(true);
            return false;
        }

        if (pipe_str.find('$') != std::string::npos) {
            break;
        }
    }

    Log.info("pipe str", Logger::separator::newline, pipe_str);

    // TODO: clean the code nonsense to droidcast

    // NOTE: here the droidcast really start up
    return true;
}

std::optional<std::string>
    asst::DroidCastController::reconnect(const std::string& cmd, int64_t timeout, bool recv_by_socket)
{
    LogTraceFunction;

    auto ret = AdbController::reconnect(cmd, timeout, recv_by_socket);
    if (!ret) {
        return std::nullopt;
    }

    call_and_hup_droidcast();
    return ret;
}

void asst::DroidCastController::clear_info() noexcept
{
    AdbController::clear_info();
    m_droidcast_available = false;
}

bool asst::DroidCastController::probe_droidcast(
    const AdbCfg& adb_cfg,
    std::function<std::string(const std::string&)> cmd_replace)
{
    LogTraceFunction;
    auto cast_uuid = m_uuid + "_cast.apk";

    auto droidcast_cmd_rep = [&](const std::string& cfg_cmd) -> std::string {
        using namespace asst::utils::path_literals;
        return utils::string_replace_all(
            cmd_replace(cfg_cmd),
            {
                { "[droidCastRawLocalPath]",
                  utils::path_to_utf8_string(ResDir.get() / "droidcast_raw" / "droidcast_raw.apk") },
                { "[droidCastRawWorkingFile]", cast_uuid },
            });
    };

    if (!call_command(droidcast_cmd_rep(adb_cfg.push_droidcast))) {
        return false;
    }
    if (!call_command(droidcast_cmd_rep(adb_cfg.chmod_droidcast))) {
        return false;
    }
    if (!call_command(droidcast_cmd_rep(adb_cfg.reforward_droidcast_port))) {
        return false;
    }

    m_adb.call_droidcast = droidcast_cmd_rep(adb_cfg.call_droidcast);

    if (!call_and_hup_droidcast()) {
        return false;
    }

    return true;
}

bool asst::DroidCastController::init_curl()
{ // 写回调函数，用于将响应数据追加到 std::string 中
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curl = curl_easy_init();
    std::shared_ptr<CURL> curl_handler(curl, curl_easy_cleanup);
    if (curl_handler) {
        return false;
    }
    else {
        return true;
    }
}

std::optional< std::string > asst::DroidCastController::curl_get()
{
    auto WriteCallback = [&](void* contents, size_t size, size_t nmemb, void* userp) -> size_t {
        size_t total_size = size * nmemb;
        std::string* str = reinterpret_cast<std::string*>(userp);
        str->append(reinterpret_cast<char*>(contents), total_size);
        return total_size;
    };

    std::string response_data;

    // 设置 curl 请求
    curl_easy_setopt(curl_handler.get(), CURLOPT_URL, drc_get_url.c_str());
    curl_easy_setopt(curl_handler.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl_handler.get(), CURLOPT_WRITEDATA, &response_data);
    // 执行请求
    CURLcode res = curl_easy_perform(curl_handler.get());
    if (res == CURLE_OK) {
        return response_data;
    }
    else {
        Log.info("CURL error:", Logger::separator::newline, curl_easy_strerror(res));
        return std::nullopt;
    }
}

bool asst::DroidCastController::connect(
    const std::string& adb_path,
    const std::string& address,
    const std::string& config)
{
    LogTraceFunction;

    release_droidcast();
    init_curl();

#ifdef ASST_DEBUG
    if (config == "DEBUG") {
        m_inited = true;
        return true;
    }
#endif

    auto ret = AdbController::connect(adb_path, address, config);

    if (!ret) {
        return false;
    }

    auto get_info_json = [&]() -> json::value {
        return json::object {
            { "uuid", m_uuid },
            { "details",
              json::object {
                  { "adb", adb_path },
                  { "address", address },
                  { "config", config },
              } },
        };
    };

    std::string display_id;
    std::string nc_address = "10.0.2.2";
    uint16_t nc_port = 0;

    auto cmd_replace = [&](const std::string& cfg_cmd) -> std::string {
        return utils::string_replace_all(
            cfg_cmd,
            {
                { "[Adb]", adb_path },
                { "[AdbSerial]", address },
                { "[DisplayId]", display_id },
                { "[NcPort]", std::to_string(nc_port) },
                { "[NcAddress]", nc_address },
            });
    };

    auto adb_ret = Config.get_adb_cfg(config);

    if (!adb_ret) {
        json::value info = get_info_json() | json::object {
            { "what", "ConnectFailed" },
            { "why", "ConfigNotFound" },
        };
        callback(AsstMsg::ConnectionInfo, info);
#ifdef ASST_DEBUG
        return false;
#else
        Log.error("config ", config, "not found");
        adb_ret = Config.get_adb_cfg("General");
#endif
    }

    const auto& adb_cfg = adb_ret.value();

    m_droidcast_available = probe_droidcast(adb_cfg, cmd_replace);

    if (!m_droidcast_available) {
        json::value info = get_info_json() | json::object {
            { "what", "DroidCast Available" },
            { "why", "" },
        };
        callback(AsstMsg::ConnectionInfo, info);
        return false;
    }

    return true;
}

