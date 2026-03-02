using MAAUnified.Application.Models;

namespace MAAUnified.Application.Configuration;

public interface IUnifiedConfigStore
{
    string ConfigPath { get; }

    bool Exists();

    Task<UnifiedConfig?> LoadAsync(CancellationToken cancellationToken = default);

    Task SaveAsync(UnifiedConfig config, CancellationToken cancellationToken = default);

    Task BackupAsync(string suffix, CancellationToken cancellationToken = default);
}
