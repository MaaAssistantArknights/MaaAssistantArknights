using System.Text.Json.Nodes;

namespace MAAUnified.Application.Models.TaskParams;

public sealed record TaskValidationReport
{
    public required int TaskIndex { get; init; }

    public required string TaskName { get; init; }

    public required string NormalizedType { get; init; }

    public required JsonObject CompiledParams { get; init; }

    public required IReadOnlyList<TaskValidationIssue> Issues { get; init; }

    public bool HasBlockingIssues => Issues.Any(i => i.Blocking);
}
