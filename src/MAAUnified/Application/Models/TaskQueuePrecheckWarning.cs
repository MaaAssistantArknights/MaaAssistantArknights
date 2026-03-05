namespace MAAUnified.Application.Models;

public sealed record TaskQueuePrecheckWarning(
    string Code,
    string Message,
    string Scope,
    bool Blocking = false);
