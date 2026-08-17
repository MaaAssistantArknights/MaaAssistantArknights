#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "BlackFlowTaskPort.h"
#include "Vision/Roguelike/BlackFlow/BlackFlowMapAnalyzer.h"

namespace asst::blackflow
{
class BlackFlowMapObservationSource final : public IBlackFlowMapObservationSource
{
public:
    bool prepare(std::string* error = nullptr);
    void release() noexcept;

    bool recognize(
        const cv::Mat& image,
        const BlackFlowObservationRequest& request,
        BlackFlowMapObservation& observation,
        FactStore& observed_facts,
        std::string* error) override;

    void configure_diagnostics(const DiagnosticSettings& settings) override;
    bool persist_diagnostics(const DiagnosticArtifactRequest& request, std::string* error) override;

private:
    std::shared_ptr<const perception::BlackFlowMapAnalyzer> m_analyzer;
    std::string m_initialization_error;
    DiagnosticSettings m_diagnostics;
    perception::MapRecognitionResult m_last_result;
    std::string m_last_attempt_id;
    std::int64_t m_accumulated_screenshot_us = 0;
    std::int64_t m_accumulated_recognition_us = 0;
    int m_last_attempt_count = 0;
    int m_last_retry_count = 0;
    std::uint64_t m_sequence = 0;
};
} // namespace asst::blackflow
