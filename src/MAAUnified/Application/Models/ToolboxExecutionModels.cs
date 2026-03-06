namespace MAAUnified.Application.Models;

public enum ToolboxToolKind
{
    Recruit = 0,
    OperBox = 1,
    Depot = 2,
    Gacha = 3,
    VideoRecognition = 4,
    MiniGame = 5,
}

public sealed record ToolboxExecuteRequest(
    ToolboxToolKind Tool,
    string ParameterText,
    TimeSpan? TimeoutOverride = null,
    string? CorrelationId = null);

public sealed record ToolboxExecuteResult(
    ToolboxToolKind Tool,
    string ResultText,
    string ParameterSummary,
    DateTimeOffset CompletedAt);
