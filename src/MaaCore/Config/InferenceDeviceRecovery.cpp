#include "InferenceDeviceRecovery.h"

#include "Config/Miscellaneous/OcrPack.h"
#include "Config/OnnxSessions.h"
#include "Utils/Logger.hpp"

asst::InferenceDeviceRecoveryResult asst::InferenceDeviceRecovery::recover_to_cpu() noexcept
{
    const bool recovered = recover_all(
        [] {
            WordOcr::get_instance().recover_from_gpu_device_removed();
            return true;
        },
        [] {
            CharOcr::get_instance().recover_from_gpu_device_removed();
            return true;
        },
        [] { return OnnxSessions::get_instance().recover_from_gpu_device_removed(); });

    try {
        if (recovered) {
            Log.warn(__FUNCTION__, "DirectML device was removed; inference switched to CPU");
        }
        else {
            Log.error(__FUNCTION__, "DirectML device was removed; safe CPU recovery requires restarting MAA");
        }
    }
    catch (...) {
    }

    return recovered ? InferenceDeviceRecoveryResult::RecoveredToCpu : InferenceDeviceRecoveryResult::RestartRequired;
}
