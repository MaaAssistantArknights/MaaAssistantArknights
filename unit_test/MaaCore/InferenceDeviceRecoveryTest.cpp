#include <catch2/catch_test_macros.hpp>

#include <stdexcept>
#include <string>

#include "Config/InferenceDeviceRecovery.h"

using asst::InferenceDeviceRecovery;

TEST_CASE("DirectML device removal is detected from stable error identifiers")
{
    REQUIRE(InferenceDeviceRecovery::is_directml_device_removed("HRESULT 887A0005"));
    REQUIRE(InferenceDeviceRecovery::is_directml_device_removed("HRESULT 887a0005"));
    REQUIRE(InferenceDeviceRecovery::is_directml_device_removed("DXGI_ERROR_DEVICE_REMOVED"));
    REQUIRE_FALSE(InferenceDeviceRecovery::is_directml_device_removed("Unhandled ONNX exception"));
}

TEST_CASE("A device removal requests recovery from every inference owner")
{
    bool word_ocr_recovered = false;
    bool char_ocr_recovered = false;
    bool onnx_sessions_recovered = false;

    try {
        throw std::runtime_error("DirectML Run failed with 887A0005");
    }
    catch (const std::exception& e) {
        REQUIRE(InferenceDeviceRecovery::is_directml_device_removed(e.what()));
        const bool recovered = InferenceDeviceRecovery::recover_all(
            [&] {
                word_ocr_recovered = true;
                return true;
            },
            [&] {
                char_ocr_recovered = true;
                return true;
            },
            [&] {
                onnx_sessions_recovered = true;
                return true;
            });
        REQUIRE(recovered);
    }

    REQUIRE(word_ocr_recovered);
    REQUIRE(char_ocr_recovered);
    REQUIRE(onnx_sessions_recovered);
}

TEST_CASE("Recovery still visits every owner and reports when restart is required")
{
    int recovery_attempts = 0;
    const bool recovered = InferenceDeviceRecovery::recover_all(
        [&] {
            ++recovery_attempts;
            return false;
        },
        [&]() -> bool {
            ++recovery_attempts;
            throw std::runtime_error("reset failed");
        },
        [&] {
            ++recovery_attempts;
            return true;
        });

    REQUIRE_FALSE(recovered);
    REQUIRE(recovery_attempts == 3);
}
