using System.Text.Json;
using MAAUnified.Application.Models;

namespace MAAUnified.Application.Configuration;

public sealed class AvaloniaJsonConfigStore : IUnifiedConfigStore
{
    private static readonly JsonSerializerOptions _serializerOptions = new()
    {
        WriteIndented = true,
        PropertyNameCaseInsensitive = true,
    };

    public AvaloniaJsonConfigStore(string baseDirectory)
    {
        BaseDirectory = baseDirectory;
        ConfigDirectory = Path.Combine(baseDirectory, "config");
        ConfigPath = Path.Combine(ConfigDirectory, "avalonia.json");
    }

    public string BaseDirectory { get; }

    public string ConfigDirectory { get; }

    public string ConfigPath { get; }

    public bool Exists() => File.Exists(ConfigPath);

    public async Task<UnifiedConfig?> LoadAsync(CancellationToken cancellationToken = default)
    {
        if (!Exists())
        {
            return null;
        }

        await using var stream = File.OpenRead(ConfigPath);
        return await JsonSerializer.DeserializeAsync<UnifiedConfig>(stream, _serializerOptions, cancellationToken);
    }

    public async Task SaveAsync(UnifiedConfig config, CancellationToken cancellationToken = default)
    {
        Directory.CreateDirectory(ConfigDirectory);
        await using var stream = File.Create(ConfigPath);
        await JsonSerializer.SerializeAsync(stream, config, _serializerOptions, cancellationToken);
    }

    public Task BackupAsync(string suffix, CancellationToken cancellationToken = default)
    {
        if (!Exists())
        {
            return Task.CompletedTask;
        }

        var backupPath = ConfigPath + suffix;
        File.Copy(ConfigPath, backupPath, true);
        return Task.CompletedTask;
    }
}
