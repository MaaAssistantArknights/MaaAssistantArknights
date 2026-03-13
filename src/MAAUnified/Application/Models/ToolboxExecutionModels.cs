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

public sealed record ToolboxRecruitRequest(
    IReadOnlyList<int> SelectLevels,
    bool AutoSetTime,
    int Level3Time,
    int Level4Time,
    int Level5Time,
    string ServerType);

public sealed record ToolboxGachaRequest(bool Once);

public sealed record ToolboxMiniGameRequest(string TaskName);

public sealed record ToolboxDispatchRequest(
    ToolboxToolKind Tool,
    ToolboxRecruitRequest? Recruit = null,
    ToolboxGachaRequest? Gacha = null,
    ToolboxMiniGameRequest? MiniGame = null,
    string? ParameterSummary = null);

public sealed record ToolboxDispatchResult(
    ToolboxToolKind Tool,
    string ParameterSummary,
    DateTimeOffset StartedAt,
    int? CoreTaskId = null,
    string? TaskType = null);
