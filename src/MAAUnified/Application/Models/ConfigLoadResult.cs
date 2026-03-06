namespace MAAUnified.Application.Models;

public sealed class ConfigLoadResult
{
    public required UnifiedConfig Config { get; init; }

    public required bool LoadedFromExistingConfig { get; init; }

    public ImportReport? ImportReport { get; init; }

    public required IReadOnlyList<ConfigValidationIssue> ValidationIssues { get; init; }

    public SchemaMigrationNotice? SchemaMigrationNotice { get; init; }

    public bool HasBlockingValidationIssues => ValidationIssues.Any(i => i.Blocking);
}

public sealed class ConfigValidationIssue
{
    public required string Scope { get; init; }

    public required string Code { get; init; }

    public required string Field { get; init; }

    public required string Message { get; init; }

    public bool Blocking { get; init; } = true;

    public string? ProfileName { get; init; }

    public int? TaskIndex { get; init; }

    public string? TaskName { get; init; }

    public string? SuggestedAction { get; init; }
}

public sealed record SchemaMigrationNotice(
    int CurrentSchemaVersion,
    int LatestSchemaVersion,
    string Message,
    string SuggestedAction);
