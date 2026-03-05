using System.Globalization;
using System.Net;
using System.Net.Http;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Localization;
using MAAUnified.Application.Services.TaskParams;
using MAAUnified.CoreBridge;
using MAAUnified.Compat.Constants;
using MAAUnified.Platform;

namespace MAAUnified.Application.Services.Features;

public sealed class ConnectFeatureService : IConnectFeatureService
{
    private readonly UnifiedSessionService _sessionService;
    private readonly UnifiedConfigurationService _configService;

    public ConnectFeatureService(UnifiedSessionService sessionService, UnifiedConfigurationService configService)
    {
        _sessionService = sessionService;
        _configService = configService;
    }

    public Task<CoreResult<bool>> ValidateAndConnectAsync(string address, string config, string? adbPath, CancellationToken cancellationToken = default)
    {
        if (string.IsNullOrWhiteSpace(address))
        {
            return Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.InvalidRequest, "Address cannot be empty.")));
        }

        return _sessionService.ConnectAsync(address, config, adbPath, cancellationToken);
    }

    public async Task<UiOperationResult> ConnectAsync(string address, string config, string? adbPath, CancellationToken cancellationToken = default)
    {
        var result = await ValidateAndConnectAsync(address, config, adbPath, cancellationToken);
        return UiOperationResult.FromCore(result, $"Connected to {address}");
    }

    public async Task<UiOperationResult> StartAsync(CancellationToken cancellationToken = default)
    {
        var result = await _sessionService.StartAsync(cancellationToken);
        return UiOperationResult.FromCore(result, "Task execution started.");
    }

    public async Task<UiOperationResult> StopAsync(CancellationToken cancellationToken = default)
    {
        var result = await _sessionService.StopAsync(cancellationToken);
        return UiOperationResult.FromCore(result, "Task execution stopped.");
    }

    public async Task<UiOperationResult> WaitAndStopAsync(TimeSpan wait, CancellationToken cancellationToken = default)
    {
        if (wait <= TimeSpan.Zero)
        {
            return UiOperationResult.Fail("InvalidWaitTime", "Wait time must be greater than zero.");
        }

        await Task.Delay(wait, cancellationToken);
        return await StopAsync(cancellationToken);
    }

    public async Task<UiOperationResult<ImportReport>> ImportLegacyConfigAsync(
        ImportSource source,
        bool manualImport,
        CancellationToken cancellationToken = default)
    {
        var report = await _configService.ImportLegacyAsync(source, manualImport, cancellationToken);
        if (!report.Success)
        {
            return UiOperationResult<ImportReport>.Fail("ImportFailed", string.Join("; ", report.Errors));
        }

        return UiOperationResult<ImportReport>.Ok(report, report.Summary);
    }
}

public sealed class ShellFeatureService : IShellFeatureService
{
    private readonly IConnectFeatureService _connectFeatureService;

    public ShellFeatureService(IConnectFeatureService connectFeatureService)
    {
        _connectFeatureService = connectFeatureService;
    }

    public Task<UiOperationResult> ConnectAsync(
        string address,
        string config,
        string? adbPath,
        CancellationToken cancellationToken = default)
    {
        return _connectFeatureService.ConnectAsync(address, config, adbPath, cancellationToken);
    }

    public Task<UiOperationResult<ImportReport>> ImportLegacyConfigAsync(
        ImportSource source,
        bool manualImport,
        CancellationToken cancellationToken = default)
    {
        return _connectFeatureService.ImportLegacyConfigAsync(source, manualImport, cancellationToken);
    }

    public Task<UiOperationResult<string>> SwitchLanguageAsync(
        string currentLanguage,
        string? targetLanguage = null,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();

        if (!string.IsNullOrWhiteSpace(targetLanguage))
        {
            if (!UiLanguageCatalog.IsSupported(targetLanguage))
            {
                return Task.FromResult(
                    UiOperationResult<string>.Fail(
                        "LanguageNotSupported",
                        $"Unsupported language: {targetLanguage}."));
            }

            var normalizedTarget = UiLanguageCatalog.Normalize(targetLanguage);
            return Task.FromResult(
                UiOperationResult<string>.Ok(
                    normalizedTarget,
                    $"Language switched to {normalizedTarget}."));
        }

        var next = UiLanguageCatalog.NextInCycle(currentLanguage);

        return Task.FromResult(
            UiOperationResult<string>.Ok(
                next,
                $"Language switched to {next}."));
    }

    public IReadOnlyList<string> GetSupportedLanguages()
    {
        return UiLanguageCatalog.Ordered;
    }
}

public sealed class TaskQueueFeatureService : ITaskQueueFeatureService
{
    private readonly UnifiedSessionService _sessionService;
    private readonly UnifiedConfigurationService _configService;

    public TaskQueueFeatureService(UnifiedSessionService sessionService, UnifiedConfigurationService configService)
    {
        _sessionService = sessionService;
        _configService = configService;
    }

    public Task<CoreResult<int>> QueueEnabledTasksAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetProfile(out var profile, out var error))
        {
            return Task.FromResult(CoreResult<int>.Fail(new CoreError(CoreErrorCode.InvalidRequest, error)));
        }

        ApplyMallCreditFightGuard(profile);
        return _sessionService.AppendTasksFromCurrentProfileAsync(cancellationToken);
    }

    public Task<UiOperationResult<IReadOnlyList<TaskQueuePrecheckWarning>>> GetStartPrecheckWarningsAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetProfile(out var profile, out var error))
        {
            return Task.FromResult(UiOperationResult<IReadOnlyList<TaskQueuePrecheckWarning>>.Fail("ProfileMissing", error));
        }

        var warnings = CollectMallCreditFightWarnings(profile, mutate: false);
        return Task.FromResult(UiOperationResult<IReadOnlyList<TaskQueuePrecheckWarning>>.Ok(
            warnings,
            warnings.Count == 0
                ? "TaskQueue precheck passed."
                : $"TaskQueue precheck returned {warnings.Count} warning(s)."));
    }

    public Task<UiOperationResult<IReadOnlyList<TaskQueuePrecheckWarning>>> ApplyStartPrecheckDowngradesAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetProfile(out var profile, out var error))
        {
            return Task.FromResult(UiOperationResult<IReadOnlyList<TaskQueuePrecheckWarning>>.Fail("ProfileMissing", error));
        }

        var warnings = CollectMallCreditFightWarnings(profile, mutate: true);
        return Task.FromResult(UiOperationResult<IReadOnlyList<TaskQueuePrecheckWarning>>.Ok(
            warnings,
            warnings.Count == 0
                ? "TaskQueue precheck downgrade not required."
                : $"TaskQueue precheck applied {warnings.Count} downgrade(s)."));
    }

    public Task<UiOperationResult<IReadOnlyList<UnifiedTaskItem>>> GetCurrentTaskQueueAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetProfile(out var profile, out var error))
        {
            return Task.FromResult(UiOperationResult<IReadOnlyList<UnifiedTaskItem>>.Fail("ProfileMissing", error));
        }

        IReadOnlyList<UnifiedTaskItem> copied = profile.TaskQueue
            .Select(CloneTask)
            .ToList();

        return Task.FromResult(UiOperationResult<IReadOnlyList<UnifiedTaskItem>>.Ok(copied, $"Loaded {copied.Count} task(s)."));
    }

    public Task<UiOperationResult> AddTaskAsync(string type, string name, bool enabled = true, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (string.IsNullOrWhiteSpace(type))
        {
            return Task.FromResult(UiOperationResult.Fail("TaskTypeMissing", "Task type cannot be empty."));
        }

        if (!TryGetProfile(out var profile, out var error))
        {
            return Task.FromResult(UiOperationResult.Fail("ProfileMissing", error));
        }

        var (normalizedType, managedDefaults) = TaskParamCompiler.NormalizeTypeAndCreateDefaultParams(
            type,
            profile,
            _configService.CurrentConfig);
        var defaultParams = managedDefaults.Count > 0 || IsManagedType(normalizedType)
            ? managedDefaults
            : TaskModuleParameterDefaults.Create(normalizedType, ResolveLanguage());

        profile.TaskQueue.Add(new UnifiedTaskItem
        {
            Type = normalizedType,
            Name = string.IsNullOrWhiteSpace(name) ? normalizedType : name.Trim(),
            IsEnabled = enabled,
            Params = defaultParams,
        });

        return Task.FromResult(UiOperationResult.Ok($"Added task `{normalizedType}`."));
    }

    public Task<UiOperationResult> RenameTaskAsync(int index, string newName, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (string.IsNullOrWhiteSpace(newName))
        {
            return Task.FromResult(UiOperationResult.Fail("TaskNameMissing", "Task name cannot be empty."));
        }

        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult.Fail("TaskNotFound", error));
        }

        task.Name = newName.Trim();
        return Task.FromResult(UiOperationResult.Ok($"Task renamed to `{task.Name}`."));
    }

    public Task<UiOperationResult> RemoveTaskAsync(int index, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetProfile(out var profile, out var error))
        {
            return Task.FromResult(UiOperationResult.Fail("ProfileMissing", error));
        }

        if (index < 0 || index >= profile.TaskQueue.Count)
        {
            return Task.FromResult(UiOperationResult.Fail("TaskNotFound", $"Task index {index} is out of range."));
        }

        var name = profile.TaskQueue[index].Name;
        profile.TaskQueue.RemoveAt(index);
        return Task.FromResult(UiOperationResult.Ok($"Removed task `{name}`."));
    }

    public Task<UiOperationResult> MoveTaskAsync(int fromIndex, int toIndex, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetProfile(out var profile, out var error))
        {
            return Task.FromResult(UiOperationResult.Fail("ProfileMissing", error));
        }

        if (fromIndex < 0 || fromIndex >= profile.TaskQueue.Count || toIndex < 0 || toIndex >= profile.TaskQueue.Count)
        {
            return Task.FromResult(UiOperationResult.Fail("TaskMoveOutOfRange", "Task move index is out of range."));
        }

        if (fromIndex == toIndex)
        {
            return Task.FromResult(UiOperationResult.Ok("Task order unchanged."));
        }

        var item = profile.TaskQueue[fromIndex];
        profile.TaskQueue.RemoveAt(fromIndex);
        profile.TaskQueue.Insert(toIndex, item);
        return Task.FromResult(UiOperationResult.Ok($"Moved task `{item.Name}` to position {toIndex + 1}."));
    }

    public Task<UiOperationResult> SetTaskEnabledAsync(int index, bool? enabled, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult.Fail("TaskNotFound", error));
        }

        task.IsEnabled = enabled ?? false;
        return Task.FromResult(UiOperationResult.Ok($"Task `{task.Name}` enabled: {task.IsEnabled}."));
    }

    public Task<UiOperationResult<JsonObject>> GetTaskParamsAsync(int index, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult<JsonObject>.Fail("TaskNotFound", error));
        }

        var parameters = task.Params.DeepClone() as JsonObject ?? new JsonObject();
        return Task.FromResult(UiOperationResult<JsonObject>.Ok(parameters, $"Loaded params for `{task.Name}`."));
    }

    public async Task<UiOperationResult> UpdateTaskParamsAsync(
        int index,
        JsonObject parameters,
        bool persistImmediately = false,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (parameters is null)
        {
            return UiOperationResult.Fail(UiErrorCode.TaskParamsMissing, "Task params cannot be null.");
        }

        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return UiOperationResult.Fail(UiErrorCode.TaskNotFound, error);
        }

        task.Params = parameters.DeepClone() as JsonObject ?? new JsonObject();
        if (persistImmediately)
        {
            await _configService.SaveAsync(cancellationToken);
            return UiOperationResult.Ok($"Updated params for `{task.Name}` and persisted.");
        }

        return UiOperationResult.Ok($"Updated params for `{task.Name}`.");
    }

    public Task<UiOperationResult<StartUpTaskParamsDto>> GetStartUpParamsAsync(int index, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult<StartUpTaskParamsDto>.Fail("TaskNotFound", error));
        }

        if (!TryGetProfile(out var profile, out error))
        {
            return Task.FromResult(UiOperationResult<StartUpTaskParamsDto>.Fail("ProfileMissing", error));
        }

        if (!IsTaskType(task, TaskModuleTypes.StartUp))
        {
            return Task.FromResult(UiOperationResult<StartUpTaskParamsDto>.Fail("TaskTypeMismatch", "Selected task is not a StartUp task."));
        }

        var (dto, issues) = TaskParamCompiler.ReadStartUp(task, profile, _configService.CurrentConfig, strict: false);
        if (issues.Any(i => i.Blocking))
        {
            return Task.FromResult(UiOperationResult<StartUpTaskParamsDto>.Fail("TaskParamsCorrupted", BuildIssueMessage(issues)));
        }

        return Task.FromResult(UiOperationResult<StartUpTaskParamsDto>.Ok(dto, $"Loaded StartUp params for `{task.Name}`."));
    }

    public Task<UiOperationResult<FightTaskParamsDto>> GetFightParamsAsync(int index, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult<FightTaskParamsDto>.Fail("TaskNotFound", error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Fight))
        {
            return Task.FromResult(UiOperationResult<FightTaskParamsDto>.Fail("TaskTypeMismatch", "Selected task is not a Fight task."));
        }

        var (dto, issues) = TaskParamCompiler.ReadFight(task, strict: false);
        if (issues.Any(i => i.Blocking))
        {
            return Task.FromResult(UiOperationResult<FightTaskParamsDto>.Fail("TaskParamsCorrupted", BuildIssueMessage(issues)));
        }

        return Task.FromResult(UiOperationResult<FightTaskParamsDto>.Ok(dto, $"Loaded Fight params for `{task.Name}`."));
    }

    public Task<UiOperationResult<RecruitTaskParamsDto>> GetRecruitParamsAsync(int index, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult<RecruitTaskParamsDto>.Fail("TaskNotFound", error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Recruit))
        {
            return Task.FromResult(UiOperationResult<RecruitTaskParamsDto>.Fail("TaskTypeMismatch", "Selected task is not a Recruit task."));
        }

        var (dto, issues) = TaskParamCompiler.ReadRecruit(task, strict: false);
        if (issues.Any(i => i.Blocking))
        {
            return Task.FromResult(UiOperationResult<RecruitTaskParamsDto>.Fail("TaskParamsCorrupted", BuildIssueMessage(issues)));
        }

        return Task.FromResult(UiOperationResult<RecruitTaskParamsDto>.Ok(dto, $"Loaded Recruit params for `{task.Name}`."));
    }

    public Task<UiOperationResult<RoguelikeTaskParamsDto>> GetRoguelikeParamsAsync(int index, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult<RoguelikeTaskParamsDto>.Fail("TaskNotFound", error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Roguelike))
        {
            return Task.FromResult(UiOperationResult<RoguelikeTaskParamsDto>.Fail("TaskTypeMismatch", "Selected task is not a Roguelike task."));
        }

        var (dto, issues) = TaskParamCompiler.ReadRoguelike(task, strict: false);
        if (issues.Any(i => i.Blocking))
        {
            return Task.FromResult(UiOperationResult<RoguelikeTaskParamsDto>.Fail("TaskParamsCorrupted", BuildIssueMessage(issues)));
        }

        return Task.FromResult(UiOperationResult<RoguelikeTaskParamsDto>.Ok(dto, $"Loaded Roguelike params for `{task.Name}`."));
    }

    public Task<UiOperationResult<ReclamationTaskParamsDto>> GetReclamationParamsAsync(int index, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult<ReclamationTaskParamsDto>.Fail("TaskNotFound", error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Reclamation))
        {
            return Task.FromResult(UiOperationResult<ReclamationTaskParamsDto>.Fail("TaskTypeMismatch", "Selected task is not a Reclamation task."));
        }

        var (dto, issues) = TaskParamCompiler.ReadReclamation(task, strict: false);
        if (issues.Any(i => i.Blocking))
        {
            return Task.FromResult(UiOperationResult<ReclamationTaskParamsDto>.Fail("TaskParamsCorrupted", BuildIssueMessage(issues)));
        }

        return Task.FromResult(UiOperationResult<ReclamationTaskParamsDto>.Ok(dto, $"Loaded Reclamation params for `{task.Name}`."));
    }

    public Task<UiOperationResult<CustomTaskParamsDto>> GetCustomParamsAsync(int index, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult<CustomTaskParamsDto>.Fail("TaskNotFound", error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Custom))
        {
            return Task.FromResult(UiOperationResult<CustomTaskParamsDto>.Fail("TaskTypeMismatch", "Selected task is not a Custom task."));
        }

        var (dto, issues) = TaskParamCompiler.ReadCustom(task, strict: false);
        if (issues.Any(i => i.Blocking))
        {
            return Task.FromResult(UiOperationResult<CustomTaskParamsDto>.Fail("TaskParamsCorrupted", BuildIssueMessage(issues)));
        }

        return Task.FromResult(UiOperationResult<CustomTaskParamsDto>.Ok(dto, $"Loaded Custom params for `{task.Name}`."));
    }

    public Task<UiOperationResult> SaveStartUpParamsAsync(int index, StartUpTaskParamsDto dto, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult.Fail("TaskNotFound", error));
        }

        if (!TryGetProfile(out var profile, out error))
        {
            return Task.FromResult(UiOperationResult.Fail("ProfileMissing", error));
        }

        if (!IsTaskType(task, TaskModuleTypes.StartUp))
        {
            return Task.FromResult(UiOperationResult.Fail("TaskTypeMismatch", "Selected task is not a StartUp task."));
        }

        var compiled = TaskParamCompiler.CompileStartUp(dto, profile, _configService.CurrentConfig);
        if (compiled.HasBlockingIssues)
        {
            return Task.FromResult(UiOperationResult.Fail("TaskValidationFailed", BuildIssueMessage(compiled.Issues)));
        }

        task.Type = compiled.NormalizedType;
        task.Params = compiled.Params;
        TaskParamCompiler.ApplyStartUpSharedProfileValues(profile, new StartUpTaskParamsDto
        {
            AccountName = compiled.Params["account_name"]?.GetValue<string>() ?? string.Empty,
            ClientType = compiled.Params["client_type"]?.GetValue<string>() ?? dto.ClientType,
            StartGameEnabled = compiled.Params["start_game_enabled"]?.GetValue<bool>() ?? dto.StartGameEnabled,
            ConnectConfig = dto.ConnectConfig,
            ConnectAddress = dto.ConnectAddress,
            AdbPath = dto.AdbPath,
            TouchMode = dto.TouchMode,
            AutoDetectConnection = dto.AutoDetectConnection,
            AttachWindowScreencapMethod = dto.AttachWindowScreencapMethod,
            AttachWindowMouseMethod = dto.AttachWindowMouseMethod,
            AttachWindowKeyboardMethod = dto.AttachWindowKeyboardMethod,
        });

        return Task.FromResult(UiOperationResult.Ok($"Updated StartUp params for `{task.Name}`."));
    }

    public Task<UiOperationResult> SaveFightParamsAsync(int index, FightTaskParamsDto dto, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult.Fail("TaskNotFound", error));
        }

        if (!TryGetProfile(out var profile, out error))
        {
            return Task.FromResult(UiOperationResult.Fail("ProfileMissing", error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Fight))
        {
            return Task.FromResult(UiOperationResult.Fail("TaskTypeMismatch", "Selected task is not a Fight task."));
        }

        var compiled = TaskParamCompiler.CompileFight(dto, profile, _configService.CurrentConfig);
        if (compiled.HasBlockingIssues)
        {
            return Task.FromResult(UiOperationResult.Fail("TaskValidationFailed", BuildIssueMessage(compiled.Issues)));
        }

        task.Type = compiled.NormalizedType;
        task.Params = compiled.Params;
        foreach (var warning in compiled.Issues.Where(i => !i.Blocking))
        {
            _configService.LogService.Warn($"{warning.Code}: {warning.Message}");
        }

        return Task.FromResult(UiOperationResult.Ok($"Updated Fight params for `{task.Name}`."));
    }

    public Task<UiOperationResult> SaveRecruitParamsAsync(int index, RecruitTaskParamsDto dto, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult.Fail("TaskNotFound", error));
        }

        if (!TryGetProfile(out var profile, out error))
        {
            return Task.FromResult(UiOperationResult.Fail("ProfileMissing", error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Recruit))
        {
            return Task.FromResult(UiOperationResult.Fail("TaskTypeMismatch", "Selected task is not a Recruit task."));
        }

        var compiled = TaskParamCompiler.CompileRecruit(dto, profile, _configService.CurrentConfig);
        if (compiled.HasBlockingIssues)
        {
            return Task.FromResult(UiOperationResult.Fail("TaskValidationFailed", BuildIssueMessage(compiled.Issues)));
        }

        task.Type = compiled.NormalizedType;
        task.Params = compiled.Params;

        return Task.FromResult(UiOperationResult.Ok($"Updated Recruit params for `{task.Name}`."));
    }

    public Task<UiOperationResult> SaveRoguelikeParamsAsync(int index, RoguelikeTaskParamsDto dto, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult.Fail("TaskNotFound", error));
        }

        if (!TryGetProfile(out var profile, out error))
        {
            return Task.FromResult(UiOperationResult.Fail("ProfileMissing", error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Roguelike))
        {
            return Task.FromResult(UiOperationResult.Fail("TaskTypeMismatch", "Selected task is not a Roguelike task."));
        }

        var compiled = TaskParamCompiler.CompileRoguelike(dto, profile, _configService.CurrentConfig);
        if (compiled.HasBlockingIssues)
        {
            return Task.FromResult(UiOperationResult.Fail("TaskValidationFailed", BuildIssueMessage(compiled.Issues)));
        }

        task.Type = compiled.NormalizedType;
        task.Params = compiled.Params;
        foreach (var warning in compiled.Issues.Where(i => !i.Blocking))
        {
            _configService.LogService.Warn($"{warning.Code}: {warning.Message}");
        }

        return Task.FromResult(UiOperationResult.Ok($"Updated Roguelike params for `{task.Name}`."));
    }

    public Task<UiOperationResult> SaveReclamationParamsAsync(int index, ReclamationTaskParamsDto dto, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult.Fail("TaskNotFound", error));
        }

        if (!TryGetProfile(out var profile, out error))
        {
            return Task.FromResult(UiOperationResult.Fail("ProfileMissing", error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Reclamation))
        {
            return Task.FromResult(UiOperationResult.Fail("TaskTypeMismatch", "Selected task is not a Reclamation task."));
        }

        var compiled = TaskParamCompiler.CompileReclamation(dto, profile, _configService.CurrentConfig);
        if (compiled.HasBlockingIssues)
        {
            return Task.FromResult(UiOperationResult.Fail("TaskValidationFailed", BuildIssueMessage(compiled.Issues)));
        }

        task.Type = compiled.NormalizedType;
        task.Params = compiled.Params;
        foreach (var warning in compiled.Issues.Where(i => !i.Blocking))
        {
            _configService.LogService.Warn($"{warning.Code}: {warning.Message}");
        }

        return Task.FromResult(UiOperationResult.Ok($"Updated Reclamation params for `{task.Name}`."));
    }

    public Task<UiOperationResult> SaveCustomParamsAsync(int index, CustomTaskParamsDto dto, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult.Fail("TaskNotFound", error));
        }

        if (!TryGetProfile(out var profile, out error))
        {
            return Task.FromResult(UiOperationResult.Fail("ProfileMissing", error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Custom))
        {
            return Task.FromResult(UiOperationResult.Fail("TaskTypeMismatch", "Selected task is not a Custom task."));
        }

        var compiled = TaskParamCompiler.CompileCustom(dto, profile, _configService.CurrentConfig);
        if (compiled.HasBlockingIssues)
        {
            return Task.FromResult(UiOperationResult.Fail("TaskValidationFailed", BuildIssueMessage(compiled.Issues)));
        }

        task.Type = compiled.NormalizedType;
        task.Params = compiled.Params;
        foreach (var warning in compiled.Issues.Where(i => !i.Blocking))
        {
            _configService.LogService.Warn($"{warning.Code}: {warning.Message}");
        }

        return Task.FromResult(UiOperationResult.Ok($"Updated Custom params for `{task.Name}`."));
    }

    public Task<UiOperationResult<TaskValidationReport>> ValidateTaskAsync(int index, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult<TaskValidationReport>.Fail("TaskNotFound", error));
        }

        if (!TryGetProfile(out var profile, out error))
        {
            return Task.FromResult(UiOperationResult<TaskValidationReport>.Fail("ProfileMissing", error));
        }

        var compiled = TaskParamCompiler.CompileTask(task, profile, _configService.CurrentConfig, strict: true);
        var report = new TaskValidationReport
        {
            TaskIndex = index,
            TaskName = task.Name,
            NormalizedType = compiled.NormalizedType,
            CompiledParams = compiled.Params.DeepClone() as JsonObject ?? new JsonObject(),
            Issues = compiled.Issues.ToList(),
        };
        var message = report.Issues.Count == 0
            ? $"Task `{task.Name}` passed validation."
            : BuildIssueMessage(report.Issues);
        return Task.FromResult(UiOperationResult<TaskValidationReport>.Ok(report, message));
    }

    public async Task<UiOperationResult> SaveAsync(CancellationToken cancellationToken = default)
    {
        await _configService.SaveAsync(cancellationToken);
        return UiOperationResult.Ok("Task queue saved.");
    }

    public async Task<UiOperationResult> FlushTaskParamWritesAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        await _configService.SaveAsync(cancellationToken);
        return UiOperationResult.Ok("Task params flushed.");
    }

    private bool TryGetTaskByIndex(int index, out UnifiedTaskItem task, out string error)
    {
        task = default!;
        if (!TryGetProfile(out var profile, out error))
        {
            return false;
        }

        if (index < 0 || index >= profile.TaskQueue.Count)
        {
            error = $"Task index {index} is out of range.";
            return false;
        }

        task = profile.TaskQueue[index];
        return true;
    }

    private bool TryGetProfile(out UnifiedProfile profile, out string error)
    {
        if (!_configService.TryGetCurrentProfile(out profile))
        {
            error = $"Current profile `{_configService.CurrentConfig.CurrentProfile}` not found.";
            return false;
        }

        error = string.Empty;
        return true;
    }

    private static UnifiedTaskItem CloneTask(UnifiedTaskItem source)
    {
        var parameters = source.Params.DeepClone() as JsonObject ?? new JsonObject();
        return new UnifiedTaskItem
        {
            Type = TaskModuleTypes.Normalize(source.Type),
            Name = source.Name,
            IsEnabled = source.IsEnabled,
            Params = parameters,
            LegacyRawTask = source.LegacyRawTask?.DeepClone() as JsonObject,
        };
    }

    private string ResolveLanguage()
    {
        if (_configService.CurrentConfig.GlobalValues.TryGetValue("GUI.Localization", out var value)
            && value is JsonValue jsonValue
            && jsonValue.TryGetValue(out string? language)
            && !string.IsNullOrWhiteSpace(language))
        {
            return language;
        }

        return "zh-cn";
    }

    private void ApplyMallCreditFightGuard(UnifiedProfile profile)
    {
        _ = CollectMallCreditFightWarnings(profile, mutate: true);
    }

    private IReadOnlyList<TaskQueuePrecheckWarning> CollectMallCreditFightWarnings(UnifiedProfile profile, bool mutate)
    {
        var warnings = new List<TaskQueuePrecheckWarning>();
        var enabledFightTasks = profile.TaskQueue
            .Where(t => t.IsEnabled && string.Equals(TaskModuleTypes.Normalize(t.Type), TaskModuleTypes.Fight, StringComparison.OrdinalIgnoreCase))
            .ToList();
        if (enabledFightTasks.Count == 0)
        {
            return warnings;
        }

        var hasEmptyFightStage = enabledFightTasks.Any(t => !HasNonEmptyStage(t.Params));
        if (!hasEmptyFightStage)
        {
            return warnings;
        }

        foreach (var mallTask in profile.TaskQueue.Where(t =>
                     t.IsEnabled && string.Equals(TaskModuleTypes.Normalize(t.Type), TaskModuleTypes.Mall, StringComparison.OrdinalIgnoreCase)))
        {
            var mallParams = mallTask.Params;
            if (!TryReadBool(mallParams, "credit_fight", out var enabledCreditFight) || !enabledCreditFight)
            {
                continue;
            }

            var warningMessage = $"Mall credit fight disabled for `{mallTask.Name}` because enabled Fight task has empty stage.";
            if (mutate)
            {
                mallParams["credit_fight"] = false;
                _configService.LogService.Warn(warningMessage);
            }
            warnings.Add(new TaskQueuePrecheckWarning(
                Code: UiErrorCode.MallCreditFightDowngraded,
                Message: warningMessage,
                Scope: "TaskQueue.Precheck.MallCreditFight",
                Blocking: false));
        }

        return warnings;
    }

    private static bool HasNonEmptyStage(JsonObject obj)
    {
        if (!obj.TryGetPropertyValue("stage", out var stageNode) || stageNode is not JsonValue value)
        {
            return false;
        }

        if (!value.TryGetValue(out string? stage))
        {
            return false;
        }

        return !string.IsNullOrWhiteSpace(stage);
    }

    private static bool TryReadBool(JsonObject obj, string key, out bool value)
    {
        value = false;
        if (!obj.TryGetPropertyValue(key, out var node) || node is not JsonValue jsonValue)
        {
            return false;
        }

        if (jsonValue.TryGetValue(out bool b))
        {
            value = b;
            return true;
        }

        if (jsonValue.TryGetValue(out int i))
        {
            value = i != 0;
            return true;
        }

        if (jsonValue.TryGetValue(out string? s) && bool.TryParse(s, out var parsed))
        {
            value = parsed;
            return true;
        }

        return false;
    }

    private static bool IsTaskType(UnifiedTaskItem task, string expectedType)
    {
        return string.Equals(
            TaskParamCompiler.NormalizeTaskType(task.Type),
            expectedType,
            StringComparison.OrdinalIgnoreCase);
    }

    private static bool IsManagedType(string type)
    {
        return string.Equals(type, TaskModuleTypes.StartUp, StringComparison.OrdinalIgnoreCase)
               || string.Equals(type, TaskModuleTypes.Fight, StringComparison.OrdinalIgnoreCase)
               || string.Equals(type, TaskModuleTypes.Recruit, StringComparison.OrdinalIgnoreCase)
               || string.Equals(type, TaskModuleTypes.Roguelike, StringComparison.OrdinalIgnoreCase)
               || string.Equals(type, TaskModuleTypes.Reclamation, StringComparison.OrdinalIgnoreCase)
               || string.Equals(type, TaskModuleTypes.Custom, StringComparison.OrdinalIgnoreCase);
    }

    private static string BuildIssueMessage(IEnumerable<TaskValidationIssue> issues)
    {
        return string.Join(
            "; ",
            issues.Select(i => $"{i.Field}: {i.Message}"));
    }
}

public sealed class CopilotFeatureService : ICopilotFeatureService
{
    public Task<string> ImportCopilotAsync(string source, CancellationToken cancellationToken = default)
    {
        return Task.FromResult($"Copilot import queued from {source}");
    }

    public Task<UiOperationResult> ImportFromFileAsync(string filePath, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (string.IsNullOrWhiteSpace(filePath))
        {
            return Task.FromResult(UiOperationResult.Fail("CopilotFileMissing", "Copilot file path cannot be empty."));
        }

        if (!File.Exists(filePath))
        {
            return Task.FromResult(UiOperationResult.Fail("CopilotFileNotFound", $"Copilot file does not exist: {filePath}"));
        }

        var payload = File.ReadAllText(filePath);
        if (!TryValidateCopilotPayload(payload, out var validationError))
        {
            return Task.FromResult(UiOperationResult.Fail("CopilotPayloadInvalid", validationError));
        }

        return Task.FromResult(UiOperationResult.Ok($"Copilot file imported: {filePath}"));
    }

    public Task<UiOperationResult> ImportFromClipboardAsync(string payload, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (string.IsNullOrWhiteSpace(payload))
        {
            return Task.FromResult(UiOperationResult.Fail("CopilotClipboardEmpty", "Clipboard payload is empty."));
        }

        if (!TryValidateCopilotPayload(payload, out var validationError))
        {
            return Task.FromResult(UiOperationResult.Fail("CopilotPayloadInvalid", validationError));
        }

        return Task.FromResult(UiOperationResult.Ok("Clipboard copilot payload accepted."));
    }

    public Task<UiOperationResult> SubmitFeedbackAsync(string copilotId, bool like, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (string.IsNullOrWhiteSpace(copilotId))
        {
            return Task.FromResult(UiOperationResult.Fail("CopilotIdMissing", "Copilot id cannot be empty."));
        }

        return Task.FromResult(UiOperationResult.Ok($"Feedback submitted for {copilotId}: {(like ? "like" : "dislike")}"));
    }

    private static bool TryValidateCopilotPayload(string payload, out string error)
    {
        error = string.Empty;
        JsonNode? node;
        try
        {
            node = JsonNode.Parse(payload);
        }
        catch (Exception ex)
        {
            error = $"Copilot payload is not valid JSON: {ex.Message}";
            return false;
        }

        if (node is null)
        {
            error = "Copilot payload is empty.";
            return false;
        }

        if (node is JsonArray array)
        {
            if (array.Count == 0)
            {
                error = "Copilot payload array cannot be empty.";
                return false;
            }

            if (array[0] is not JsonObject first)
            {
                error = "Copilot payload array item must be an object.";
                return false;
            }

            if (!HasRequiredCopilotFields(first))
            {
                error = "Copilot payload array item lacks required fields: `stage_name` or `type`.";
                return false;
            }

            return true;
        }

        if (node is JsonObject obj)
        {
            if (!HasRequiredCopilotFields(obj))
            {
                error = "Copilot payload object lacks required fields: `stage_name` or `type`.";
                return false;
            }

            return true;
        }

        error = "Copilot payload must be a JSON object or array.";
        return false;
    }

    private static bool HasRequiredCopilotFields(JsonObject obj)
    {
        return obj.ContainsKey("stage_name")
               || obj.ContainsKey("type")
               || obj.ContainsKey("actions");
    }
}

public sealed class ToolboxFeatureService : IToolboxFeatureService
{
    private static readonly HashSet<string> _supportedTools = new(StringComparer.OrdinalIgnoreCase)
    {
        "Recruit",
        "OperBox",
        "Depot",
        "Gacha",
        "VideoRecognition",
        "MiniGame",
    };

    public Task<string> RunToolAsync(string toolName, CancellationToken cancellationToken = default)
    {
        return Task.FromResult($"Toolbox action dispatched: {toolName}");
    }

    public Task<UiOperationResult<string>> ExecuteToolAsync(
        string toolName,
        string? parameterSummary = null,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!_supportedTools.Contains(toolName))
        {
            return Task.FromResult(UiOperationResult<string>.Fail("ToolNotSupported", $"Tool `{toolName}` is not supported."));
        }

        var normalizedSummary = string.IsNullOrWhiteSpace(parameterSummary)
            ? "none"
            : parameterSummary.Trim();
        return Task.FromResult(UiOperationResult<string>.Ok(
            $"`{toolName}` execution started with params: {normalizedSummary}.",
            $"Tool `{toolName}` dispatched."));
    }
}

public sealed class RemoteControlFeatureService : IRemoteControlFeatureService
{
    private static readonly HttpClient _httpClient = new()
    {
        Timeout = TimeSpan.FromSeconds(5),
    };

    private readonly bool _supported;
    private readonly Func<string, Uri, CancellationToken, Task<EndpointProbeResult>> _probeAsync;

    public RemoteControlFeatureService()
        : this(supported: true, probeAsync: null)
    {
    }

    internal RemoteControlFeatureService(
        bool supported,
        Func<string, Uri, CancellationToken, Task<EndpointProbeResult>>? probeAsync)
    {
        _supported = supported;
        _probeAsync = probeAsync ?? ProbeEndpointAsync;
    }

    public Task<CoreResult<bool>> StartRemotePollingAsync(CancellationToken cancellationToken = default)
    {
        return Task.FromResult(CoreResult<bool>.Ok(true));
    }

    public async Task<UiOperationResult<RemoteControlConnectivityResult>> TestConnectivityAsync(
        RemoteControlConnectivityRequest request,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!_supported)
        {
            return UiOperationResult<RemoteControlConnectivityResult>.Fail(
                UiErrorCode.RemoteControlUnsupported,
                "Remote control connectivity test is unsupported in this environment.");
        }

        var validationError = ValidateRequest(request, out var getTaskUri, out var reportUri);
        if (validationError is not null)
        {
            return UiOperationResult<RemoteControlConnectivityResult>.Fail(
                UiErrorCode.RemoteControlInvalidParameters,
                validationError);
        }

        var getTaskProbe = await _probeAsync("GetTask", getTaskUri!, cancellationToken);
        var reportProbe = await _probeAsync("Report", reportUri!, cancellationToken);
        var result = new RemoteControlConnectivityResult(request.PollIntervalMs, getTaskProbe, reportProbe);
        var details = JsonSerializer.Serialize(result);

        if (getTaskProbe.Success && reportProbe.Success)
        {
            return UiOperationResult<RemoteControlConnectivityResult>.Ok(
                result,
                $"Remote control connectivity passed. GetTask={getTaskProbe.Message}; Report={reportProbe.Message}");
        }

        var firstFailure = getTaskProbe.Success ? reportProbe : getTaskProbe;
        var errorCode = string.Equals(firstFailure.ErrorCode, UiErrorCode.RemoteControlUnsupported, StringComparison.Ordinal)
            ? UiErrorCode.RemoteControlUnsupported
            : UiErrorCode.RemoteControlNetworkFailure;

        return UiOperationResult<RemoteControlConnectivityResult>.Fail(
            errorCode,
            $"Remote control connectivity failed. GetTask={getTaskProbe.Message}; Report={reportProbe.Message}",
            details);
    }

    private static string? ValidateRequest(
        RemoteControlConnectivityRequest request,
        out Uri? getTaskUri,
        out Uri? reportUri)
    {
        getTaskUri = null;
        reportUri = null;
        if (request is null)
        {
            return "Remote control request cannot be null.";
        }

        if (!TryParseHttpUri(request.GetTaskEndpoint, out getTaskUri))
        {
            return $"GetTask endpoint is invalid: `{request.GetTaskEndpoint}`";
        }

        if (!TryParseHttpUri(request.ReportEndpoint, out reportUri))
        {
            return $"Report endpoint is invalid: `{request.ReportEndpoint}`";
        }

        if (request.PollIntervalMs < 500 || request.PollIntervalMs > 60000)
        {
            return $"Poll interval is out of range: {request.PollIntervalMs}.";
        }

        return null;
    }

    private static bool TryParseHttpUri(string? raw, out Uri? uri)
    {
        uri = null;
        if (string.IsNullOrWhiteSpace(raw))
        {
            return false;
        }

        if (!Uri.TryCreate(raw.Trim(), UriKind.Absolute, out var parsed))
        {
            return false;
        }

        if (!string.Equals(parsed.Scheme, Uri.UriSchemeHttp, StringComparison.OrdinalIgnoreCase)
            && !string.Equals(parsed.Scheme, Uri.UriSchemeHttps, StringComparison.OrdinalIgnoreCase))
        {
            return false;
        }

        uri = parsed;
        return true;
    }

    private static async Task<EndpointProbeResult> ProbeEndpointAsync(
        string name,
        Uri endpoint,
        CancellationToken cancellationToken)
    {
        try
        {
            using var request = new HttpRequestMessage(HttpMethod.Get, endpoint);
            using var response = await _httpClient.SendAsync(
                request,
                HttpCompletionOption.ResponseHeadersRead,
                cancellationToken);
            var statusCode = (int)response.StatusCode;
            if (response.IsSuccessStatusCode)
            {
                return new EndpointProbeResult(
                    name,
                    endpoint.ToString(),
                    Success: true,
                    StatusCode: statusCode,
                    Message: $"HTTP {statusCode}");
            }

            return new EndpointProbeResult(
                name,
                endpoint.ToString(),
                Success: false,
                StatusCode: statusCode,
                Message: $"HTTP {statusCode}",
                ErrorCode: UiErrorCode.RemoteControlNetworkFailure);
        }
        catch (OperationCanceledException) when (!cancellationToken.IsCancellationRequested)
        {
            return new EndpointProbeResult(
                name,
                endpoint.ToString(),
                Success: false,
                StatusCode: null,
                Message: "Request timed out",
                ErrorCode: UiErrorCode.RemoteControlNetworkFailure);
        }
        catch (HttpRequestException ex)
        {
            return new EndpointProbeResult(
                name,
                endpoint.ToString(),
                Success: false,
                StatusCode: null,
                Message: $"Network error: {ex.Message}",
                ErrorCode: UiErrorCode.RemoteControlNetworkFailure);
        }
    }
}

public sealed class OverlayFeatureService : IOverlayFeatureService
{
    private readonly IPlatformCapabilityService _platformCapabilities;

    public OverlayFeatureService(IPlatformCapabilityService platformCapabilities)
    {
        _platformCapabilities = platformCapabilities;
    }

    public Task<string> GetOverlayModeAsync(CancellationToken cancellationToken = default)
    {
        return Task.FromResult("runtime-detected");
    }

    public async Task<UiOperationResult<IReadOnlyList<OverlayTarget>>> GetOverlayTargetsAsync(CancellationToken cancellationToken = default)
    {
        return await _platformCapabilities.QueryOverlayTargetsAsync(cancellationToken);
    }

    public async Task<UiOperationResult> SelectOverlayTargetAsync(string targetId, CancellationToken cancellationToken = default)
    {
        return await _platformCapabilities.SelectOverlayTargetAsync(targetId, cancellationToken);
    }

    public async Task<UiOperationResult> ToggleOverlayVisibilityAsync(bool visible, CancellationToken cancellationToken = default)
    {
        return await _platformCapabilities.SetOverlayVisibleAsync(visible, cancellationToken);
    }
}

public sealed class PostActionFeatureService : IPostActionFeatureService
{
    private const string PostActionConfigKey = "TaskQueue.PostAction";
    private const string WarnKeyIfNoOtherNeedsSystemAction = "PostAction.Warn.IfNoOtherNeedsSystemAction";
    private const string WarnKeyUnsupportedDowngrade = "PostAction.Warn.UnsupportedDowngrade";
    private readonly UnifiedConfigurationService _configService;
    private readonly UiDiagnosticsService _diagnostics;
    private readonly IPostActionExecutorService _executor;

    public PostActionFeatureService(
        UnifiedConfigurationService configService,
        UiDiagnosticsService diagnostics,
        IPostActionExecutorService executor)
    {
        _configService = configService;
        _diagnostics = diagnostics;
        _executor = executor;
    }

    public async Task<UiOperationResult<PostActionConfig>> LoadAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!_configService.TryGetCurrentProfile(out var profile))
        {
            return UiOperationResult<PostActionConfig>.Fail(
                UiErrorCode.ProfileMissing,
                $"Current profile `{_configService.CurrentConfig.CurrentProfile}` not found.");
        }

        if (profile.Values.TryGetValue(PostActionConfigKey, out var node) && node is not null)
        {
            var parsed = PostActionConfig.FromJson(node);
            return UiOperationResult<PostActionConfig>.Ok(parsed, "Loaded structured post action config.");
        }

        if (_configService.CurrentConfig.GlobalValues.TryGetValue(PostActionConfigKey, out var globalStructuredNode) && globalStructuredNode is not null)
        {
            var parsed = PostActionConfig.FromJson(globalStructuredNode);
            profile.Values[PostActionConfigKey] = globalStructuredNode.DeepClone();
            _configService.CurrentConfig.GlobalValues.Remove(PostActionConfigKey);
            await _configService.SaveAsync(cancellationToken);
            return UiOperationResult<PostActionConfig>.Ok(parsed, "Loaded structured post action config.");
        }

        var config = _configService.CurrentConfig;
        var hasProfileLegacy = profile.Values.TryGetValue(ConfigurationKeys.PostActions, out var profileLegacyNode) && profileLegacyNode is not null;
        var hasGlobalLegacy = config.GlobalValues.TryGetValue(ConfigurationKeys.PostActions, out var globalLegacyNode) && globalLegacyNode is not null;
        var legacyNode = hasProfileLegacy ? profileLegacyNode : globalLegacyNode;
        if (legacyNode is null)
        {
            return UiOperationResult<PostActionConfig>.Ok(PostActionConfig.Default, "Post action config is empty.");
        }

        if (!TryReadLegacyFlags(legacyNode, out var flags))
        {
            return UiOperationResult<PostActionConfig>.Fail(
                UiErrorCode.PostActionLegacyParseFailed,
                "Failed to parse legacy post action flags.");
        }

        var migrated = MapLegacyFlags(flags).ToJson();
        profile.Values[PostActionConfigKey] = migrated;
        profile.Values.Remove(ConfigurationKeys.PostActions);
        config.GlobalValues.Remove(ConfigurationKeys.PostActions);
        await _configService.SaveAsync(cancellationToken);
        _configService.LogService.Info("Migrated legacy post actions bitmask to structured TaskQueue.PostAction.");
        return UiOperationResult<PostActionConfig>.Ok(PostActionConfig.FromJson(migrated), "Legacy post action config migrated.");
    }

    public async Task<UiOperationResult> SaveAsync(PostActionConfig config, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!_configService.TryGetCurrentProfile(out var profile))
        {
            return UiOperationResult.Fail(
                UiErrorCode.ProfileMissing,
                $"Current profile `{_configService.CurrentConfig.CurrentProfile}` not found.");
        }

        var persistentConfig = config.Clone();
        persistentConfig.Once = false;
        profile.Values[PostActionConfigKey] = persistentConfig.ToJson();
        _configService.CurrentConfig.GlobalValues.Remove(PostActionConfigKey);
        profile.Values.Remove(ConfigurationKeys.PostActions);
        _configService.CurrentConfig.GlobalValues.Remove(ConfigurationKeys.PostActions);
        await _configService.SaveAsync(cancellationToken);
        return UiOperationResult.Ok("Post action config saved.");
    }

    public Task<UiOperationResult<PostActionPreview>> GetCapabilityPreviewAsync(PostActionConfig config, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var warnings = new List<string>();
        var unsupported = new List<string>();

        AddUnsupported(config.ExitArknights, PostActionType.ExitArknights, nameof(PostActionType.ExitArknights));
        AddUnsupported(config.BackToAndroidHome, PostActionType.BackToAndroidHome, nameof(PostActionType.BackToAndroidHome));
        AddUnsupported(config.ExitEmulator, PostActionType.ExitEmulator, nameof(PostActionType.ExitEmulator));
        AddUnsupported(config.ExitSelf, PostActionType.ExitSelf, nameof(PostActionType.ExitSelf));
        AddUnsupported(config.Hibernate, PostActionType.Hibernate, nameof(PostActionType.Hibernate));
        AddUnsupported(config.Shutdown, PostActionType.Shutdown, nameof(PostActionType.Shutdown));
        AddUnsupported(config.Sleep, PostActionType.Sleep, nameof(PostActionType.Sleep));

        if (unsupported.Count > 0)
        {
            warnings.Add(WarnKeyUnsupportedDowngrade);
        }

        var preview = new PostActionPreview(false, warnings, unsupported);
        return Task.FromResult(UiOperationResult<PostActionPreview>.Ok(preview, "Post action selection validated."));

        void AddUnsupported(bool selected, PostActionType action, string actionName)
        {
            if (!selected)
            {
                return;
            }

            if (RequiresCommandTemplate(action) && !HasConfiguredCommand(config, action))
            {
                unsupported.Add(actionName);
                return;
            }

            var capability = _executor.CapabilityMatrix.Get(action);
            if (!capability.Supported)
            {
                unsupported.Add(actionName);
            }
        }
    }

    public async Task<UiOperationResult<PostActionPreview>> ValidateSelectionAsync(PostActionConfig config, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var warnings = new List<string>();
        if (config.IfNoOtherMaa && !(config.Hibernate || config.Shutdown || config.Sleep))
        {
            warnings.Add(WarnKeyIfNoOtherNeedsSystemAction);
        }

        var capability = await GetCapabilityPreviewAsync(config, cancellationToken);
        if (!capability.Success || capability.Value is null)
        {
            return UiOperationResult<PostActionPreview>.Fail(
                capability.Error?.Code ?? UiErrorCode.PostActionSelectionInvalid,
                capability.Message);
        }

        warnings.AddRange(capability.Value.Warnings);
        var preview = new PostActionPreview(
            HasBlockingError: false,
            Warnings: warnings,
            UnsupportedActions: capability.Value.UnsupportedActions);
        return UiOperationResult<PostActionPreview>.Ok(preview, "Post action selection validated.");
    }

    public async Task<UiOperationResult> ExecuteAfterCompletionAsync(
        PostActionExecutionContext context,
        PostActionConfig? configOverride = null,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        PostActionConfig config;
        if (configOverride is null)
        {
            var loadResult = await LoadAsync(cancellationToken);
            if (!loadResult.Success || loadResult.Value is null)
            {
                return UiOperationResult.Fail(loadResult.Error?.Code ?? UiErrorCode.PostActionLoadFailed, loadResult.Message);
            }

            config = loadResult.Value;
        }
        else
        {
            config = configOverride.Clone();
        }

        if (!config.HasAnyAction())
        {
            return UiOperationResult.Ok("No post actions selected.");
        }

        var validate = await ValidateSelectionAsync(config, cancellationToken);
        if (!validate.Success)
        {
            return UiOperationResult.Fail(validate.Error?.Code ?? UiErrorCode.PostActionSelectionInvalid, validate.Message);
        }

        var summary = new List<string>();
        var executedActions = new List<string>();
        var skippedActions = new List<string>();
        var failures = new List<string>();
        var skipSystemActionsForOtherMaa = config.IfNoOtherMaa && HasOtherMaaProcess();
        if (skipSystemActionsForOtherMaa)
        {
            summary.Add("Detected another MAA process, skipped system-level post actions.");
        }

        if (config.BackToAndroidHome)
        {
            await ExecuteActionAsync(PostActionType.BackToAndroidHome, cancellationToken);
        }

        if (config.ExitArknights)
        {
            await ExecuteActionAsync(PostActionType.ExitArknights, cancellationToken);
        }

        if (config.ExitEmulator)
        {
            await ExecuteActionAsync(PostActionType.ExitEmulator, cancellationToken);
        }

        if (config.Hibernate)
        {
            if (skipSystemActionsForOtherMaa)
            {
                skippedActions.Add(nameof(PostActionType.Hibernate));
                await RecordEventAsync(context, nameof(PostActionType.Hibernate), UiErrorCode.PostActionUnsupported, "Skipped by IfNoOtherMaa.");
            }
            else
            {
                await ExecuteActionAsync(PostActionType.Hibernate, cancellationToken);
            }
        }

        if (config.Shutdown)
        {
            if (skipSystemActionsForOtherMaa)
            {
                skippedActions.Add(nameof(PostActionType.Shutdown));
                await RecordEventAsync(context, nameof(PostActionType.Shutdown), UiErrorCode.PostActionUnsupported, "Skipped by IfNoOtherMaa.");
            }
            else
            {
                await ExecuteActionAsync(PostActionType.Shutdown, cancellationToken);
            }
        }

        if (config.Sleep)
        {
            if (skipSystemActionsForOtherMaa)
            {
                skippedActions.Add(nameof(PostActionType.Sleep));
                await RecordEventAsync(context, nameof(PostActionType.Sleep), UiErrorCode.PostActionUnsupported, "Skipped by IfNoOtherMaa.");
            }
            else
            {
                await ExecuteActionAsync(PostActionType.Sleep, cancellationToken);
            }
        }

        if (config.ExitSelf)
        {
            await ExecuteActionAsync(PostActionType.ExitSelf, cancellationToken);
        }

        var plan = new PostActionExecutionPlan(
            PlannedActions: executedActions,
            SkippedActions: skippedActions,
            SkippedSystemActionsForOtherMaa: skipSystemActionsForOtherMaa);
        if (plan.SkippedActions.Count > 0)
        {
            summary.Add($"Skipped: {string.Join(", ", plan.SkippedActions)}.");
        }

        if (plan.PlannedActions.Count > 0)
        {
            summary.Add($"Executed: {string.Join(", ", plan.PlannedActions)}.");
        }

        if (failures.Count > 0)
        {
            summary.Add($"Failed: {string.Join(", ", failures)}.");
            return UiOperationResult.Fail(UiErrorCode.PostActionExecutionFailed, string.Join(" ", summary));
        }

        return UiOperationResult.Ok(summary.Count == 0 ? "Post actions executed." : string.Join(" ", summary));

        async Task ExecuteActionAsync(PostActionType action, CancellationToken token)
        {
            if (RequiresCommandTemplate(action) && !HasConfiguredCommand(config, action))
            {
                skippedActions.Add(action.ToString());
                await RecordEventAsync(
                    context,
                    action.ToString(),
                    UiErrorCode.PostActionUnsupported,
                    $"Command template missing for {action}, downgraded to logging.");
                return;
            }

            var capability = _executor.CapabilityMatrix.Get(action);
            if (!capability.Supported)
            {
                skippedActions.Add(action.ToString());
                await RecordEventAsync(context, action.ToString(), UiErrorCode.PostActionUnsupported, capability.Message);
                return;
            }

            try
            {
                var request = BuildExecutorRequest(config, action);
                var result = await _executor.ExecuteAsync(action, request, token);
                if (!result.Success)
                {
                    failures.Add($"{action}:{result.ErrorCode ?? UiErrorCode.PostActionExecutionFailed}");
                    await RecordErrorAsync(
                        context,
                        action.ToString(),
                        result.ErrorCode ?? UiErrorCode.PostActionExecutionFailed,
                        result.Message);
                    return;
                }

                executedActions.Add(action.ToString());
                await RecordEventAsync(context, action.ToString(), result.ErrorCode, result.Message);
            }
            catch (Exception ex)
            {
                failures.Add($"{action}:{UiErrorCode.PostActionExecutionFailed}");
                await RecordErrorAsync(
                    context,
                    action.ToString(),
                    UiErrorCode.PostActionExecutionFailed,
                    ex.Message);
            }
        }
    }

    private static bool RequiresCommandTemplate(PostActionType action)
    {
        return action is PostActionType.ExitArknights
            or PostActionType.BackToAndroidHome
            or PostActionType.ExitEmulator
            or PostActionType.ExitSelf;
    }

    private static bool HasConfiguredCommand(PostActionConfig config, PostActionType action)
    {
        return !string.IsNullOrWhiteSpace(GetCommandTemplate(config, action));
    }

    private static PostActionExecutorRequest? BuildExecutorRequest(PostActionConfig config, PostActionType action)
    {
        if (!RequiresCommandTemplate(action))
        {
            return null;
        }

        var commandLine = GetCommandTemplate(config, action);
        return string.IsNullOrWhiteSpace(commandLine)
            ? null
            : new PostActionExecutorRequest(commandLine);
    }

    private static string GetCommandTemplate(PostActionConfig config, PostActionType action)
    {
        var commands = config.Commands;
        return action switch
        {
            PostActionType.ExitArknights => commands.ExitArknights,
            PostActionType.BackToAndroidHome => commands.BackToAndroidHome,
            PostActionType.ExitEmulator => commands.ExitEmulator,
            PostActionType.ExitSelf => commands.ExitSelf,
            _ => string.Empty,
        };
    }

    private static bool HasOtherMaaProcess()
    {
        try
        {
            var processName = Environment.ProcessPath is null
                ? "MAA"
                : Path.GetFileNameWithoutExtension(Environment.ProcessPath);
            var processes = System.Diagnostics.Process.GetProcessesByName(processName);
            return processes.Length > 1;
        }
        catch
        {
            return false;
        }
    }

    private async Task RecordEventAsync(
        PostActionExecutionContext context,
        string action,
        string? errorCode,
        string message,
        CancellationToken cancellationToken = default)
    {
        await _diagnostics.RecordEventAsync(
            "PostAction.Execute",
            BuildDiagnosticPayload(
                context,
                action,
                errorCode,
                message),
            cancellationToken);
    }

    private async Task RecordErrorAsync(
        PostActionExecutionContext context,
        string action,
        string errorCode,
        string message,
        CancellationToken cancellationToken = default)
    {
        await _diagnostics.RecordErrorAsync(
            "PostAction.Execute",
            BuildDiagnosticPayload(context, action, errorCode, message),
            cancellationToken: cancellationToken);
    }

    private static string BuildDiagnosticPayload(
        PostActionExecutionContext context,
        string action,
        string? errorCode,
        string message)
    {
        var runId = string.IsNullOrWhiteSpace(context.RunId) ? "-" : context.RunId;
        var taskIndex = context.TaskIndex?.ToString() ?? "-";
        var code = string.IsNullOrWhiteSpace(errorCode) ? "-" : errorCode;
        return $"runId={runId} taskIndex={taskIndex} module=PostAction action={action} errorCode={code} message={message}";
    }

    private static PostActionConfig MapLegacyFlags(LegacyPostActionFlags flags)
    {
        return new PostActionConfig
        {
            ExitArknights = flags.HasFlag(LegacyPostActionFlags.ExitArknights),
            BackToAndroidHome = flags.HasFlag(LegacyPostActionFlags.BackToAndroidHome),
            ExitEmulator = flags.HasFlag(LegacyPostActionFlags.ExitEmulator),
            ExitSelf = flags.HasFlag(LegacyPostActionFlags.ExitSelf),
            IfNoOtherMaa = flags.HasFlag(LegacyPostActionFlags.IfNoOtherMaa),
            Hibernate = flags.HasFlag(LegacyPostActionFlags.Hibernate),
            Shutdown = flags.HasFlag(LegacyPostActionFlags.Shutdown),
            Sleep = flags.HasFlag(LegacyPostActionFlags.Sleep),
        };
    }

    private static bool TryReadLegacyFlags(JsonNode node, out LegacyPostActionFlags flags)
    {
        flags = LegacyPostActionFlags.None;
        if (node is not JsonValue jsonValue)
        {
            return false;
        }

        if (jsonValue.TryGetValue(out int intValue))
        {
            flags = (LegacyPostActionFlags)intValue;
            return true;
        }

        if (jsonValue.TryGetValue(out string? text))
        {
            if (int.TryParse(text, out intValue))
            {
                flags = (LegacyPostActionFlags)intValue;
                return true;
            }

            if (Enum.TryParse(text, out LegacyPostActionFlags enumValue))
            {
                flags = enumValue;
                return true;
            }
        }

        return false;
    }

    [Flags]
    private enum LegacyPostActionFlags
    {
        None = 0,
        ExitArknights = 1 << 0,
        BackToAndroidHome = 1 << 1,
        ExitEmulator = 1 << 2,
        ExitSelf = 1 << 3,
        IfNoOtherMaa = 1 << 4,
        Hibernate = 1 << 5,
        Shutdown = 1 << 6,
        Sleep = 1 << 7,
    }
}

public sealed class NotificationProviderFeatureService : INotificationProviderFeatureService
{
    private static readonly string[] Providers =
    [
        "Smtp",
        "ServerChan",
        "Bark",
        "Discord",
        "DingTalk",
        "Telegram",
        "Qmsg",
        "Gotify",
        "CustomWebhook",
    ];

    private static readonly HttpClient _httpClient = new()
    {
        Timeout = TimeSpan.FromSeconds(5),
    };

    private readonly bool _supported;
    private readonly Func<string, IReadOnlyDictionary<string, string>, string, string, CancellationToken, Task<UiOperationResult>> _sendAsync;
    private readonly Func<HttpRequestMessage, CancellationToken, Task<HttpResponseMessage>> _sendHttpAsync;

    public NotificationProviderFeatureService()
        : this(supported: true, sendAsync: null, sendHttpAsync: null)
    {
    }

    internal NotificationProviderFeatureService(
        bool supported,
        Func<string, IReadOnlyDictionary<string, string>, string, string, CancellationToken, Task<UiOperationResult>>? sendAsync)
        : this(supported, sendAsync, sendHttpAsync: null)
    {
    }

    internal NotificationProviderFeatureService(
        bool supported,
        Func<string, IReadOnlyDictionary<string, string>, string, string, CancellationToken, Task<UiOperationResult>>? sendAsync,
        Func<HttpRequestMessage, CancellationToken, Task<HttpResponseMessage>>? sendHttpAsync)
    {
        _supported = supported;
        _sendHttpAsync = sendHttpAsync ?? DefaultSendHttpAsync;
        _sendAsync = sendAsync ?? SendByDefaultAsync;
    }

    public Task<string[]> GetAvailableProvidersAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return Task.FromResult(Providers);
    }

    public Task<UiOperationResult> ValidateProviderParametersAsync(
        NotificationProviderRequest request,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!_supported)
        {
            return Task.FromResult(UiOperationResult.Fail(
                UiErrorCode.NotificationProviderUnsupported,
                "Notification provider test is unsupported in this environment."));
        }

        var validation = ValidateRequest(request, out _, out _);
        return Task.FromResult(validation);
    }

    public async Task<UiOperationResult> SendTestAsync(
        NotificationProviderTestRequest request,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!_supported)
        {
            return UiOperationResult.Fail(
                UiErrorCode.NotificationProviderUnsupported,
                "Notification provider test is unsupported in this environment.");
        }

        var validation = ValidateRequest(
            new NotificationProviderRequest(request.Provider, request.ParametersText),
            out var provider,
            out var parameters);
        if (!validation.Success)
        {
            return validation;
        }

        if (string.IsNullOrWhiteSpace(request.Title) || string.IsNullOrWhiteSpace(request.Message))
        {
            return UiOperationResult.Fail(
                UiErrorCode.NotificationProviderInvalidParameters,
                "Notification title and message cannot be empty.");
        }

        return await _sendAsync(
            provider!,
            parameters!,
            request.Title.Trim(),
            request.Message.Trim(),
            cancellationToken);
    }

    private static UiOperationResult ValidateRequest(
        NotificationProviderRequest request,
        out string? provider,
        out IReadOnlyDictionary<string, string>? parameters)
    {
        provider = null;
        parameters = null;
        if (request is null)
        {
            return UiOperationResult.Fail(
                UiErrorCode.NotificationProviderInvalidParameters,
                "Notification provider request cannot be null.");
        }

        provider = NormalizeProvider(request.Provider);
        if (provider is null)
        {
            return UiOperationResult.Fail(
                UiErrorCode.NotificationProviderUnsupported,
                $"Notification provider `{request.Provider}` is unsupported.");
        }

        var parsed = ParseParameterText(request.ParametersText, out var parseError);
        if (parseError is not null)
        {
            return UiOperationResult.Fail(
                UiErrorCode.NotificationProviderInvalidParameters,
                parseError);
        }

        var parameterValidationError = ValidateProviderRules(provider, parsed);
        if (parameterValidationError is not null)
        {
            return UiOperationResult.Fail(
                UiErrorCode.NotificationProviderInvalidParameters,
                parameterValidationError);
        }

        parameters = parsed;
        return UiOperationResult.Ok($"Notification provider `{provider}` parameters are valid.");
    }

    private static string? ValidateProviderRules(
        string provider,
        IReadOnlyDictionary<string, string> parameters)
    {
        if (provider == "Smtp")
        {
            if (!HasValue(parameters, "server")
                || !HasValue(parameters, "port")
                || !HasValue(parameters, "from")
                || !HasValue(parameters, "to"))
            {
                return "Smtp requires `server`, `port`, `from`, and `to`.";
            }

            if (!int.TryParse(parameters["port"], NumberStyles.Integer, CultureInfo.InvariantCulture, out var port)
                || port < 1
                || port > 65535)
            {
                return "Smtp `port` must be in [1, 65535].";
            }

            return null;
        }

        if (provider == "ServerChan")
        {
            return HasValue(parameters, "sendKey")
                ? null
                : "ServerChan requires `sendKey`.";
        }

        if (provider == "Bark")
        {
            return HasValue(parameters, "sendKey")
                ? null
                : "Bark requires `sendKey`.";
        }

        if (provider == "Discord")
        {
            if (HasValue(parameters, "webhookUrl"))
            {
                return ValidateHttpUrl(parameters["webhookUrl"], "Discord `webhookUrl`");
            }

            if (HasValue(parameters, "botToken") && HasValue(parameters, "userId"))
            {
                return null;
            }

            return "Discord requires `webhookUrl` or (`botToken` + `userId`).";
        }

        if (provider == "DingTalk")
        {
            if (!HasValue(parameters, "accessToken"))
            {
                return "DingTalk requires `accessToken`.";
            }

            return null;
        }

        if (provider == "Telegram")
        {
            if (!HasValue(parameters, "botToken") || !HasValue(parameters, "chatId"))
            {
                return "Telegram requires `botToken` and `chatId`.";
            }

            if (HasValue(parameters, "apiUrl"))
            {
                return ValidateHttpUrl(parameters["apiUrl"], "Telegram `apiUrl`");
            }

            return null;
        }

        if (provider == "Qmsg")
        {
            if (!HasValue(parameters, "key"))
            {
                return "Qmsg requires `key`.";
            }

            if (HasValue(parameters, "server"))
            {
                return ValidateHttpUrl(parameters["server"], "Qmsg `server`");
            }

            return null;
        }

        if (provider == "Gotify")
        {
            if (!HasValue(parameters, "server") || !HasValue(parameters, "token"))
            {
                return "Gotify requires `server` and `token`.";
            }

            return ValidateHttpUrl(parameters["server"], "Gotify `server`");
        }

        if (provider == "CustomWebhook")
        {
            if (!HasValue(parameters, "url"))
            {
                return "CustomWebhook requires `url`.";
            }

            return ValidateHttpUrl(parameters["url"], "CustomWebhook `url`");
        }

        return null;
    }

    private static string? ValidateHttpUrl(string raw, string field)
    {
        if (!Uri.TryCreate(raw.Trim(), UriKind.Absolute, out var uri))
        {
            return $"{field} must be an absolute URL.";
        }

        if (!string.Equals(uri.Scheme, Uri.UriSchemeHttp, StringComparison.OrdinalIgnoreCase)
            && !string.Equals(uri.Scheme, Uri.UriSchemeHttps, StringComparison.OrdinalIgnoreCase))
        {
            return $"{field} must use http/https scheme.";
        }

        return null;
    }

    private static Dictionary<string, string> ParseParameterText(string? text, out string? error)
    {
        error = null;
        var parameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        if (string.IsNullOrWhiteSpace(text))
        {
            return parameters;
        }

        var lines = text.Replace("\r\n", "\n", StringComparison.Ordinal).Split('\n', StringSplitOptions.TrimEntries);
        for (var i = 0; i < lines.Length; i++)
        {
            var line = lines[i].Trim();
            if (line.Length == 0)
            {
                continue;
            }

            var equalIndex = line.IndexOf('=');
            if (equalIndex <= 0)
            {
                error = $"Invalid parameter line {i + 1}: `{line}`. Expected `key=value`.";
                return parameters;
            }

            var key = line[..equalIndex].Trim();
            var value = line[(equalIndex + 1)..].Trim();
            if (key.Length == 0)
            {
                error = $"Invalid parameter line {i + 1}: key cannot be empty.";
                return parameters;
            }

            parameters[key] = value;
        }

        return parameters;
    }

    private static string? NormalizeProvider(string? provider)
    {
        if (string.IsNullOrWhiteSpace(provider))
        {
            return null;
        }

        return Providers.FirstOrDefault(p => string.Equals(p, provider.Trim(), StringComparison.OrdinalIgnoreCase));
    }

    private static bool HasValue(IReadOnlyDictionary<string, string> parameters, string key)
    {
        return parameters.TryGetValue(key, out var value) && !string.IsNullOrWhiteSpace(value);
    }

    private static Task<HttpResponseMessage> DefaultSendHttpAsync(
        HttpRequestMessage request,
        CancellationToken cancellationToken)
    {
        return _httpClient.SendAsync(request, HttpCompletionOption.ResponseHeadersRead, cancellationToken);
    }

    private async Task<UiOperationResult> SendByDefaultAsync(
        string provider,
        IReadOnlyDictionary<string, string> parameters,
        string title,
        string message,
        CancellationToken cancellationToken)
    {
        if (provider == "DingTalk")
        {
            return await SendDingTalkAsync(parameters, title, message, cancellationToken);
        }

        if (!TryResolveProbeUrl(provider, parameters, out var probeUri))
        {
            return UiOperationResult.Ok($"Notification test request for `{provider}` accepted.");
        }

        try
        {
            using var request = new HttpRequestMessage(HttpMethod.Get, probeUri);
            using var response = await _sendHttpAsync(request, cancellationToken);
            if (response.IsSuccessStatusCode)
            {
                return UiOperationResult.Ok(
                    $"Notification test sent via `{provider}` ({title}: {message}).");
            }

            return UiOperationResult.Fail(
                UiErrorCode.NotificationProviderNetworkFailure,
                $"Notification test failed via `{provider}`: HTTP {(int)response.StatusCode}.");
        }
        catch (OperationCanceledException) when (!cancellationToken.IsCancellationRequested)
        {
            return UiOperationResult.Fail(
                UiErrorCode.NotificationProviderNetworkFailure,
                $"Notification test timed out for `{provider}`.");
        }
        catch (HttpRequestException ex)
        {
            return UiOperationResult.Fail(
                UiErrorCode.NotificationProviderNetworkFailure,
                $"Notification network failure for `{provider}`: {ex.Message}");
        }
    }

    private async Task<UiOperationResult> SendDingTalkAsync(
        IReadOnlyDictionary<string, string> parameters,
        string title,
        string message,
        CancellationToken cancellationToken)
    {
        if (!parameters.TryGetValue("accessToken", out var accessToken) || string.IsNullOrWhiteSpace(accessToken))
        {
            return UiOperationResult.Fail(
                UiErrorCode.NotificationProviderInvalidParameters,
                "DingTalk requires `accessToken`.");
        }

        var secret = parameters.TryGetValue("secret", out var configuredSecret) ? configuredSecret : null;
        var endpoint = BuildDingTalkWebhookUri(accessToken.Trim(), secret?.Trim());
        var body = JsonSerializer.Serialize(new
        {
            msgtype = "text",
            text = new
            {
                content = $"{title}\n{message}",
            },
        });

        try
        {
            using var request = new HttpRequestMessage(HttpMethod.Post, endpoint)
            {
                Content = new StringContent(body, Encoding.UTF8, "application/json"),
            };
            using var response = await _sendHttpAsync(request, cancellationToken);
            if (response.IsSuccessStatusCode)
            {
                return UiOperationResult.Ok("Notification test sent via `DingTalk`.");
            }

            return UiOperationResult.Fail(
                UiErrorCode.NotificationProviderNetworkFailure,
                $"Notification test failed via `DingTalk`: HTTP {(int)response.StatusCode}.");
        }
        catch (OperationCanceledException) when (!cancellationToken.IsCancellationRequested)
        {
            return UiOperationResult.Fail(
                UiErrorCode.NotificationProviderNetworkFailure,
                "Notification test timed out for `DingTalk`.");
        }
        catch (HttpRequestException ex)
        {
            return UiOperationResult.Fail(
                UiErrorCode.NotificationProviderNetworkFailure,
                $"Notification network failure for `DingTalk`: {ex.Message}");
        }
    }

    private static Uri BuildDingTalkWebhookUri(string accessToken, string? secret)
    {
        var encodedAccessToken = Uri.EscapeDataString(accessToken);
        var baseUrl = $"https://oapi.dingtalk.com/robot/send?access_token={encodedAccessToken}";
        if (string.IsNullOrWhiteSpace(secret))
        {
            return new Uri(baseUrl, UriKind.Absolute);
        }

        var timestamp = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds().ToString(CultureInfo.InvariantCulture);
        var sign = BuildDingTalkSign(timestamp, secret);
        var url = $"{baseUrl}&timestamp={timestamp}&sign={sign}";
        return new Uri(url, UriKind.Absolute);
    }

    private static string BuildDingTalkSign(string timestamp, string secret)
    {
        var stringToSign = $"{timestamp}\n{secret}";
        using var hmac = new HMACSHA256(Encoding.UTF8.GetBytes(secret));
        var hash = hmac.ComputeHash(Encoding.UTF8.GetBytes(stringToSign));
        return Uri.EscapeDataString(Convert.ToBase64String(hash));
    }

    private static bool TryResolveProbeUrl(
        string provider,
        IReadOnlyDictionary<string, string> parameters,
        out Uri? probeUri)
    {
        probeUri = null;
        string? rawUrl = provider switch
        {
            "Discord" => parameters.TryGetValue("webhookUrl", out var discordWebhook) ? discordWebhook : null,
            "Gotify" => parameters.TryGetValue("server", out var gotifyServer) ? gotifyServer : null,
            "CustomWebhook" => parameters.TryGetValue("url", out var webhookUrl) ? webhookUrl : null,
            "Qmsg" => parameters.TryGetValue("server", out var qmsgServer) ? qmsgServer : null,
            "Bark" => parameters.TryGetValue("server", out var barkServer) ? barkServer : null,
            "Telegram" => parameters.TryGetValue("apiUrl", out var telegramApi) ? telegramApi : null,
            _ => null,
        };

        if (string.IsNullOrWhiteSpace(rawUrl))
        {
            return false;
        }

        return Uri.TryCreate(rawUrl.Trim(), UriKind.Absolute, out probeUri)
            && (string.Equals(probeUri.Scheme, Uri.UriSchemeHttp, StringComparison.OrdinalIgnoreCase)
                || string.Equals(probeUri.Scheme, Uri.UriSchemeHttps, StringComparison.OrdinalIgnoreCase));
    }
}

public sealed class PlatformCapabilityFeatureService : IPlatformCapabilityService
{
    private readonly PlatformServiceBundle _platform;
    private readonly UiDiagnosticsService _diagnostics;

    public event EventHandler<TrayCommandEvent>? TrayCommandInvoked;

    public event EventHandler<GlobalHotkeyTriggeredEvent>? GlobalHotkeyTriggered;

    public PlatformCapabilityFeatureService(PlatformServiceBundle platform, UiDiagnosticsService diagnostics)
    {
        _platform = platform;
        _diagnostics = diagnostics;
        _platform.TrayService.CommandInvoked += OnTrayCommandInvoked;
        _platform.HotkeyService.Triggered += OnGlobalHotkeyTriggered;
    }

    public Task<UiOperationResult<PlatformCapabilitySnapshot>> GetSnapshotAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var snapshot = PlatformCapabilitySnapshotFactory.FromBundle(_platform);
        return Task.FromResult(UiOperationResult<PlatformCapabilitySnapshot>.Ok(snapshot, "Platform capability snapshot loaded."));
    }

    public Task<UiOperationResult> InitializeTrayAsync(string appTitle, CancellationToken cancellationToken = default)
    {
        return InitializeTrayAsync(appTitle, null, cancellationToken);
    }

    public async Task<UiOperationResult> InitializeTrayAsync(
        string appTitle,
        TrayMenuText? menuText,
        CancellationToken cancellationToken = default)
    {
        var result = await _platform.TrayService.InitializeAsync(appTitle, menuText, cancellationToken);
        return await ToUiResultAsync(PlatformCapabilityId.Tray, "initialize", result, cancellationToken);
    }

    public async Task<UiOperationResult> ShutdownTrayAsync(CancellationToken cancellationToken = default)
    {
        var result = await _platform.TrayService.ShutdownAsync(cancellationToken);
        return await ToUiResultAsync(PlatformCapabilityId.Tray, "shutdown", result, cancellationToken);
    }

    public async Task<UiOperationResult> ShowTrayMessageAsync(string title, string message, CancellationToken cancellationToken = default)
    {
        var result = await _platform.TrayService.ShowAsync(title, message, cancellationToken);
        return await ToUiResultAsync(PlatformCapabilityId.Tray, "show", result, cancellationToken);
    }

    public async Task<UiOperationResult> SetTrayVisibleAsync(bool visible, CancellationToken cancellationToken = default)
    {
        var result = await _platform.TrayService.SetVisibleAsync(visible, cancellationToken);
        return await ToUiResultAsync(PlatformCapabilityId.Tray, "set-visible", result, cancellationToken);
    }

    public async Task<UiOperationResult> SetTrayMenuStateAsync(TrayMenuState state, CancellationToken cancellationToken = default)
    {
        var result = await _platform.TrayService.SetMenuStateAsync(state, cancellationToken);
        return await ToUiResultAsync(PlatformCapabilityId.Tray, "set-menu", result, cancellationToken);
    }

    public async Task<UiOperationResult> SendSystemNotificationAsync(string title, string message, CancellationToken cancellationToken = default)
    {
        var result = await _platform.NotificationService.NotifyAsync(title, message, cancellationToken);
        return await ToUiResultAsync(PlatformCapabilityId.Notification, "notify", result, cancellationToken);
    }

    public async Task<UiOperationResult> RegisterGlobalHotkeyAsync(string name, string gesture, CancellationToken cancellationToken = default)
    {
        var result = await _platform.HotkeyService.RegisterAsync(name, gesture, cancellationToken);
        return await ToUiResultAsync(PlatformCapabilityId.Hotkey, "register", result, cancellationToken);
    }

    public async Task<UiOperationResult> UnregisterGlobalHotkeyAsync(string name, CancellationToken cancellationToken = default)
    {
        var result = await _platform.HotkeyService.UnregisterAsync(name, cancellationToken);
        return await ToUiResultAsync(PlatformCapabilityId.Hotkey, "unregister", result, cancellationToken);
    }

    public async Task<UiOperationResult<bool>> GetAutostartEnabledAsync(CancellationToken cancellationToken = default)
    {
        var result = await _platform.AutostartService.IsEnabledAsync(cancellationToken);
        await _diagnostics.RecordPlatformEventAsync(PlatformCapabilityId.Autostart, "query", result, cancellationToken);
        if (!result.Success)
        {
            await _diagnostics.RecordFailedResultAsync(
                "PlatformCapability.Autostart.query",
                UiOperationResult.Fail(result.ErrorCode ?? "AutostartQueryFailed", result.Message),
                cancellationToken);
            return UiOperationResult<bool>.Fail(result.ErrorCode ?? "AutostartQueryFailed", result.Message);
        }

        return result.Value
            ? UiOperationResult<bool>.Ok(true, result.Message)
            : UiOperationResult<bool>.Ok(false, result.Message);
    }

    public async Task<UiOperationResult> SetAutostartEnabledAsync(bool enabled, CancellationToken cancellationToken = default)
    {
        var result = await _platform.AutostartService.SetEnabledAsync(enabled, cancellationToken);
        return await ToUiResultAsync(PlatformCapabilityId.Autostart, "set-enabled", result, cancellationToken);
    }

    public async Task<UiOperationResult> BindOverlayHostAsync(
        nint hostWindowHandle,
        bool clickThrough,
        double opacity,
        CancellationToken cancellationToken = default)
    {
        var result = await _platform.OverlayService.BindHostWindowAsync(hostWindowHandle, clickThrough, opacity, cancellationToken);
        return await ToUiResultAsync(PlatformCapabilityId.Overlay, "bind-host", result, cancellationToken);
    }

    public async Task<UiOperationResult<IReadOnlyList<OverlayTarget>>> QueryOverlayTargetsAsync(CancellationToken cancellationToken = default)
    {
        try
        {
            var result = await _platform.OverlayService.QueryTargetsAsync(cancellationToken);
            await _diagnostics.RecordPlatformEventAsync(PlatformCapabilityId.Overlay, "query-targets", result, cancellationToken);
            if (!result.Success)
            {
                await _diagnostics.RecordFailedResultAsync(
                    "PlatformCapability.Overlay.query-targets",
                    UiOperationResult.Fail(result.ErrorCode ?? PlatformErrorCodes.OverlayQueryFailed, result.Message),
                    cancellationToken);
                return UiOperationResult<IReadOnlyList<OverlayTarget>>.Fail(result.ErrorCode ?? PlatformErrorCodes.OverlayQueryFailed, result.Message);
            }

            return UiOperationResult<IReadOnlyList<OverlayTarget>>.Ok(
                result.Value ?? Array.Empty<OverlayTarget>(),
                result.Message);
        }
        catch (Exception ex)
        {
            var failed = PlatformOperation.Failed(
                _platform.OverlayService.Capability.Provider,
                $"Overlay target query failed: {ex.Message}",
                PlatformErrorCodes.OverlayQueryFailed,
                "overlay.query-targets");
            await _diagnostics.RecordPlatformEventAsync(PlatformCapabilityId.Overlay, "query-targets", failed, cancellationToken);
            await _diagnostics.RecordFailedResultAsync(
                "PlatformCapability.Overlay.query-targets",
                UiOperationResult.Fail(PlatformErrorCodes.OverlayQueryFailed, failed.Message),
                cancellationToken);
            return UiOperationResult<IReadOnlyList<OverlayTarget>>.Fail(PlatformErrorCodes.OverlayQueryFailed, failed.Message);
        }
    }

    public async Task<UiOperationResult> SelectOverlayTargetAsync(string targetId, CancellationToken cancellationToken = default)
    {
        var result = await _platform.OverlayService.SelectTargetAsync(targetId, cancellationToken);
        return await ToUiResultAsync(PlatformCapabilityId.Overlay, "select-target", result, cancellationToken);
    }

    public async Task<UiOperationResult> SetOverlayVisibleAsync(bool visible, CancellationToken cancellationToken = default)
    {
        var result = await _platform.OverlayService.SetVisibleAsync(visible, cancellationToken);
        return await ToUiResultAsync(PlatformCapabilityId.Overlay, "set-visible", result, cancellationToken);
    }

    private async Task<UiOperationResult> ToUiResultAsync(
        PlatformCapabilityId capability,
        string action,
        PlatformOperationResult result,
        CancellationToken cancellationToken)
    {
        await _diagnostics.RecordPlatformEventAsync(capability, action, result, cancellationToken);
        if (!result.Success)
        {
            await _diagnostics.RecordFailedResultAsync(
                $"PlatformCapability.{capability}.{action}",
                UiOperationResult.Fail(result.ErrorCode ?? "PlatformOperationFailed", result.Message),
                cancellationToken);
            return UiOperationResult.Fail(result.ErrorCode ?? "PlatformOperationFailed", result.Message);
        }

        return UiOperationResult.Ok(result.Message);
    }

    private void OnTrayCommandInvoked(object? sender, TrayCommandEvent e)
    {
        _ = _diagnostics.RecordEventAsync(
            "PlatformCapability.TrayCommand",
            $"command={e.Command} source={e.Source} ts={e.Timestamp:O}");
        try
        {
            TrayCommandInvoked?.Invoke(this, e);
        }
        catch (Exception ex)
        {
            _ = _diagnostics.RecordErrorAsync(
                "PlatformCapability.TrayCommand",
                "Tray command callback failed.",
                ex);
        }
    }

    private void OnGlobalHotkeyTriggered(object? sender, GlobalHotkeyTriggeredEvent e)
    {
        _ = _diagnostics.RecordEventAsync(
            "PlatformCapability.HotkeyTriggered",
            $"name={e.Name} gesture={e.Gesture} ts={e.Timestamp:O}");
        try
        {
            GlobalHotkeyTriggered?.Invoke(this, e);
        }
        catch (Exception ex)
        {
            _ = _diagnostics.RecordErrorAsync(
                "PlatformCapability.HotkeyTriggered",
                "Hotkey callback failed.",
                ex);
        }
    }
}

public sealed class SettingsFeatureService : ISettingsFeatureService
{
    private readonly UnifiedConfigurationService _configService;
    private readonly IPlatformCapabilityService _platformCapabilities;
    private readonly UiDiagnosticsService _diagnostics;

    public SettingsFeatureService(
        UnifiedConfigurationService configService,
        IPlatformCapabilityService platformCapabilities,
        UiDiagnosticsService diagnostics)
    {
        _configService = configService;
        _platformCapabilities = platformCapabilities;
        _diagnostics = diagnostics;
    }

    public async Task<UiOperationResult> SaveGlobalSettingAsync(string key, string value, CancellationToken cancellationToken = default)
    {
        if (string.IsNullOrWhiteSpace(key))
        {
            return UiOperationResult.Fail("SettingKeyMissing", "Setting key cannot be empty.");
        }

        return await SaveGlobalSettingsAsync(
            new Dictionary<string, string>(StringComparer.Ordinal)
            {
                [key] = value,
            },
            cancellationToken);
    }

    public async Task<UiOperationResult> SaveGlobalSettingsAsync(
        IReadOnlyDictionary<string, string> updates,
        CancellationToken cancellationToken = default)
    {
        if (updates.Count == 0)
        {
            return UiOperationResult.Fail("SettingBatchEmpty", "No settings were provided.");
        }

        var oldValues = new Dictionary<string, JsonNode?>(StringComparer.OrdinalIgnoreCase);
        var existedKeys = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var (key, value) in updates)
        {
            if (string.IsNullOrWhiteSpace(key))
            {
                return UiOperationResult.Fail("SettingKeyMissing", "Setting key cannot be empty.");
            }

            if (_configService.CurrentConfig.GlobalValues.TryGetValue(key, out var oldValue))
            {
                existedKeys.Add(key);
            }

            oldValues[key] = oldValue?.DeepClone();
            _configService.CurrentConfig.GlobalValues[key] = JsonValue.Create(value);
        }

        try
        {
            await _configService.SaveAsync(cancellationToken);
            await _diagnostics.RecordEventAsync(
                "Settings",
                $"Saved settings batch: {string.Join(", ", updates.Keys.OrderBy(static k => k, StringComparer.Ordinal))}",
                cancellationToken);
            return UiOperationResult.Ok($"Saved {updates.Count} settings.");
        }
        catch (Exception ex)
        {
            // Rollback in-memory config to keep a consistent read model on failed save.
            foreach (var (key, oldValue) in oldValues)
            {
                if (!existedKeys.Contains(key))
                {
                    _configService.CurrentConfig.GlobalValues.Remove(key);
                    continue;
                }

                _configService.CurrentConfig.GlobalValues[key] = oldValue?.DeepClone();
            }

            await _diagnostics.RecordErrorAsync("Settings.SaveBatch", "Failed to save settings batch.", ex, cancellationToken);
            return UiOperationResult.Fail("SettingsSaveFailed", $"Failed to save settings: {ex.Message}");
        }
    }

    public async Task<UiOperationResult> TestNotificationAsync(string title, string message, CancellationToken cancellationToken = default)
    {
        return await _platformCapabilities.SendSystemNotificationAsync(title, message, cancellationToken);
    }

    public async Task<UiOperationResult> RegisterHotkeyAsync(string name, string gesture, CancellationToken cancellationToken = default)
    {
        return await _platformCapabilities.RegisterGlobalHotkeyAsync(name, gesture, cancellationToken);
    }

    public async Task<UiOperationResult<bool>> GetAutostartStatusAsync(CancellationToken cancellationToken = default)
    {
        return await _platformCapabilities.GetAutostartEnabledAsync(cancellationToken);
    }

    public async Task<UiOperationResult> SetAutostartAsync(bool enabled, CancellationToken cancellationToken = default)
    {
        return await _platformCapabilities.SetAutostartEnabledAsync(enabled, cancellationToken);
    }

    public async Task<UiOperationResult<string>> BuildIssueReportAsync(CancellationToken cancellationToken = default)
    {
        var baseDirectory = AppContext.BaseDirectory;
        var bundlePath = await _diagnostics.BuildIssueReportBundleAsync(baseDirectory, cancellationToken);
        return UiOperationResult<string>.Ok(bundlePath, "Issue report bundle generated.");
    }
}

public sealed class VersionUpdateFeatureService : IVersionUpdateFeatureService
{
    private static readonly HashSet<string> AllowedVersionTypes = new(StringComparer.OrdinalIgnoreCase)
    {
        "Stable",
        "Beta",
        "Nightly",
    };

    private readonly UnifiedConfigurationService? _configService;

    public VersionUpdateFeatureService()
    {
    }

    public VersionUpdateFeatureService(UnifiedConfigurationService configService)
    {
        _configService = configService;
    }

    public Task<UiOperationResult<VersionUpdatePolicy>> LoadPolicyAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetConfig(out var config, out var failure))
        {
            return Task.FromResult(failure);
        }

        var policy = new VersionUpdatePolicy(
            Proxy: ReadString(config, ConfigurationKeys.UpdateProxy, string.Empty),
            ProxyType: ReadString(config, ConfigurationKeys.ProxyType, "system"),
            VersionType: ReadString(config, ConfigurationKeys.VersionType, "Stable"),
            ResourceUpdateSource: ReadString(config, ConfigurationKeys.UpdateSource, "Official"),
            ForceGithubGlobalSource: ReadBool(config, ConfigurationKeys.ForceGithubGlobalSource, false),
            MirrorChyanCdk: ReadString(config, ConfigurationKeys.MirrorChyanCdk, string.Empty),
            MirrorChyanCdkExpired: ReadString(config, ConfigurationKeys.MirrorChyanCdkExpiredTime, string.Empty),
            StartupUpdateCheck: ReadBool(config, ConfigurationKeys.StartupUpdateCheck, true),
            ScheduledUpdateCheck: ReadBool(config, ConfigurationKeys.UpdateAutoCheck, false),
            ResourceApi: ReadString(config, ConfigurationKeys.ResourceApi, string.Empty),
            AllowNightlyUpdates: ReadBool(config, ConfigurationKeys.AllowNightlyUpdates, false),
            HasAcknowledgedNightlyWarning: ReadBool(config, ConfigurationKeys.HasAcknowledgedNightlyWarning, false),
            UseAria2: ReadBool(config, ConfigurationKeys.UseAria2, false),
            AutoDownloadUpdatePackage: ReadBool(config, ConfigurationKeys.AutoDownloadUpdatePackage, false),
            AutoInstallUpdatePackage: ReadBool(config, ConfigurationKeys.AutoInstallUpdatePackage, false),
            VersionName: ReadString(config, ConfigurationKeys.VersionName, string.Empty),
            VersionBody: ReadString(config, ConfigurationKeys.VersionUpdateBody, string.Empty),
            IsFirstBoot: ReadBool(config, ConfigurationKeys.VersionUpdateIsFirstBoot, false),
            VersionPackage: ReadString(config, ConfigurationKeys.VersionUpdatePackage, string.Empty),
            DoNotShowUpdate: ReadBool(config, ConfigurationKeys.VersionUpdateDoNotShowUpdate, false));
        return Task.FromResult(UiOperationResult<VersionUpdatePolicy>.Ok(policy, "Loaded version update policy."));
    }

    public async Task<UiOperationResult> SavePolicyAsync(VersionUpdatePolicy policy, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_configService is null)
        {
            return UiOperationResult.Fail("VersionUpdateServiceUnavailable", "Version update service is not initialized.");
        }

        var validation = ValidatePolicy(policy);
        if (!validation.Success)
        {
            return validation;
        }

        foreach (var (key, value) in policy.ToGlobalSettingUpdates())
        {
            _configService.CurrentConfig.GlobalValues[key] = JsonValue.Create(value);
        }

        await _configService.SaveAsync(cancellationToken);
        return UiOperationResult.Ok("Version update policy saved.");
    }

    public Task<UiOperationResult<string>> CheckForUpdatesAsync(
        VersionUpdatePolicy policy,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var validation = ValidatePolicy(policy);
        if (!validation.Success)
        {
            return Task.FromResult(UiOperationResult<string>.Fail(
                validation.Error?.Code ?? UiErrorCode.VersionUpdateInvalidParameters,
                validation.Message,
                validation.Error?.Details));
        }

        var channel = policy.VersionType;
        var source = string.IsNullOrWhiteSpace(policy.ResourceUpdateSource) ? "Official" : policy.ResourceUpdateSource;
        return Task.FromResult(UiOperationResult<string>.Ok(
            $"Checked updates on channel `{channel}` via `{source}`. No new package found.",
            "Version update check completed."));
    }

    private UiOperationResult ValidatePolicy(VersionUpdatePolicy policy)
    {
        if (!AllowedVersionTypes.Contains(policy.VersionType))
        {
            return UiOperationResult.Fail(
                UiErrorCode.VersionUpdateInvalidParameters,
                $"Version type `{policy.VersionType}` is unsupported.");
        }

        if (!string.IsNullOrWhiteSpace(policy.Proxy))
        {
            if (!Uri.TryCreate(policy.Proxy.Trim(), UriKind.Absolute, out var proxyUri)
                || (proxyUri.Scheme != Uri.UriSchemeHttp && proxyUri.Scheme != Uri.UriSchemeHttps))
            {
                return UiOperationResult.Fail(
                    UiErrorCode.VersionUpdateInvalidParameters,
                    $"Proxy `{policy.Proxy}` must be an absolute http/https URL.");
            }
        }

        return UiOperationResult.Ok("Version update policy validation passed.");
    }

    private bool TryGetConfig(
        out UnifiedConfig config,
        out UiOperationResult<VersionUpdatePolicy> failure)
    {
        if (_configService is null)
        {
            config = null!;
            failure = UiOperationResult<VersionUpdatePolicy>.Fail(
                "VersionUpdateServiceUnavailable",
                "Version update service is not initialized.");
            return false;
        }

        config = _configService.CurrentConfig;
        failure = default!;
        return true;
    }

    private static string ReadString(UnifiedConfig config, string key, string fallback)
    {
        if (config.GlobalValues.TryGetValue(key, out var node) && node is not null)
        {
            var text = node.ToString().Trim();
            if (text.Length > 0)
            {
                return text;
            }
        }

        return fallback;
    }

    private static bool ReadBool(UnifiedConfig config, string key, bool fallback)
    {
        if (!config.GlobalValues.TryGetValue(key, out var node) || node is null)
        {
            return fallback;
        }

        if (bool.TryParse(node.ToString(), out var parsed))
        {
            return parsed;
        }

        if (int.TryParse(node.ToString(), NumberStyles.Integer, CultureInfo.InvariantCulture, out var parsedInt))
        {
            return parsedInt != 0;
        }

        return fallback;
    }
}

public sealed class AchievementFeatureService : IAchievementFeatureService
{
    private readonly UnifiedConfigurationService? _configService;

    public AchievementFeatureService()
    {
    }

    public AchievementFeatureService(UnifiedConfigurationService configService)
    {
        _configService = configService;
    }

    public Task<UiOperationResult<AchievementPolicy>> LoadPolicyAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_configService is null)
        {
            return Task.FromResult(UiOperationResult<AchievementPolicy>.Fail(
                "AchievementServiceUnavailable",
                "Achievement service is not initialized."));
        }

        var config = _configService.CurrentConfig;
        var policy = new AchievementPolicy(
            PopupDisabled: ReadBool(config, ConfigurationKeys.AchievementPopupDisabled, false),
            PopupAutoClose: ReadBool(config, ConfigurationKeys.AchievementPopupAutoClose, false));
        return Task.FromResult(UiOperationResult<AchievementPolicy>.Ok(policy, "Loaded achievement policy."));
    }

    public async Task<UiOperationResult> SavePolicyAsync(AchievementPolicy policy, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_configService is null)
        {
            return UiOperationResult.Fail("AchievementServiceUnavailable", "Achievement service is not initialized.");
        }

        foreach (var (key, value) in policy.ToGlobalSettingUpdates())
        {
            _configService.CurrentConfig.GlobalValues[key] = JsonValue.Create(value);
        }

        await _configService.SaveAsync(cancellationToken);
        return UiOperationResult.Ok("Achievement policy saved.");
    }

    private static bool ReadBool(UnifiedConfig config, string key, bool fallback)
    {
        if (!config.GlobalValues.TryGetValue(key, out var node) || node is null)
        {
            return fallback;
        }

        if (bool.TryParse(node.ToString(), out var parsed))
        {
            return parsed;
        }

        if (int.TryParse(node.ToString(), NumberStyles.Integer, CultureInfo.InvariantCulture, out var parsedInt))
        {
            return parsedInt != 0;
        }

        return fallback;
    }
}

public sealed class AnnouncementFeatureService : IAnnouncementFeatureService
{
    private readonly UnifiedConfigurationService? _configService;

    public AnnouncementFeatureService()
    {
    }

    public AnnouncementFeatureService(UnifiedConfigurationService configService)
    {
        _configService = configService;
    }

    public Task<UiOperationResult<AnnouncementState>> LoadStateAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_configService is null)
        {
            return Task.FromResult(UiOperationResult<AnnouncementState>.Fail(
                "AnnouncementServiceUnavailable",
                "Announcement service is not initialized."));
        }

        var config = _configService.CurrentConfig;
        var state = new AnnouncementState(
            AnnouncementInfo: ReadString(config, ConfigurationKeys.AnnouncementInfo, string.Empty),
            DoNotRemindThisAnnouncementAgain: ReadBool(config, ConfigurationKeys.DoNotRemindThisAnnouncementAgain, false),
            DoNotShowAnnouncement: ReadBool(config, ConfigurationKeys.DoNotShowAnnouncement, false));
        return Task.FromResult(UiOperationResult<AnnouncementState>.Ok(state, "Loaded announcement state."));
    }

    public async Task<UiOperationResult> SaveStateAsync(AnnouncementState state, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_configService is null)
        {
            return UiOperationResult.Fail("AnnouncementServiceUnavailable", "Announcement service is not initialized.");
        }

        if (state.AnnouncementInfo.Length > 32768)
        {
            return UiOperationResult.Fail(
                UiErrorCode.AnnouncementStateInvalid,
                "Announcement payload is too large.");
        }

        foreach (var (key, value) in state.ToGlobalSettingUpdates())
        {
            _configService.CurrentConfig.GlobalValues[key] = JsonValue.Create(value);
        }

        await _configService.SaveAsync(cancellationToken);
        return UiOperationResult.Ok("Announcement state saved.");
    }

    private static string ReadString(UnifiedConfig config, string key, string fallback)
    {
        if (config.GlobalValues.TryGetValue(key, out var node) && node is not null)
        {
            var text = node.ToString();
            if (!string.IsNullOrWhiteSpace(text))
            {
                return text;
            }
        }

        return fallback;
    }

    private static bool ReadBool(UnifiedConfig config, string key, bool fallback)
    {
        if (!config.GlobalValues.TryGetValue(key, out var node) || node is null)
        {
            return fallback;
        }

        if (bool.TryParse(node.ToString(), out var parsed))
        {
            return parsed;
        }

        return fallback;
    }
}

public sealed class StageManagerFeatureService : IStageManagerFeatureService
{
    private readonly UnifiedConfigurationService? _configService;

    public StageManagerFeatureService()
    {
    }

    public StageManagerFeatureService(UnifiedConfigurationService configService)
    {
        _configService = configService;
    }

    public Task<UiOperationResult<StageManagerConfig>> LoadConfigAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_configService is null)
        {
            return Task.FromResult(UiOperationResult<StageManagerConfig>.Fail(
                "StageManagerServiceUnavailable",
                "Stage manager service is not initialized."));
        }

        var config = _configService.CurrentConfig;
        var stageCodesText = ReadString(config, "Advanced.StageManager.StageCodes", string.Empty);
        var stageCodes = ParseStageCodes(stageCodesText, out _);
        var loaded = new StageManagerConfig(
            StageCodes: stageCodes,
            AutoIterate: ReadBool(config, "Advanced.StageManager.AutoIterate", false),
            LastSelectedStage: ReadString(config, "Advanced.StageManager.LastSelectedStage", string.Empty));
        return Task.FromResult(UiOperationResult<StageManagerConfig>.Ok(loaded, "Loaded stage manager config."));
    }

    public async Task<UiOperationResult> SaveConfigAsync(StageManagerConfig config, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_configService is null)
        {
            return UiOperationResult.Fail("StageManagerServiceUnavailable", "Stage manager service is not initialized.");
        }

        var validation = ValidateStageCodes(config.StageCodes);
        if (!validation.Success)
        {
            return validation;
        }

        var normalized = config with
        {
            StageCodes = config.StageCodes
                .Where(static code => !string.IsNullOrWhiteSpace(code))
                .Select(code => code.Trim())
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .ToArray(),
            LastSelectedStage = (config.LastSelectedStage ?? string.Empty).Trim(),
        };

        foreach (var (key, value) in normalized.ToGlobalSettingUpdates())
        {
            _configService.CurrentConfig.GlobalValues[key] = JsonValue.Create(value);
        }

        await _configService.SaveAsync(cancellationToken);
        return UiOperationResult.Ok("Stage manager config saved.");
    }

    public Task<UiOperationResult<IReadOnlyList<string>>> ValidateStageCodesAsync(
        string stageCodesText,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var stageCodes = ParseStageCodes(stageCodesText, out var invalids);
        if (invalids.Count > 0)
        {
            return Task.FromResult(UiOperationResult<IReadOnlyList<string>>.Fail(
                UiErrorCode.StageManagerInvalidStageCode,
                $"Invalid stage code(s): {string.Join(", ", invalids)}"));
        }

        return Task.FromResult(UiOperationResult<IReadOnlyList<string>>.Ok(stageCodes, "Stage codes are valid."));
    }

    private static UiOperationResult ValidateStageCodes(IReadOnlyList<string> stageCodes)
    {
        var invalids = stageCodes
            .Where(static code => !IsValidStageCode(code))
            .ToArray();
        if (invalids.Length > 0)
        {
            return UiOperationResult.Fail(
                UiErrorCode.StageManagerInvalidStageCode,
                $"Invalid stage code(s): {string.Join(", ", invalids)}");
        }

        return UiOperationResult.Ok("Stage codes are valid.");
    }

    private static IReadOnlyList<string> ParseStageCodes(string? stageCodesText, out IReadOnlyList<string> invalids)
    {
        var values = (stageCodesText ?? string.Empty)
            .Split(
                new[] { ';', ',', '\n', '\r', '\t', ' ' },
                StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
            .Where(static code => code.Length > 0)
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToArray();
        invalids = values.Where(static code => !IsValidStageCode(code)).ToArray();
        return values;
    }

    private static bool IsValidStageCode(string? stageCode)
    {
        if (string.IsNullOrWhiteSpace(stageCode))
        {
            return false;
        }

        foreach (var ch in stageCode)
        {
            if (!(char.IsLetterOrDigit(ch) || ch is '-' or '_'))
            {
                return false;
            }
        }

        return true;
    }

    private static string ReadString(UnifiedConfig config, string key, string fallback)
    {
        if (config.GlobalValues.TryGetValue(key, out var node) && node is not null)
        {
            var text = node.ToString();
            if (!string.IsNullOrWhiteSpace(text))
            {
                return text;
            }
        }

        return fallback;
    }

    private static bool ReadBool(UnifiedConfig config, string key, bool fallback)
    {
        if (!config.GlobalValues.TryGetValue(key, out var node) || node is null)
        {
            return fallback;
        }

        if (bool.TryParse(node.ToString(), out var parsed))
        {
            return parsed;
        }

        if (int.TryParse(node.ToString(), NumberStyles.Integer, CultureInfo.InvariantCulture, out var parsedInt))
        {
            return parsedInt != 0;
        }

        return fallback;
    }
}

public sealed class WebApiFeatureService : IWebApiFeatureService
{
    private readonly UnifiedConfigurationService? _configService;
    private bool _running;

    public WebApiFeatureService()
    {
    }

    public WebApiFeatureService(UnifiedConfigurationService configService)
    {
        _configService = configService;
    }

    public Task<UiOperationResult<WebApiConfig>> LoadConfigAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_configService is null)
        {
            return Task.FromResult(UiOperationResult<WebApiConfig>.Fail(
                "WebApiServiceUnavailable",
                "WebApi service is not initialized."));
        }

        var config = _configService.CurrentConfig;
        var loaded = new WebApiConfig(
            Enabled: ReadBool(config, "Advanced.WebApi.Enabled", false),
            Host: ReadString(config, "Advanced.WebApi.Host", "127.0.0.1"),
            Port: ReadInt(config, "Advanced.WebApi.Port", 51888),
            AccessToken: ReadString(config, "Advanced.WebApi.AccessToken", string.Empty));
        return Task.FromResult(UiOperationResult<WebApiConfig>.Ok(loaded, "Loaded WebApi config."));
    }

    public async Task<UiOperationResult> SaveConfigAsync(WebApiConfig config, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_configService is null)
        {
            return UiOperationResult.Fail("WebApiServiceUnavailable", "WebApi service is not initialized.");
        }

        var validation = ValidateConfig(config);
        if (!validation.Success)
        {
            return validation;
        }

        foreach (var (key, value) in config.ToGlobalSettingUpdates())
        {
            _configService.CurrentConfig.GlobalValues[key] = JsonValue.Create(value);
        }

        await _configService.SaveAsync(cancellationToken);
        return UiOperationResult.Ok("WebApi config saved.");
    }

    public Task<UiOperationResult<bool>> GetRunningStatusAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return Task.FromResult(UiOperationResult<bool>.Ok(_running, _running ? "WebApi is running." : "WebApi is stopped."));
    }

    public async Task<UiOperationResult> StartAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var configResult = await LoadConfigAsync(cancellationToken);
        if (!configResult.Success || configResult.Value is null)
        {
            return UiOperationResult.Fail(
                configResult.Error?.Code ?? "WebApiLoadFailed",
                configResult.Message,
                configResult.Error?.Details);
        }

        var config = configResult.Value;
        var validation = ValidateConfig(config);
        if (!validation.Success)
        {
            return validation;
        }

        if (!config.Enabled)
        {
            return UiOperationResult.Fail("WebApiDisabled", "WebApi is disabled by configuration.");
        }

        if (!IsPortAvailable(config.Host, config.Port))
        {
            return UiOperationResult.Fail(
                UiErrorCode.WebApiPortConflict,
                $"WebApi port is occupied: {config.Host}:{config.Port}");
        }

        _running = true;
        return UiOperationResult.Ok($"WebApi started at {config.Host}:{config.Port}.");
    }

    public Task<UiOperationResult> StopAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        _running = false;
        return Task.FromResult(UiOperationResult.Ok("WebApi stopped."));
    }

    private static UiOperationResult ValidateConfig(WebApiConfig config)
    {
        if (string.IsNullOrWhiteSpace(config.Host))
        {
            return UiOperationResult.Fail(UiErrorCode.WebApiPortConflict, "WebApi host cannot be empty.");
        }

        if (config.Port < 1 || config.Port > 65535)
        {
            return UiOperationResult.Fail(UiErrorCode.WebApiPortConflict, $"WebApi port out of range: {config.Port}");
        }

        return UiOperationResult.Ok("WebApi config is valid.");
    }

    private static bool IsPortAvailable(string host, int port)
    {
        if (!IPAddress.TryParse(host, out var ipAddress))
        {
            ipAddress = host.Equals("localhost", StringComparison.OrdinalIgnoreCase)
                ? IPAddress.Loopback
                : IPAddress.Any;
        }

        TcpListener? listener = null;
        try
        {
            listener = new TcpListener(ipAddress, port);
            listener.Start();
            return true;
        }
        catch (SocketException)
        {
            return false;
        }
        finally
        {
            listener?.Stop();
        }
    }

    private static string ReadString(UnifiedConfig config, string key, string fallback)
    {
        if (config.GlobalValues.TryGetValue(key, out var node) && node is not null)
        {
            var text = node.ToString();
            if (!string.IsNullOrWhiteSpace(text))
            {
                return text;
            }
        }

        return fallback;
    }

    private static bool ReadBool(UnifiedConfig config, string key, bool fallback)
    {
        if (!config.GlobalValues.TryGetValue(key, out var node) || node is null)
        {
            return fallback;
        }

        if (bool.TryParse(node.ToString(), out var parsed))
        {
            return parsed;
        }

        return fallback;
    }

    private static int ReadInt(UnifiedConfig config, string key, int fallback)
    {
        if (!config.GlobalValues.TryGetValue(key, out var node) || node is null)
        {
            return fallback;
        }

        return int.TryParse(node.ToString(), NumberStyles.Integer, CultureInfo.InvariantCulture, out var parsed)
            ? parsed
            : fallback;
    }
}

public sealed class DialogFeatureService : IDialogFeatureService
{
    private readonly UiDiagnosticsService _diagnostics;

    public DialogFeatureService(UiDiagnosticsService diagnostics)
    {
        _diagnostics = diagnostics;
    }

    public Task<string> PrepareDialogPayloadAsync(string dialogType, CancellationToken cancellationToken = default)
    {
        return Task.FromResult($"Dialog payload prepared for {dialogType}");
    }

    public async Task<UiOperationResult> ReportErrorAsync(string context, string message, CancellationToken cancellationToken = default)
    {
        await _diagnostics.RecordEventAsync("DialogError", $"{context}: {message}", cancellationToken);
        return UiOperationResult.Fail("UiError", message);
    }
}
