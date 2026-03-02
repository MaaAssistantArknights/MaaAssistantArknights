namespace MAAUnified.Application.Models;

public sealed class ConfigLoadResult
{
    public required UnifiedConfig Config { get; init; }

    public required bool LoadedFromExistingConfig { get; init; }

    public ImportReport? ImportReport { get; init; }
}
