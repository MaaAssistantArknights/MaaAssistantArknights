using MAAUnified.Application.Models;

namespace MAAUnified.Application.Configuration;

public interface IConfigImporter
{
    string Name { get; }

    bool CanImport(LegacyConfigSnapshot snapshot);

    Task ImportAsync(
        LegacyConfigSnapshot snapshot,
        UnifiedConfig target,
        ImportReport report,
        bool fillMissingOnly,
        CancellationToken cancellationToken = default);
}
