namespace MAAUnified.Application.Models;

public sealed record EndpointProbeResult(
    string Name,
    string Endpoint,
    bool Success,
    int? StatusCode,
    string Message,
    string? ErrorCode = null);

public sealed record RemoteControlConnectivityRequest(
    string GetTaskEndpoint,
    string ReportEndpoint,
    int PollIntervalMs);

public sealed record RemoteControlConnectivityResult(
    int PollIntervalMs,
    EndpointProbeResult GetTaskProbe,
    EndpointProbeResult ReportProbe);

public sealed record NotificationProviderRequest(
    string Provider,
    string ParametersText);

public sealed record NotificationProviderTestRequest(
    string Provider,
    string ParametersText,
    string Title,
    string Message);
