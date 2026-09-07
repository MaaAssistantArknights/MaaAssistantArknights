#include "MaaFwAdbController.h"

namespace asst
{
bool MaaFwAdbController::connect(
    const std::string& adb_path,
    const std::string& address,
    const std::string& config)
{
    return connect_with_extras(
        adb_path,
        address,
        config,
        json::object {
            { "library_name", "MaaAdbControlUnit" },
        });
}
} // namespace asst
