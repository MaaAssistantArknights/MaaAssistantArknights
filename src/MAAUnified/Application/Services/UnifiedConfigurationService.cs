using System.Text.Json;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services.TaskParams;

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
    private List<ConfigValidationIssue> _currentValidationIssues = [];

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

    public IReadOnlyList<ConfigValidationIssue> CurrentValidationIssues => _currentValidationIssues;

    public bool HasBlockingValidationIssues => _currentValidationIssues.Any(i => i.Blocking);

    public event Action<UnifiedConfig>? ConfigChanged;

    public IReadOnlyList<ConfigValidationIssue> RevalidateCurrentConfig(bool logIssues = false)
    {
        var issues = ValidateCurrentConfig();
        UpdateValidationIssues(issues);
        if (logIssues)
        {
            LogValidationIssues(issues);
        }

        return CurrentValidationIssues;
    }

    public bool TryGetCurrentProfile(out UnifiedProfile profile)
    {
        return CurrentConfig.Profiles.TryGetValue(CurrentConfig.CurrentProfile, out profile!);
    }

    public async Task SaveAsync(CancellationToken cancellationToken = default)
    {
        CurrentConfig.SchemaVersion = UnifiedConfig.LatestSchemaVersion;
        await _store.SaveAsync(CurrentConfig, cancellationToken);
        var validationIssues = ValidateCurrentConfig();
        UpdateValidationIssues(validationIssues);
        LogValidationIssues(validationIssues);
        ConfigChanged?.Invoke(CurrentConfig);
        LogService.Info("Saved config/avalonia.json");
    }

    public async Task<ConfigLoadResult> LoadOrBootstrapAsync(CancellationToken cancellationToken = default)
    {
        if (_store.Exists())
        {
            try
            {
                var loaded = await _store.LoadAsync(cancellationToken);
                if (loaded is not null)
                {
                    CurrentConfig = loaded;
                    if (CurrentConfig.SchemaVersion != UnifiedConfig.LatestSchemaVersion)
                    {
                        LogService.Warn(
                            $"config/avalonia.json schema is {CurrentConfig.SchemaVersion}, latest is {UnifiedConfig.LatestSchemaVersion}. " +
                            "No automatic migration is applied.");
                    }

                    var validationIssues = ValidateCurrentConfig();
                    UpdateValidationIssues(validationIssues);
                    LogValidationIssues(validationIssues);
                    LogService.Info("Loaded config/avalonia.json and skipped legacy auto import");
                    ConfigChanged?.Invoke(CurrentConfig);

                    return new ConfigLoadResult {
                        Config = CurrentConfig,
                        LoadedFromExistingConfig = true,
                        ValidationIssues = validationIssues,
                    };
                }

                LogService.Warn("config/avalonia.json exists but could not be parsed; rebuilding avalonia.json from defaults and skipping legacy import");
                CurrentConfig = new UnifiedConfig();
                await _store.SaveAsync(CurrentConfig, cancellationToken);
                var rebuildIssues = ValidateCurrentConfig();
                UpdateValidationIssues(rebuildIssues);
                LogValidationIssues(rebuildIssues);
                ConfigChanged?.Invoke(CurrentConfig);
                return new ConfigLoadResult {
                    Config = CurrentConfig,
                    LoadedFromExistingConfig = true,
                    ValidationIssues = rebuildIssues,
                };
            }
            catch (Exception ex)
            {
                LogService.Warn($"Failed to load config/avalonia.json ({ex.Message}); rebuilding defaults and skipping legacy import");
                CurrentConfig = new UnifiedConfig();
                await _store.SaveAsync(CurrentConfig, cancellationToken);
                var fallbackIssues = ValidateCurrentConfig();
                UpdateValidationIssues(fallbackIssues);
                LogValidationIssues(fallbackIssues);
                ConfigChanged?.Invoke(CurrentConfig);
                return new ConfigLoadResult {
                    Config = CurrentConfig,
                    LoadedFromExistingConfig = true,
                    ValidationIssues = fallbackIssues,
                };
            }
        }

        var report = await ImportLegacyAsync(ImportSource.Auto, manualImport: false, cancellationToken: cancellationToken);
        var importValidationIssues = ValidateCurrentConfig();
        UpdateValidationIssues(importValidationIssues);
        LogValidationIssues(importValidationIssues);

        return new ConfigLoadResult {
            Config = CurrentConfig,
            LoadedFromExistingConfig = false,
            ImportReport = report,
            ValidationIssues = importValidationIssues,
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
            var validationIssues = ValidateCurrentConfig();
            UpdateValidationIssues(validationIssues);
            LogValidationIssues(validationIssues);
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

    private IReadOnlyList<ConfigValidationIssue> ValidateCurrentConfig()
    {
        var issues = new List<ConfigValidationIssue>();

        if (CurrentConfig.Profiles.Count == 0)
        {
            issues.Add(new ConfigValidationIssue
            {
                Scope = "ConfigLoad",
                Code = "ProfileMissing",
                Field = "profiles",
                Message = "No profile was found in config.",
                Blocking = true,
                SuggestedAction = "Create a new profile and reconfigure task queue.",
            });
            return issues;
        }

        if (!CurrentConfig.Profiles.ContainsKey(CurrentConfig.CurrentProfile))
        {
            issues.Add(new ConfigValidationIssue
            {
                Scope = "ConfigLoad",
                Code = "CurrentProfileMissing",
                Field = "current_profile",
                Message = $"Current profile `{CurrentConfig.CurrentProfile}` is missing.",
                Blocking = true,
                SuggestedAction = "Switch to an existing profile or recreate the current profile.",
            });
        }

        foreach (var (profileName, profile) in CurrentConfig.Profiles)
        {
            for (var index = 0; index < profile.TaskQueue.Count; index++)
            {
                var task = profile.TaskQueue[index];
                var compiled = TaskParamCompiler.CompileTask(task, profile, CurrentConfig, strict: true);
                foreach (var issue in compiled.Issues)
                {
                    issues.Add(new ConfigValidationIssue
                    {
                        Scope = "TaskValidation",
                        Code = issue.Code,
                        Field = issue.Field,
                        Message = issue.Message,
                        Blocking = issue.Blocking,
                        ProfileName = profileName,
                        TaskIndex = index,
                        TaskName = task.Name,
                        SuggestedAction = issue.Blocking
                            ? $"Open task `{task.Name}` ({TaskParamCompiler.NormalizeTaskType(task.Type)}) and save again."
                            : null,
                    });
                }
            }
        }

        return issues;
    }

    private void LogValidationIssues(IReadOnlyList<ConfigValidationIssue> issues)
    {
        if (issues.Count == 0)
        {
            return;
        }

        LogService.Warn($"Detected {issues.Count} config validation issue(s).");
        foreach (var issue in issues)
        {
            var location = issue.TaskIndex is int taskIndex
                ? $"profile={issue.ProfileName},taskIndex={taskIndex},taskName={issue.TaskName}"
                : $"profile={issue.ProfileName ?? "-"}";
            LogService.Warn(
                $"[{issue.Scope}] {issue.Code} field={issue.Field} blocking={issue.Blocking} {location} message={issue.Message}");
        }
    }

    private void UpdateValidationIssues(IReadOnlyList<ConfigValidationIssue> issues)
    {
        _currentValidationIssues = [.. issues];
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
