using System.Text.Json;
using System.Text.Json.Nodes;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Services.TaskParams;

namespace MAAUnified.Application.Services;

public sealed class UnifiedConfigurationService
{
    private static readonly JsonSerializerOptions _reportOptions = new()
    {
        WriteIndented = true,
    };
    private const string GuiNewFileName = "gui.new.json";
    private const string GuiFileName = "gui.json";
    private const string ParseNullWarningCode = "ConfigRepair.DeserializeNull";
    private const string ParseExceptionWarningCode = "ConfigRepair.DeserializeException";

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
        return RefreshValidationState(logIssues);
    }

    public bool TryGetCurrentProfile(out UnifiedProfile profile)
    {
        return CurrentConfig.Profiles.TryGetValue(CurrentConfig.CurrentProfile, out profile!);
    }

    public async Task SaveAsync(CancellationToken cancellationToken = default)
    {
        await SaveCoreAsync(CurrentConfig, logSavedConfig: true, cancellationToken);
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
                    var normalizedFightStageCount = NormalizeFightStageSelections(CurrentConfig);
                    if (normalizedFightStageCount > 0)
                    {
                        LogService.Info(
                            $"Normalized {normalizedFightStageCount} legacy Fight stage selector(s) to `{FightStageSelection.CurrentOrLast}`.");
                        if (CurrentConfig.SchemaVersion == UnifiedConfig.LatestSchemaVersion)
                        {
                            try
                            {
                                await _store.SaveAsync(CurrentConfig, cancellationToken);
                                LogService.Info("Persisted normalized fight stage selectors to config/avalonia.json");
                            }
                            catch (Exception ex)
                            {
                                LogService.Warn($"Failed to persist normalized fight stage selectors: {ex.Message}");
                            }
                        }
                    }

                    var schemaMigrationNotice = BuildSchemaMigrationNotice();
                    if (schemaMigrationNotice is not null)
                    {
                        LogService.Warn(
                            $"config/avalonia.json schema is {CurrentConfig.SchemaVersion}, latest is {UnifiedConfig.LatestSchemaVersion}. " +
                            "No automatic migration is applied.");
                    }

                    var validationIssues = RefreshValidationState(logIssues: true);
                    LogService.Info("Loaded config/avalonia.json and skipped legacy auto import");
                    ConfigChanged?.Invoke(CurrentConfig);

                    return new ConfigLoadResult {
                        Config = CurrentConfig,
                        LoadedFromExistingConfig = true,
                        ValidationIssues = validationIssues,
                        SchemaMigrationNotice = schemaMigrationNotice,
                    };
                }

                LogService.Warn(
                    $"[{ParseNullWarningCode}] config/avalonia.json parse returned null; rebuilding defaults and skipping legacy import");
                var rebuildIssues = await SaveCoreAsync(new UnifiedConfig(), logSavedConfig: false, cancellationToken);
                return new ConfigLoadResult {
                    Config = CurrentConfig,
                    LoadedFromExistingConfig = true,
                    ValidationIssues = rebuildIssues,
                    SchemaMigrationNotice = BuildSchemaMigrationNotice(),
                };
            }
            catch (Exception ex)
            {
                LogService.Warn(
                    $"[{ParseExceptionWarningCode}] failed to load config/avalonia.json ({ex.GetType().Name}: {ex.Message}); rebuilding defaults and skipping legacy import");
                var fallbackIssues = await SaveCoreAsync(new UnifiedConfig(), logSavedConfig: false, cancellationToken);
                return new ConfigLoadResult {
                    Config = CurrentConfig,
                    LoadedFromExistingConfig = true,
                    ValidationIssues = fallbackIssues,
                    SchemaMigrationNotice = BuildSchemaMigrationNotice(),
                };
            }
        }

        var report = await ImportLegacyAsync(ImportSource.Auto, manualImport: false, cancellationToken: cancellationToken);
        var importValidationIssues = RefreshValidationState(logIssues: true);

        return new ConfigLoadResult {
            Config = CurrentConfig,
            LoadedFromExistingConfig = false,
            ImportReport = report,
            ValidationIssues = importValidationIssues,
            SchemaMigrationNotice = BuildSchemaMigrationNotice(),
        };
    }

    public async Task<ImportReport> ImportLegacyAsync(
        ImportSource source,
        bool manualImport,
        CancellationToken cancellationToken = default)
    {
        var snapshot = LegacyConfigSnapshot.FromBaseDirectory(_baseDirectory);
        var request = new LegacyImportRequest(
            snapshot,
            source,
            manualImport,
            AllowPartialImport: true);
        return await ImportLegacyAsync(request, cancellationToken);
    }

    public async Task<ImportReport> ImportLegacyAsync(
        LegacyImportRequest request,
        CancellationToken cancellationToken = default)
    {
        var report = new ImportReport {
            Source = request.Source,
            StartedAt = DateTimeOffset.UtcNow,
            OutputConfigPath = _store.ConfigPath,
            ReportPath = Path.Combine(_baseDirectory, "debug", "config-import-report.json"),
        };

        try
        {
            var config = new UnifiedConfig();
            AppendUnselectedMissingFiles(request, report);

            var importPlan = BuildImportPlan(request.Source);

            foreach (var step in importPlan)
            {
                await step.Importer.ImportAsync(request.Snapshot, config, report, step.FillMissingOnly, cancellationToken);
            }

            var importedAny = report.ImportedFiles.Count > 0;
            if (!importedAny && !request.ManualImport)
            {
                report.CreatedDefaultConfig = true;
                report.DefaultFallbackCount += 1;
                report.Warnings.Add("No legacy config file found, generated default avalonia.json");
            }

            if (report.DamagedFiles.Count > 0 && request.ManualImport && !request.AllowPartialImport)
            {
                report.Success = false;
                report.AppliedConfig = false;
                LogImportReport(report, request.ManualImport);
                return report;
            }

            if (!importedAny && !report.CreatedDefaultConfig)
            {
                report.Success = false;
                report.AppliedConfig = false;
                LogImportReport(report, request.ManualImport);
                return report;
            }

            config.SchemaVersion = UnifiedConfig.LatestSchemaVersion;
            NormalizeFightStageSelections(config);
            config.Migration = new UnifiedMigrationMetadata {
                ImportedAt = DateTimeOffset.UtcNow,
                ImportedBy = "MAAUnified",
                ImportedFromGuiNew = report.ImportedGuiNew,
                ImportedFromGui = report.ImportedGui,
                Warnings = [.. report.Warnings],
            };

            if (request.ManualImport && _store.Exists())
            {
                var suffix = $".bak.{DateTimeOffset.UtcNow:yyyyMMddHHmmss}";
                await _store.BackupAsync(suffix, cancellationToken);
                LogService.Info($"Backed up current config to {_store.ConfigPath}{suffix}");
            }

            await SaveCoreAsync(config, logSavedConfig: false, cancellationToken);

            report.AppliedConfig = true;
            report.Success = report.Errors.Count == 0;
            LogImportReport(report, request.ManualImport);
        }
        catch (Exception ex)
        {
            report.Errors.Add(ex.Message);
            report.Success = false;
            report.AppliedConfig = false;
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
        var schemaMigrationNotice = BuildSchemaMigrationNotice();
        if (schemaMigrationNotice is not null)
        {
            issues.Add(new ConfigValidationIssue
            {
                Scope = "ConfigMigration",
                Code = "SchemaOutdated",
                Field = "schema_version",
                Message = schemaMigrationNotice.Message,
                Blocking = false,
                SuggestedAction = schemaMigrationNotice.SuggestedAction,
            });
        }

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

    private SchemaMigrationNotice? BuildSchemaMigrationNotice()
    {
        if (CurrentConfig.SchemaVersion == UnifiedConfig.LatestSchemaVersion)
        {
            return null;
        }

        var current = CurrentConfig.SchemaVersion;
        var latest = UnifiedConfig.LatestSchemaVersion;
        return new SchemaMigrationNotice(
            CurrentSchemaVersion: current,
            LatestSchemaVersion: latest,
            Message: $"Detected outdated schema version v{current}. Configuration will stay on compatible read mode until an explicit save.",
            SuggestedAction: "Review settings and save configuration to migrate to the latest schema. A schema backup will be created before overwrite.");
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

    private IReadOnlyList<ConfigValidationIssue> RefreshValidationState(bool logIssues)
    {
        var issues = ValidateCurrentConfig();
        UpdateValidationIssues(issues);
        if (logIssues)
        {
            LogValidationIssues(issues);
        }

        return CurrentValidationIssues;
    }

    private async Task<IReadOnlyList<ConfigValidationIssue>> SaveCoreAsync(
        UnifiedConfig config,
        bool logSavedConfig,
        CancellationToken cancellationToken)
    {
        var sourceSchemaVersion = config.SchemaVersion;
        if (sourceSchemaVersion != UnifiedConfig.LatestSchemaVersion && _store.Exists())
        {
            var suffix = $".schema-v{sourceSchemaVersion}.bak.{DateTimeOffset.UtcNow:yyyyMMddHHmmss}";
            await _store.BackupAsync(suffix, cancellationToken);
            LogService.Warn(
                $"Schema migration write detected: v{sourceSchemaVersion} -> v{UnifiedConfig.LatestSchemaVersion}. Backup created at {_store.ConfigPath}{suffix}");
        }

        config.SchemaVersion = UnifiedConfig.LatestSchemaVersion;
        await _store.SaveAsync(config, cancellationToken);
        CurrentConfig = config;
        var issues = RefreshValidationState(logIssues: true);
        ConfigChanged?.Invoke(CurrentConfig);

        if (logSavedConfig)
        {
            LogService.Info("Saved config/avalonia.json");
        }

        return issues;
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

    private void LogImportReport(ImportReport report, bool manualImport)
    {
        foreach (var line in ImportReportTextFormatter.BuildLogLines(report, manualImport))
        {
            switch (line.Level.ToUpperInvariant())
            {
                case "ERROR":
                    LogService.Error(line.Message);
                    break;
                case "WARN":
                case "WARNING":
                    LogService.Warn(line.Message);
                    break;
                default:
                    LogService.Info(line.Message);
                    break;
            }
        }

        LogService.Info($"Legacy import report: {report.Summary}");
    }

    private static void AppendUnselectedMissingFiles(LegacyImportRequest request, ImportReport report)
    {
        if (request.Source == ImportSource.GuiOnly && !request.Snapshot.GuiNewExists)
        {
            AddReportFile(report.MissingFiles, GuiNewFileName);
        }

        if (request.Source == ImportSource.GuiNewOnly && !request.Snapshot.GuiExists)
        {
            AddReportFile(report.MissingFiles, GuiFileName);
        }
    }

    private static void AddReportFile(ICollection<string> collection, string fileName)
    {
        foreach (var existing in collection)
        {
            if (string.Equals(existing, fileName, StringComparison.OrdinalIgnoreCase))
            {
                return;
            }
        }

        collection.Add(fileName);
    }

    private static int NormalizeFightStageSelections(UnifiedConfig config)
    {
        var normalizedCount = 0;
        foreach (var profile in config.Profiles.Values)
        {
            foreach (var task in profile.TaskQueue)
            {
                if (!string.Equals(TaskParamCompiler.NormalizeTaskType(task.Type), TaskModuleTypes.Fight, StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                if (!task.Params.TryGetPropertyValue("stage", out var stageNode)
                    || stageNode is not JsonValue stageValue)
                {
                    continue;
                }

                if (!stageValue.TryGetValue(out string? stage))
                {
                    continue;
                }

                var normalizedStage = FightStageSelection.NormalizeStoredValue(stage);
                if (string.Equals(stage, normalizedStage, StringComparison.Ordinal))
                {
                    continue;
                }

                task.Params["stage"] = normalizedStage;
                normalizedCount += 1;
            }
        }

        return normalizedCount;
    }
}
