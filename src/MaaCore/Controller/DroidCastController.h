#pragma once

#include "AdbController.h"

#include "Config/GeneralConfig.h"

#include <curl/curl.h>

namespace asst
{
class DroidCastController : public AdbController
{
public:
    DroidCastController(const AsstCallback& callback, Assistant* inst, PlatformType type) :
        AdbController(callback, inst, type)
    {
    }

    DroidCastController(const DroidCastController&) = delete;
    DroidCastController(DroidCastController&&) = delete;
    virtual ~DroidCastController();

    virtual bool connect(const std::string& adb_path, const std::string& address, const std::string& config) override;

    DroidCastController& operator=(const DroidCastController&) = delete;
    DroidCastController& operator=(DroidCastController&&) = delete;

protected:
    virtual std::optional<std::string> reconnect(const std::string& cmd, int64_t timeout, bool recv_by_socket) override;

    bool call_and_hup_droidcast();

    bool probe_droidcast(const AdbCfg& adb_cfg, std::function<std::string(const std::string&)> cmd_replace);

    bool input_to_droidcast(const std::string& cmd);
    void release_droidcast(bool force = false);
    std::shared_ptr<CURL> curl_handler = nullptr;
    bool init_curl();
    std::string drc_get_url = "http://127.0.0.1:53516/screenshot";
    std::optional<std::string> curl_get();

    virtual void clear_info() noexcept override;

    bool m_droidcast_available = false;

    std::shared_ptr<IOHandler> m_droidcast_handler = nullptr;
};
} // namespace asst
