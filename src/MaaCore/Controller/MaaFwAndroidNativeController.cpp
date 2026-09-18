#ifdef __ANDROID__

#include "MaaFwAndroidNativeController.h"

#include "Utils/Logger.hpp"

namespace asst
{
bool MaaFwAndroidNativeController::connect(
    const std::string& adb_path,
    const std::string& address,
    const std::string& config)
{
    auto parsed = json::parse(config);
    if (!parsed || !parsed->is_object()) {
        LogError << "Failed to parse MaaFwAndroidNativeController config as JSON object";
        return false;
    }
    json::object extras = parsed->as_object();
    extras["library_name"] = "MaaAndroidNativeControlUnit";
    return connect_with_extras(adb_path, address, config, extras);
}
} // namespace asst

#endif // __ANDROID__
