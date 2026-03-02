using System.Text.Json;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;

namespace MAAUnified.Application.Services;

public sealed class UnifiedConfigurationService
{
    private static readonly JsonSerializerOptions _reportOptions = new()
    {
        WriteIndented = true,
    };

    private readonly IUnifiedConfigStore _store;
    private readonly IConfigImporter _guiNewImporter;
    private readonly IConfigImporter _guiImporter;
    private readonly string _baseDirectory;

    public UnifiedConfigurationService(
        IUnifiedConfigStore store,
        IConfigImporter guiNewImporter,
        IConfigImporter guiImporter,
        UiLogService logService,
        string baseDirectory)
    {
        _store = store;
        _guiNewImporter = guiNewImporter;
        _guiImporter = guiImporter;
        _baseDirectory = baseDirectory;
        LogService = logService;
    }

    public UiLogService LogService { get; }

    public UnifiedConfig CurrentConfig { get; private set; } = new();

    public event Action<UnifiedConfig>? ConfigChanged;

    public async Task<ConfigLoadResult> LoadOrBootstrapAsync(CancellationToken cancellationToken = default)
    {
        if (_store.Exists())
        {
            try
            {
                var loaded = await _store.LoadAsync(cancellationToken);
                if (loaded is not null)
                {
                    var migrated = await TryMigrateSchemaAsync(loaded, cancellationToken);
                    if (migrated.SchemaVersion != loaded.SchemaVersion)
                    {
                        var suffix = $".schema-v{loaded.SchemaVersion}.bak.{DateTimeOffset.UtcNow:yyyyMMddHHmmss}";
                        await _store.BackupAsync(suffix, cancellationToken);
                        await _store.SaveAsync(migrated, cancellationToken);
                        LogService.Info($"Migrated config schema {loaded.SchemaVersion} -> {migrated.SchemaVersion}, backup: {_store.ConfigPath}{suffix}");
                    }

                    CurrentConfig = migrated;
                    LogService.Info("Loaded config/avalonia.json and skipped legacy auto import");
                    ConfigChanged?.Invoke(CurrentConfig);

                    return new ConfigLoadResult {
                        Config = CurrentConfig,
                        LoadedFromExistingConfig = true,
                    };
                }

                LogService.Warn("config/avalonia.json exists but could not be parsed; rebuilding avalonia.json from defaults and skipping legacy import");
                CurrentConfig = new UnifiedConfig();
                await _store.SaveAsync(CurrentConfig, cancellationToken);
                ConfigChanged?.Invoke(CurrentConfig);
                return new ConfigLoadResult {
                    Config = CurrentConfig,
                    LoadedFromExistingConfig = true,
                };
            }
            catch (Exception ex)
            {
                LogService.Warn($"Failed to load config/avalonia.json ({ex.Message}); rebuilding defaults and skipping legacy import");
                CurrentConfig = new UnifiedConfig();
                await _store.SaveAsync(CurrentConfig, cancellationToken);
                ConfigChanged?.Invoke(CurrentConfig);
                return new ConfigLoadResult {
                    Config = CurrentConfig,
                    LoadedFromExistingConfig = true,
                };
            }
        }

        var report = await ImportLegacyAsync(ImportSource.Auto, manualImport: false, cancellationToken: cancellationToken);

        return new ConfigLoadResult {
            Config = CurrentConfig,
            LoadedFromExistingConfig = false,
            ImportReport = report,
        };
    }

    public async Task<ImportReport> ImportLegacyAsync(
        ImportSource source,
        bool manualImport,
        CancellationToken cancellationToken = default)
    {
        var report = new ImportReport {
            Source = source,
            StartedAt = DateTimeOffset.UtcNow,
            OutputConfigPath = _store.ConfigPath,
            ReportPath = Path.Combine(_baseDirectory, "debug", "config-import-report.json"),
        };

        try
        {
            if (manualImport && _store.Exists())
            {
                var suffix = $".bak.{DateTimeOffset.UtcNow:yyyyMMddHHmmss}";
                await _store.BackupAsync(suffix, cancellationToken);
                LogService.Info($"Backed up current config to {_store.ConfigPath}{suffix}");
            }

            var snapshot = LegacyConfigSnapshot.FromBaseDirectory(_baseDirectory);
            var config = new UnifiedConfig();

            var importPlan = BuildImportPlan(source);
            bool importedAny = false;

            foreach (var step in importPlan)
            {
                if (!step.Importer.CanImport(snapshot))
                {
                    continue;
                }

                await step.Importer.ImportAsync(snapshot, config, report, step.FillMissingOnly, cancellationToken);
                importedAny = true;
            }

            if (!importedAny)
            {
                report.DefaultFallbackCount += 1;
                report.Warnings.Add("No legacy config file found, generated default avalonia.json");
            }

            config.SchemaVersion = UnifiedConfig.LatestSchemaVersion;
            config.Migration = new UnifiedMigrationMetadata {
                ImportedAt = DateTimeOffset.UtcNow,
                ImportedBy = "MAAUnified",
                ImportedFromGuiNew = report.ImportedGuiNew,
                ImportedFromGui = report.ImportedGui,
                Warnings = [.. report.Warnings],
            };

            await _store.SaveAsync(config, cancellationToken);
            CurrentConfig = config;
            ConfigChanged?.Invoke(CurrentConfig);

            report.Success = report.Errors.Count == 0;
            LogService.Info($"Legacy import complete: {report.Summary}");
        }
        catch (Exception ex)
        {
            report.Errors.Add(ex.Message);
            report.Success = false;
            LogService.Error($"Legacy import failed: {ex.Message}");
        }
        finally
        {
            report.FinishedAt = DateTimeOffset.UtcNow;
            await WriteReportAsync(report, cancellationToken);
        }

        return report;
    }

    private Task<UnifiedConfig> TryMigrateSchemaAsync(UnifiedConfig loaded, CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();

        if (loaded.SchemaVersion >= UnifiedConfig.LatestSchemaVersion)
        {
            return Task.FromResult(loaded);
        }

        var migrated = new UnifiedConfig
        {
            SchemaVersion = UnifiedConfig.LatestSchemaVersion,
            CurrentProfile = loaded.CurrentProfile,
            GlobalValues = loaded.GlobalValues,
            Profiles = new Dictionary<string, UnifiedProfile>(StringComparer.OrdinalIgnoreCase),
            Migration = loaded.Migration ?? new UnifiedMigrationMetadata(),
        };

        foreach (var (profileName, profile) in loaded.Profiles)
        {
            var migratedProfile = new UnifiedProfile
            {
                Values = profile.Values,
            };

            foreach (var task in profile.TaskQueue)
            {
                if (!LegacyTaskSchemaConverter.TryUpgradeTaskToSchemaV2(task, profile, loaded, out var convertedTask, out var error))
                {
                    migrated.Migration.Warnings.Add(error ?? $"Task `{task.Name}` migration failed.");
                    LogService.Warn(error ?? $"Task `{task.Name}` migration failed.");
                }

                migratedProfile.TaskQueue.Add(convertedTask);
            }

            migrated.Profiles[profileName] = migratedProfile;
        }

        return Task.FromResult(migrated);
    }

    private List<(IConfigImporter Importer, bool FillMissingOnly)> BuildImportPlan(ImportSource source)
    {
        return source switch
        {
            ImportSource.GuiNewOnly => [(_guiNewImporter, false)],
            ImportSource.GuiOnly => [(_guiImporter, false)],
            _ => [(_guiNewImporter, false), (_guiImporter, true)],
        };
    }

    private async Task WriteReportAsync(ImportReport report, CancellationToken cancellationToken)
    {
        var reportDir = Path.GetDirectoryName(report.ReportPath);
        if (!string.IsNullOrEmpty(reportDir))
        {
            Directory.CreateDirectory(reportDir);
        }

        await using var stream = File.Create(report.ReportPath);
        await JsonSerializer.SerializeAsync(stream, report, _reportOptions, cancellationToken);
    }
}
