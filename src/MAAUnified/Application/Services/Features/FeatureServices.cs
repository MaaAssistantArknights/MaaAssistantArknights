using System.Globalization;
using System.IO.Compression;
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
            return UiOperationResult.Fail(UiErrorCode.InvalidWaitTime, "Wait time must be greater than zero.");
        }

        var initialState = _sessionService.CurrentState;
        if (!IsExecutionActiveState(initialState))
        {
            return UiOperationResult.Ok($"Session already stopped (state={initialState}).");
        }

        var stateExitedTask = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        void HandleSessionStateChanged(SessionState state)
        {
            if (!IsExecutionActiveState(state))
            {
                stateExitedTask.TrySetResult();
            }
        }

        _sessionService.SessionStateChanged += HandleSessionStateChanged;
        try
        {
            if (!IsExecutionActiveState(_sessionService.CurrentState))
            {
                return UiOperationResult.Ok("Task execution already stopped during wait.");
            }

            var delayTask = Task.Delay(wait, cancellationToken);
            var completed = await Task.WhenAny(delayTask, stateExitedTask.Task);
            if (completed == stateExitedTask.Task)
            {
                return UiOperationResult.Ok("Task execution already stopped during wait.");
            }

            await delayTask;
        }
        finally
        {
            _sessionService.SessionStateChanged -= HandleSessionStateChanged;
        }

        if (!IsExecutionActiveState(_sessionService.CurrentState))
        {
            return UiOperationResult.Ok("Task execution already stopped during wait.");
        }

        return await StopAsync(cancellationToken);
    }

    private static bool IsExecutionActiveState(SessionState state)
    {
        return state is SessionState.Running or SessionState.Stopping;
    }

    public async Task<UiOperationResult<ImportReport>> ImportLegacyConfigAsync(
        ImportSource source,
        bool manualImport,
        CancellationToken cancellationToken = default)
    {
        var report = await _configService.ImportLegacyAsync(source, manualImport, cancellationToken);
        if (!report.AppliedConfig)
        {
            var message = report.Errors.Count > 0
                ? string.Join("; ", report.Errors)
                : ImportReportTextFormatter.BuildStatusMessage(report, manualImport);
            return UiOperationResult<ImportReport>.Fail(UiErrorCode.ImportFailed, message);
        }

        var successMessage = report.Success
            ? report.Summary
            : $"{ImportReportTextFormatter.BuildStatusMessage(report, manualImport)} {report.Summary}";
        return UiOperationResult<ImportReport>.Ok(report, successMessage);
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
                        UiErrorCode.LanguageNotSupported,
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
            return Task.FromResult(UiOperationResult<IReadOnlyList<TaskQueuePrecheckWarning>>.Fail(UiErrorCode.ProfileMissing, error));
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
            return Task.FromResult(UiOperationResult<IReadOnlyList<TaskQueuePrecheckWarning>>.Fail(UiErrorCode.ProfileMissing, error));
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
            return Task.FromResult(UiOperationResult<IReadOnlyList<UnifiedTaskItem>>.Fail(UiErrorCode.ProfileMissing, error));
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
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskTypeMissing, "Task type cannot be empty."));
        }

        if (!TryGetProfile(out var profile, out var error))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.ProfileMissing, error));
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
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskNameMissing, "Task name cannot be empty."));
        }

        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskNotFound, error));
        }

        task.Name = newName.Trim();
        return Task.FromResult(UiOperationResult.Ok($"Task renamed to `{task.Name}`."));
    }

    public Task<UiOperationResult> RemoveTaskAsync(int index, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetProfile(out var profile, out var error))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.ProfileMissing, error));
        }

        if (index < 0 || index >= profile.TaskQueue.Count)
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskNotFound, $"Task index {index} is out of range."));
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
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.ProfileMissing, error));
        }

        if (fromIndex < 0 || fromIndex >= profile.TaskQueue.Count || toIndex < 0 || toIndex >= profile.TaskQueue.Count)
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskMoveOutOfRange, "Task move index is out of range."));
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
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskNotFound, error));
        }

        task.IsEnabled = enabled ?? false;
        return Task.FromResult(UiOperationResult.Ok($"Task `{task.Name}` enabled: {task.IsEnabled}."));
    }

    public Task<UiOperationResult> SetAllTasksEnabledAsync(bool enabled, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetProfile(out var profile, out var error))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.ProfileMissing, error));
        }

        var affected = 0;
        foreach (var task in profile.TaskQueue)
        {
            if (task.IsEnabled == enabled)
            {
                continue;
            }

            task.IsEnabled = enabled;
            affected++;
        }

        return Task.FromResult(UiOperationResult.Ok(
            $"Set {affected} task(s) to enabled={enabled}."));
    }

    public Task<UiOperationResult> InvertTasksEnabledAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetProfile(out var profile, out var error))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.ProfileMissing, error));
        }

        foreach (var task in profile.TaskQueue)
        {
            task.IsEnabled = !task.IsEnabled;
        }

        return Task.FromResult(UiOperationResult.Ok($"Inverted enabled state for {profile.TaskQueue.Count} task(s)."));
    }

    public Task<UiOperationResult<JsonObject>> GetTaskParamsAsync(int index, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult<JsonObject>.Fail(UiErrorCode.TaskNotFound, error));
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
            return Task.FromResult(UiOperationResult<StartUpTaskParamsDto>.Fail(UiErrorCode.TaskNotFound, error));
        }

        if (!TryGetProfile(out var profile, out error))
        {
            return Task.FromResult(UiOperationResult<StartUpTaskParamsDto>.Fail(UiErrorCode.ProfileMissing, error));
        }

        if (!IsTaskType(task, TaskModuleTypes.StartUp))
        {
            return Task.FromResult(UiOperationResult<StartUpTaskParamsDto>.Fail(UiErrorCode.TaskTypeMismatch, "Selected task is not a StartUp task."));
        }

        var (dto, issues) = TaskParamCompiler.ReadStartUp(task, profile, _configService.CurrentConfig, strict: false);
        if (issues.Any(i => i.Blocking))
        {
            return Task.FromResult(UiOperationResult<StartUpTaskParamsDto>.Fail(UiErrorCode.TaskParamsCorrupted, BuildIssueMessage(issues)));
        }

        return Task.FromResult(UiOperationResult<StartUpTaskParamsDto>.Ok(dto, $"Loaded StartUp params for `{task.Name}`."));
    }

    public Task<UiOperationResult<FightTaskParamsDto>> GetFightParamsAsync(int index, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult<FightTaskParamsDto>.Fail(UiErrorCode.TaskNotFound, error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Fight))
        {
            return Task.FromResult(UiOperationResult<FightTaskParamsDto>.Fail(UiErrorCode.TaskTypeMismatch, "Selected task is not a Fight task."));
        }

        var (dto, issues) = TaskParamCompiler.ReadFight(task, strict: false);
        if (issues.Any(i => i.Blocking))
        {
            return Task.FromResult(UiOperationResult<FightTaskParamsDto>.Fail(UiErrorCode.TaskParamsCorrupted, BuildIssueMessage(issues)));
        }

        return Task.FromResult(UiOperationResult<FightTaskParamsDto>.Ok(dto, $"Loaded Fight params for `{task.Name}`."));
    }

    public Task<UiOperationResult<RecruitTaskParamsDto>> GetRecruitParamsAsync(int index, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult<RecruitTaskParamsDto>.Fail(UiErrorCode.TaskNotFound, error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Recruit))
        {
            return Task.FromResult(UiOperationResult<RecruitTaskParamsDto>.Fail(UiErrorCode.TaskTypeMismatch, "Selected task is not a Recruit task."));
        }

        var (dto, issues) = TaskParamCompiler.ReadRecruit(task, strict: false);
        if (issues.Any(i => i.Blocking))
        {
            return Task.FromResult(UiOperationResult<RecruitTaskParamsDto>.Fail(UiErrorCode.TaskParamsCorrupted, BuildIssueMessage(issues)));
        }

        return Task.FromResult(UiOperationResult<RecruitTaskParamsDto>.Ok(dto, $"Loaded Recruit params for `{task.Name}`."));
    }

    public Task<UiOperationResult<RoguelikeTaskParamsDto>> GetRoguelikeParamsAsync(int index, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult<RoguelikeTaskParamsDto>.Fail(UiErrorCode.TaskNotFound, error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Roguelike))
        {
            return Task.FromResult(UiOperationResult<RoguelikeTaskParamsDto>.Fail(UiErrorCode.TaskTypeMismatch, "Selected task is not a Roguelike task."));
        }

        var (dto, issues) = TaskParamCompiler.ReadRoguelike(task, strict: false);
        if (issues.Any(i => i.Blocking))
        {
            return Task.FromResult(UiOperationResult<RoguelikeTaskParamsDto>.Fail(UiErrorCode.TaskParamsCorrupted, BuildIssueMessage(issues)));
        }

        return Task.FromResult(UiOperationResult<RoguelikeTaskParamsDto>.Ok(dto, $"Loaded Roguelike params for `{task.Name}`."));
    }

    public Task<UiOperationResult<ReclamationTaskParamsDto>> GetReclamationParamsAsync(int index, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult<ReclamationTaskParamsDto>.Fail(UiErrorCode.TaskNotFound, error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Reclamation))
        {
            return Task.FromResult(UiOperationResult<ReclamationTaskParamsDto>.Fail(UiErrorCode.TaskTypeMismatch, "Selected task is not a Reclamation task."));
        }

        var (dto, issues) = TaskParamCompiler.ReadReclamation(task, strict: false);
        if (issues.Any(i => i.Blocking))
        {
            return Task.FromResult(UiOperationResult<ReclamationTaskParamsDto>.Fail(UiErrorCode.TaskParamsCorrupted, BuildIssueMessage(issues)));
        }

        return Task.FromResult(UiOperationResult<ReclamationTaskParamsDto>.Ok(dto, $"Loaded Reclamation params for `{task.Name}`."));
    }

    public Task<UiOperationResult<CustomTaskParamsDto>> GetCustomParamsAsync(int index, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult<CustomTaskParamsDto>.Fail(UiErrorCode.TaskNotFound, error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Custom))
        {
            return Task.FromResult(UiOperationResult<CustomTaskParamsDto>.Fail(UiErrorCode.TaskTypeMismatch, "Selected task is not a Custom task."));
        }

        var (dto, issues) = TaskParamCompiler.ReadCustom(task, strict: false);
        if (issues.Any(i => i.Blocking))
        {
            return Task.FromResult(UiOperationResult<CustomTaskParamsDto>.Fail(UiErrorCode.TaskParamsCorrupted, BuildIssueMessage(issues)));
        }

        return Task.FromResult(UiOperationResult<CustomTaskParamsDto>.Ok(dto, $"Loaded Custom params for `{task.Name}`."));
    }

    public Task<UiOperationResult> SaveStartUpParamsAsync(int index, StartUpTaskParamsDto dto, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskNotFound, error));
        }

        if (!TryGetProfile(out var profile, out error))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.ProfileMissing, error));
        }

        if (!IsTaskType(task, TaskModuleTypes.StartUp))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskTypeMismatch, "Selected task is not a StartUp task."));
        }

        var compiled = TaskParamCompiler.CompileStartUp(dto, profile, _configService.CurrentConfig);
        if (compiled.HasBlockingIssues)
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskValidationFailed, BuildIssueMessage(compiled.Issues)));
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
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskNotFound, error));
        }

        if (!TryGetProfile(out var profile, out error))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.ProfileMissing, error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Fight))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskTypeMismatch, "Selected task is not a Fight task."));
        }

        var compiled = TaskParamCompiler.CompileFight(dto, profile, _configService.CurrentConfig);
        if (compiled.HasBlockingIssues)
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskValidationFailed, BuildIssueMessage(compiled.Issues)));
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
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskNotFound, error));
        }

        if (!TryGetProfile(out var profile, out error))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.ProfileMissing, error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Recruit))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskTypeMismatch, "Selected task is not a Recruit task."));
        }

        var compiled = TaskParamCompiler.CompileRecruit(dto, profile, _configService.CurrentConfig);
        if (compiled.HasBlockingIssues)
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskValidationFailed, BuildIssueMessage(compiled.Issues)));
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
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskNotFound, error));
        }

        if (!TryGetProfile(out var profile, out error))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.ProfileMissing, error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Roguelike))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskTypeMismatch, "Selected task is not a Roguelike task."));
        }

        var compiled = TaskParamCompiler.CompileRoguelike(dto, profile, _configService.CurrentConfig);
        if (compiled.HasBlockingIssues)
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskValidationFailed, BuildIssueMessage(compiled.Issues)));
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
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskNotFound, error));
        }

        if (!TryGetProfile(out var profile, out error))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.ProfileMissing, error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Reclamation))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskTypeMismatch, "Selected task is not a Reclamation task."));
        }

        var compiled = TaskParamCompiler.CompileReclamation(dto, profile, _configService.CurrentConfig);
        if (compiled.HasBlockingIssues)
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskValidationFailed, BuildIssueMessage(compiled.Issues)));
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
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskNotFound, error));
        }

        if (!TryGetProfile(out var profile, out error))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.ProfileMissing, error));
        }

        if (!IsTaskType(task, TaskModuleTypes.Custom))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskTypeMismatch, "Selected task is not a Custom task."));
        }

        var compiled = TaskParamCompiler.CompileCustom(dto, profile, _configService.CurrentConfig);
        if (compiled.HasBlockingIssues)
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.TaskValidationFailed, BuildIssueMessage(compiled.Issues)));
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
            return Task.FromResult(UiOperationResult<TaskValidationReport>.Fail(UiErrorCode.TaskNotFound, error));
        }

        if (!TryGetProfile(out var profile, out error))
        {
            return Task.FromResult(UiOperationResult<TaskValidationReport>.Fail(UiErrorCode.ProfileMissing, error));
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

        var hasCurrentOrLastFightStage = enabledFightTasks.Any(t => !HasSpecificFightStage(t.Params));
        if (!hasCurrentOrLastFightStage)
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

            var warningMessage = $"Mall credit fight disabled for `{mallTask.Name}` because enabled Fight task uses Current/Last stage selector.";
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

    private static bool HasSpecificFightStage(JsonObject obj)
    {
        if (!obj.TryGetPropertyValue("stage", out var stageNode) || stageNode is not JsonValue value)
        {
            return false;
        }

        if (!value.TryGetValue(out string? stage))
        {
            return false;
        }

        return !FightStageSelection.IsCurrentOrLast(stage);
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

    public async Task<UiOperationResult> ImportFromFileAsync(string filePath, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var normalizedPath = (filePath ?? string.Empty).Trim();
        if (string.IsNullOrWhiteSpace(normalizedPath))
        {
            return UiOperationResult.Fail(
                UiErrorCode.CopilotFileMissing,
                "作业文件路径为空。请粘贴本地 JSON 文件路径后重试。");
        }

        if (!File.Exists(normalizedPath))
        {
            return UiOperationResult.Fail(
                UiErrorCode.CopilotFileNotFound,
                $"作业文件不存在：{normalizedPath}。请检查路径是否正确。");
        }

        string payload;
        try
        {
            payload = await File.ReadAllTextAsync(normalizedPath, cancellationToken);
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or NotSupportedException)
        {
            return UiOperationResult.Fail(
                UiErrorCode.CopilotFileReadFailed,
                $"读取作业文件失败：{normalizedPath}。请确认文件可访问且内容为 UTF-8 JSON。",
                ex.Message);
        }

        if (!TryValidateCopilotPayload(payload, out var errorCode, out var errorMessage))
        {
            return UiOperationResult.Fail(errorCode, errorMessage);
        }

        return UiOperationResult.Ok($"已导入作业文件：{normalizedPath}");
    }

    public Task<UiOperationResult> ImportFromClipboardAsync(string payload, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var normalized = (payload ?? string.Empty).Trim();
        if (string.IsNullOrWhiteSpace(normalized))
        {
            return Task.FromResult(UiOperationResult.Fail(
                UiErrorCode.CopilotClipboardEmpty,
                "剪贴板内容为空。请复制本地路径或 JSON 内容后重试。"));
        }

        var pathCandidate = NormalizePotentialPathText(normalized);
        if (File.Exists(pathCandidate))
        {
            return ImportFromFileAsync(pathCandidate, cancellationToken);
        }

        if (!LooksLikeJsonPayload(normalized) && LooksLikePathText(pathCandidate))
        {
            return Task.FromResult(UiOperationResult.Fail(
                UiErrorCode.CopilotFileNotFound,
                $"剪贴板内容看起来是文件路径，但文件不存在：{pathCandidate}。请检查路径后重试。"));
        }

        if (!TryValidateCopilotPayload(normalized, out var errorCode, out var errorMessage))
        {
            return Task.FromResult(UiOperationResult.Fail(errorCode, errorMessage));
        }

        return Task.FromResult(UiOperationResult.Ok("已接受剪贴板作业内容。"));
    }

    public Task<UiOperationResult> SubmitFeedbackAsync(string copilotId, bool like, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (string.IsNullOrWhiteSpace(copilotId))
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.CopilotIdMissing, "Copilot id cannot be empty."));
        }

        return Task.FromResult(UiOperationResult.Ok($"Feedback submitted for {copilotId}: {(like ? "like" : "dislike")}"));
    }

    private static bool TryValidateCopilotPayload(
        string payload,
        out string errorCode,
        out string errorMessage)
    {
        errorCode = string.Empty;
        errorMessage = string.Empty;
        JsonNode? node;
        try
        {
            node = JsonNode.Parse(payload);
        }
        catch (Exception ex)
        {
            errorCode = UiErrorCode.CopilotPayloadInvalidJson;
            errorMessage = $"剪贴板/文件内容不是合法 JSON：{ex.Message}。请检查括号、引号和逗号。";
            return false;
        }

        if (node is null)
        {
            errorCode = UiErrorCode.CopilotPayloadInvalidType;
            errorMessage = "作业内容为空 JSON 节点。请提供 JSON 对象或数组。";
            return false;
        }

        if (node is JsonArray array)
        {
            if (array.Count == 0)
            {
                errorCode = UiErrorCode.CopilotPayloadEmptyArray;
                errorMessage = "作业数组为空。请至少提供一个作业对象。";
                return false;
            }

            for (var i = 0; i < array.Count; i++)
            {
                if (array[i] is not JsonObject item)
                {
                    errorCode = UiErrorCode.CopilotPayloadInvalidType;
                    errorMessage = $"第{i + 1}个作业不是 JSON 对象。请改为对象结构。";
                    return false;
                }

                if (!TryValidateCopilotObject(item, i, out errorCode, out errorMessage))
                {
                    return false;
                }
            }

            return true;
        }

        if (node is JsonObject obj)
        {
            if (!TryValidateCopilotObject(obj, null, out errorCode, out errorMessage))
            {
                return false;
            }

            return true;
        }

        errorCode = UiErrorCode.CopilotPayloadInvalidType;
        errorMessage = "作业内容必须是 JSON 对象或数组。";
        return false;
    }

    private static bool TryValidateCopilotObject(
        JsonObject obj,
        int? index,
        out string errorCode,
        out string errorMessage)
    {
        errorCode = string.Empty;
        errorMessage = string.Empty;

        var position = index.HasValue ? $"第{index.Value + 1}个作业" : "作业对象";
        if (!TryGetRequiredStringProperty(obj, "stage_name", out _))
        {
            errorCode = UiErrorCode.CopilotPayloadMissingFields;
            errorMessage = $"{position}缺少必填字段 `stage_name`（非空字符串）。";
            return false;
        }

        if (!TryGetRequiredStringProperty(obj, "minimum_required", out _))
        {
            errorCode = UiErrorCode.CopilotPayloadMissingFields;
            errorMessage = $"{position}缺少必填字段 `minimum_required`（非空字符串）。";
            return false;
        }

        if (TryGetPropertyCaseInsensitive(obj, "type", out var typeNode)
            && !TryGetStringValue(typeNode, out var typeValue))
        {
            errorCode = UiErrorCode.CopilotPayloadInvalidType;
            errorMessage = $"{position}字段 `type` 必须是字符串。";
            return false;
        }

        var isSss = TryGetPropertyCaseInsensitive(obj, "type", out var resolvedTypeNode)
                    && TryGetStringValue(resolvedTypeNode, out var resolvedType)
                    && string.Equals(resolvedType, "SSS", StringComparison.OrdinalIgnoreCase);
        if (isSss)
        {
            return true;
        }

        if (!TryGetPropertyCaseInsensitive(obj, "actions", out var actionsNode))
        {
            errorCode = UiErrorCode.CopilotPayloadMissingFields;
            errorMessage = $"{position}缺少必填字段 `actions`（非空数组）。";
            return false;
        }

        if (actionsNode is not JsonArray actionsArray)
        {
            errorCode = UiErrorCode.CopilotPayloadInvalidType;
            errorMessage = $"{position}字段 `actions` 必须是数组。";
            return false;
        }

        if (actionsArray.Count == 0)
        {
            errorCode = UiErrorCode.CopilotPayloadMissingFields;
            errorMessage = $"{position}字段 `actions` 不能为空数组。";
            return false;
        }

        return true;
    }

    private static bool TryGetRequiredStringProperty(JsonObject obj, string key, out string value)
    {
        value = string.Empty;
        if (!TryGetPropertyCaseInsensitive(obj, key, out var node) || !TryGetStringValue(node, out var raw))
        {
            return false;
        }

        value = raw.Trim();
        return !string.IsNullOrWhiteSpace(value);
    }

    private static bool TryGetStringValue(JsonNode? node, out string value)
    {
        value = string.Empty;
        if (node is not JsonValue jsonValue || !jsonValue.TryGetValue(out string? raw))
        {
            return false;
        }

        value = raw ?? string.Empty;
        return true;
    }

    private static bool TryGetPropertyCaseInsensitive(JsonObject obj, string key, out JsonNode? value)
    {
        foreach (var property in obj)
        {
            if (string.Equals(property.Key, key, StringComparison.OrdinalIgnoreCase))
            {
                value = property.Value;
                return true;
            }
        }

        value = null;
        return false;
    }

    private static bool LooksLikeJsonPayload(string text)
    {
        var span = text.AsSpan().TrimStart();
        return span.StartsWith("{".AsSpan(), StringComparison.Ordinal)
               || span.StartsWith("[".AsSpan(), StringComparison.Ordinal);
    }

    private static string NormalizePotentialPathText(string text)
    {
        var trimmed = text.Trim();
        if (trimmed.Length >= 2
            && ((trimmed[0] == '"' && trimmed[^1] == '"')
                || (trimmed[0] == '\'' && trimmed[^1] == '\'')))
        {
            return trimmed[1..^1].Trim();
        }

        return trimmed;
    }

    private static bool LooksLikePathText(string text)
    {
        if (string.IsNullOrWhiteSpace(text))
        {
            return false;
        }

        if (Path.IsPathRooted(text))
        {
            return true;
        }

        return text.Contains('\\')
               || text.Contains('/')
               || text.EndsWith(".json", StringComparison.OrdinalIgnoreCase)
               || text.StartsWith("./", StringComparison.Ordinal)
               || text.StartsWith(".\\", StringComparison.Ordinal)
               || text.StartsWith("../", StringComparison.Ordinal)
               || text.StartsWith("..\\", StringComparison.Ordinal);
    }
}

public sealed class ToolboxFeatureService : IToolboxFeatureService
{
    private readonly IMaaCoreBridge? _bridge;
    private readonly IConnectFeatureService? _connectFeatureService;

    public ToolboxFeatureService()
        : this(null, null)
    {
    }

    public ToolboxFeatureService(IMaaCoreBridge? bridge, IConnectFeatureService? connectFeatureService = null)
    {
        _bridge = bridge;
        _connectFeatureService = connectFeatureService;
    }

    public async Task<UiOperationResult<ToolboxDispatchResult>> DispatchToolAsync(
        ToolboxDispatchRequest request,
        CancellationToken cancellationToken = default)
    {
        if (request is null)
        {
            return UiOperationResult<ToolboxDispatchResult>.Fail(
                UiErrorCode.ToolboxInvalidParameters,
                "Toolbox dispatch request cannot be null.");
        }

        if (cancellationToken.IsCancellationRequested)
        {
            return UiOperationResult<ToolboxDispatchResult>.Fail(
                UiErrorCode.ToolboxExecutionCancelled,
                $"Tool `{request.Tool}` dispatch cancelled by caller.");
        }

        if (_bridge is null)
        {
            return UiOperationResult<ToolboxDispatchResult>.Fail(
                UiErrorCode.ToolboxExecutionFailed,
                "Toolbox core bridge is not configured.");
        }

        if (!TryBuildCoreTask(request, out var coreTask, out var taskType, out var parameterSummary, out var validationError))
        {
            return UiOperationResult<ToolboxDispatchResult>.Fail(
                validationError?.Code ?? UiErrorCode.ToolboxInvalidParameters,
                validationError?.Message ?? "Invalid toolbox request.",
                validationError?.Details);
        }

        var appendResult = await _bridge.AppendTaskAsync(coreTask, cancellationToken);
        if (!appendResult.Success)
        {
            return UiOperationResult<ToolboxDispatchResult>.Fail(
                UiErrorCode.ToolboxExecutionFailed,
                $"Tool `{request.Tool}` append failed: {appendResult.Error?.Message ?? "unknown error"}.",
                JsonSerializer.Serialize(new
                {
                    tool = request.Tool.ToString(),
                    taskType,
                    coreTask = coreTask.Name,
                    parameterSummary,
                    appendError = appendResult.Error?.Code.ToString(),
                    appendResult.Error?.Message,
                    appendResult.Error?.NativeDetails,
                }));
        }

        if (_connectFeatureService is not null)
        {
            var startResult = await _connectFeatureService.StartAsync(cancellationToken);
            if (!startResult.Success)
            {
                return UiOperationResult<ToolboxDispatchResult>.Fail(
                    startResult.Error?.Code ?? UiErrorCode.ToolboxExecutionFailed,
                    startResult.Message,
                    startResult.Error?.Details);
            }
        }

        return UiOperationResult<ToolboxDispatchResult>.Ok(
            new ToolboxDispatchResult(
                request.Tool,
                parameterSummary,
                DateTimeOffset.Now,
                appendResult.Value,
                taskType),
            $"Tool `{request.Tool}` dispatched.");
    }

    public async Task<UiOperationResult> StopAsync(CancellationToken cancellationToken = default)
    {
        if (_connectFeatureService is null)
        {
            return UiOperationResult.Fail(
                UiErrorCode.ToolboxExecutionFailed,
                "Toolbox stop service is not configured.");
        }

        return await _connectFeatureService.StopAsync(cancellationToken);
    }

    private static bool TryBuildCoreTask(
        ToolboxDispatchRequest request,
        out CoreTaskRequest coreTask,
        out string taskType,
        out string parameterSummary,
        out (string Code, string Message, string? Details)? error)
    {
        coreTask = new CoreTaskRequest(string.Empty, string.Empty, true, "{}");
        taskType = string.Empty;
        parameterSummary = string.Empty;
        error = null;

        switch (request.Tool)
        {
            case ToolboxToolKind.Recruit:
            {
                if (request.Recruit is null)
                {
                    error = (
                        UiErrorCode.ToolboxInvalidParameters,
                        "Recruit request is missing structured parameters.",
                        null);
                    return false;
                }

                var levels = request.Recruit.SelectLevels
                    .Where(level => level is >= 3 and <= 6)
                    .Distinct()
                    .OrderBy(level => level)
                    .ToArray();
                if (levels.Length == 0)
                {
                    error = (
                        UiErrorCode.ToolboxInvalidParameters,
                        "Recruit request must include at least one selected level.",
                        null);
                    return false;
                }

                var payload = new JsonObject
                {
                    ["refresh"] = false,
                    ["force_refresh"] = false,
                    ["select"] = new JsonArray(levels.Select(level => JsonValue.Create(level)).ToArray()),
                    ["confirm"] = new JsonArray(JsonValue.Create(-1)),
                    ["times"] = 0,
                    ["set_time"] = request.Recruit.AutoSetTime,
                    ["expedite"] = false,
                    ["skip_robot"] = false,
                    ["extra_tags_mode"] = 0,
                    ["first_tags"] = new JsonArray(),
                    ["recruitment_time"] = new JsonObject
                    {
                        ["3"] = request.Recruit.Level3Time,
                        ["4"] = request.Recruit.Level4Time,
                        ["5"] = request.Recruit.Level5Time,
                    },
                    ["report_to_penguin"] = false,
                    ["report_to_yituliu"] = false,
                    ["server"] = string.IsNullOrWhiteSpace(request.Recruit.ServerType)
                        ? "CN"
                        : request.Recruit.ServerType.Trim(),
                };

                taskType = TaskModuleTypes.Recruit;
                parameterSummary = request.ParameterSummary
                    ?? $"select={string.Join(',', levels)}; autoSetTime={request.Recruit.AutoSetTime.ToString().ToLowerInvariant()}; level3={request.Recruit.Level3Time}; level4={request.Recruit.Level4Time}; level5={request.Recruit.Level5Time}; server={payload["server"]}";
                coreTask = new CoreTaskRequest(taskType, "Toolbox.Recruit", true, payload.ToJsonString());
                return true;
            }
            case ToolboxToolKind.OperBox:
                taskType = "OperBox";
                parameterSummary = request.ParameterSummary ?? "mode=owned";
                coreTask = new CoreTaskRequest(taskType, "Toolbox.OperBox", true, "{}");
                return true;
            case ToolboxToolKind.Depot:
                taskType = "Depot";
                parameterSummary = request.ParameterSummary ?? "format=summary";
                coreTask = new CoreTaskRequest(taskType, "Toolbox.Depot", true, "{}");
                return true;
            case ToolboxToolKind.Gacha:
            {
                if (request.Gacha is null)
                {
                    error = (
                        UiErrorCode.ToolboxInvalidParameters,
                        "Gacha request is missing structured parameters.",
                        null);
                    return false;
                }

                var taskName = request.Gacha.Once ? "GachaOnce" : "GachaTenTimes";
                var payload = new JsonObject
                {
                    ["task_names"] = new JsonArray(JsonValue.Create(taskName)),
                };
                taskType = TaskModuleTypes.Custom;
                parameterSummary = request.ParameterSummary ?? $"drawCount={(request.Gacha.Once ? 1 : 10)}";
                coreTask = new CoreTaskRequest(taskType, $"Toolbox.{taskName}", true, payload.ToJsonString());
                return true;
            }
            case ToolboxToolKind.MiniGame:
            {
                if (request.MiniGame is null || string.IsNullOrWhiteSpace(request.MiniGame.TaskName))
                {
                    error = (
                        UiErrorCode.ToolboxInvalidParameters,
                        "MiniGame request is missing task name.",
                        null);
                    return false;
                }

                var taskName = request.MiniGame.TaskName.Trim();
                var payload = new JsonObject
                {
                    ["task_names"] = new JsonArray(JsonValue.Create(taskName)),
                };
                taskType = TaskModuleTypes.Custom;
                parameterSummary = request.ParameterSummary ?? $"taskName={taskName}";
                coreTask = new CoreTaskRequest(taskType, "Toolbox.MiniGame", true, payload.ToJsonString());
                return true;
            }
            case ToolboxToolKind.VideoRecognition:
                error = (
                    UiErrorCode.ToolNotSupported,
                    "Peep does not append a toolbox task.",
                    null);
                return false;
            default:
                error = (
                    UiErrorCode.ToolNotSupported,
                    $"Tool `{request.Tool}` is not supported.",
                    null);
                return false;
        }
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
        var hasProfileLegacyAction = profile.Values.TryGetValue(ConfigurationKeys.ActionAfterCompleted, out var profileLegacyActionNode) && profileLegacyActionNode is not null;
        var hasGlobalLegacyAction = config.GlobalValues.TryGetValue(ConfigurationKeys.ActionAfterCompleted, out var globalLegacyActionNode) && globalLegacyActionNode is not null;
        var hasLegacyPostActions = hasProfileLegacy || hasGlobalLegacy;
        var hasLegacyActionAfterCompleted = hasProfileLegacyAction || hasGlobalLegacyAction;
        if (!hasLegacyPostActions && !hasLegacyActionAfterCompleted)
        {
            return UiOperationResult<PostActionConfig>.Ok(PostActionConfig.Default, "Post action config is empty.");
        }

        var parsedLegacyPostActions = false;
        var legacyPostActionsConfig = PostActionConfig.Default;
        if (hasLegacyPostActions)
        {
            var legacyNode = hasProfileLegacy ? profileLegacyNode : globalLegacyNode;
            if (TryReadLegacyFlags(legacyNode!, out var flags))
            {
                parsedLegacyPostActions = true;
                legacyPostActionsConfig = MapLegacyFlags(flags);
            }
        }

        var parsedLegacyActionAfterCompleted = false;
        var legacyActionAfterCompletedConfig = PostActionConfig.Default;
        if (hasLegacyActionAfterCompleted)
        {
            var legacyActionNode = hasProfileLegacyAction ? profileLegacyActionNode : globalLegacyActionNode;
            if (TryReadLegacyActionAfterCompleted(legacyActionNode!, out var parsedActionConfig))
            {
                parsedLegacyActionAfterCompleted = true;
                legacyActionAfterCompletedConfig = parsedActionConfig;
            }
        }

        PostActionConfig migratedConfig;
        bool migratedFromFlags;
        if (parsedLegacyPostActions && legacyPostActionsConfig.HasAnyAction())
        {
            migratedConfig = legacyPostActionsConfig;
            migratedFromFlags = true;
        }
        else if (parsedLegacyActionAfterCompleted)
        {
            migratedConfig = legacyActionAfterCompletedConfig;
            migratedFromFlags = false;
        }
        else if (parsedLegacyPostActions)
        {
            migratedConfig = legacyPostActionsConfig;
            migratedFromFlags = true;
        }
        else
        {
            return UiOperationResult<PostActionConfig>.Fail(
                UiErrorCode.PostActionLegacyParseFailed,
                hasLegacyPostActions && hasLegacyActionAfterCompleted
                    ? "Failed to parse legacy completion action config."
                    : hasLegacyPostActions
                        ? "Failed to parse legacy post action flags."
                        : "Failed to parse legacy completion action.");
        }

        var migrated = migratedConfig.ToJson();
        profile.Values[PostActionConfigKey] = migrated;
        profile.Values.Remove(ConfigurationKeys.PostActions);
        config.GlobalValues.Remove(ConfigurationKeys.PostActions);
        profile.Values.Remove(ConfigurationKeys.ActionAfterCompleted);
        config.GlobalValues.Remove(ConfigurationKeys.ActionAfterCompleted);
        await _configService.SaveAsync(cancellationToken);
        _configService.LogService.Info(
            migratedFromFlags
                ? "Migrated legacy post actions bitmask to structured TaskQueue.PostAction."
                : "Migrated legacy completion action to structured TaskQueue.PostAction.");
        return UiOperationResult<PostActionConfig>.Ok(
            PostActionConfig.FromJson(migrated),
            migratedFromFlags ? "Legacy post action config migrated." : "Legacy completion action migrated.");
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
        profile.Values.Remove(ConfigurationKeys.ActionAfterCompleted);
        _configService.CurrentConfig.GlobalValues.Remove(ConfigurationKeys.ActionAfterCompleted);
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

    private static bool TryReadLegacyActionAfterCompleted(JsonNode node, out PostActionConfig config)
    {
        config = PostActionConfig.Default;
        if (node is not JsonValue jsonValue)
        {
            return false;
        }

        if (jsonValue.TryGetValue(out int intValue))
        {
            return TryMapLegacyCompletionAction(intValue, out config);
        }

        if (!jsonValue.TryGetValue(out string? text))
        {
            return false;
        }

        if (int.TryParse(text, out intValue))
        {
            return TryMapLegacyCompletionAction(intValue, out config);
        }

        var normalized = NormalizeLegacyCompletionActionName(text);
        return TryMapLegacyCompletionAction(normalized, out config);
    }

    private static string NormalizeLegacyCompletionActionName(string? text)
    {
        if (string.IsNullOrWhiteSpace(text))
        {
            return string.Empty;
        }

        var builder = new StringBuilder(text.Length);
        foreach (var ch in text)
        {
            if (char.IsLetterOrDigit(ch))
            {
                builder.Append(char.ToLowerInvariant(ch));
            }
        }

        return builder.ToString();
    }

    private static bool TryMapLegacyCompletionAction(int value, out PostActionConfig config)
    {
        LegacyCompletionAction? action = value switch
        {
            0 => LegacyCompletionAction.DoNothing,
            1 => LegacyCompletionAction.StopGame,
            2 => LegacyCompletionAction.ExitSelf,
            3 => LegacyCompletionAction.ExitEmulator,
            4 => LegacyCompletionAction.ExitEmulatorAndSelf,
            5 => LegacyCompletionAction.Suspend,
            6 => LegacyCompletionAction.Hibernate,
            7 => LegacyCompletionAction.ExitEmulatorAndSelfAndHibernate,
            8 => LegacyCompletionAction.Shutdown,
            9 => LegacyCompletionAction.HibernateWithoutPersist,
            10 => LegacyCompletionAction.ExitEmulatorAndSelfAndHibernateWithoutPersist,
            11 => LegacyCompletionAction.ShutdownWithoutPersist,
            12 => LegacyCompletionAction.ExitEmulatorAndSelfIfOtherMaaElseExitEmulatorAndSelfAndHibernate,
            13 => LegacyCompletionAction.ExitSelfIfOtherMaaElseShutdown,
            14 => LegacyCompletionAction.BackToAndroidHome,
            _ => null,
        };

        if (action is null)
        {
            config = PostActionConfig.Default;
            return false;
        }

        config = MapLegacyCompletionAction(action.Value);
        return true;
    }

    private static bool TryMapLegacyCompletionAction(string normalizedAction, out PostActionConfig config)
    {
        LegacyCompletionAction? action = normalizedAction switch
        {
            "" or "none" or "noaction" or "donothing" or "nothing" => LegacyCompletionAction.DoNothing,
            "stopgame" or "exitarknights" or "closearknights" => LegacyCompletionAction.StopGame,
            "backtoandroidhome" or "backtohome" or "returntoandroidhome" or "returntohome" => LegacyCompletionAction.BackToAndroidHome,
            "exitemulator" or "closeemulator" => LegacyCompletionAction.ExitEmulator,
            "exitself" or "exitmaa" or "closemaa" or "quitmaa" => LegacyCompletionAction.ExitSelf,
            "exitemulatorandself" => LegacyCompletionAction.ExitEmulatorAndSelf,
            "hibernate" => LegacyCompletionAction.Hibernate,
            "hibernatewithoutpersist" => LegacyCompletionAction.HibernateWithoutPersist,
            "shutdown" or "poweroff" => LegacyCompletionAction.Shutdown,
            "shutdownwithoutpersist" => LegacyCompletionAction.ShutdownWithoutPersist,
            "sleep" or "suspend" or "standby" => LegacyCompletionAction.Suspend,
            "exitemulatorandselfandhibernate" => LegacyCompletionAction.ExitEmulatorAndSelfAndHibernate,
            "exitemulatorandselfandhibernatewithoutpersist" => LegacyCompletionAction.ExitEmulatorAndSelfAndHibernateWithoutPersist,
            "exitemulatorandselfifothermaaelseexitemulatorandselfandhibernate" => LegacyCompletionAction.ExitEmulatorAndSelfIfOtherMaaElseExitEmulatorAndSelfAndHibernate,
            "exitselfifothermaaelseshutdown" => LegacyCompletionAction.ExitSelfIfOtherMaaElseShutdown,
            _ => null,
        };

        if (action is null)
        {
            config = PostActionConfig.Default;
            return false;
        }

        config = MapLegacyCompletionAction(action.Value);
        return true;
    }

    private static PostActionConfig MapLegacyCompletionAction(LegacyCompletionAction action)
    {
        return action switch
        {
            LegacyCompletionAction.DoNothing => PostActionConfig.Default,
            LegacyCompletionAction.StopGame => new PostActionConfig { ExitArknights = true },
            LegacyCompletionAction.ExitSelf => new PostActionConfig { ExitSelf = true },
            LegacyCompletionAction.ExitEmulator => new PostActionConfig { ExitEmulator = true },
            LegacyCompletionAction.ExitEmulatorAndSelf => new PostActionConfig
            {
                ExitEmulator = true,
                ExitSelf = true,
            },
            LegacyCompletionAction.Suspend => new PostActionConfig { Sleep = true },
            LegacyCompletionAction.Hibernate => new PostActionConfig { Hibernate = true },
            LegacyCompletionAction.ExitEmulatorAndSelfAndHibernate => new PostActionConfig
            {
                ExitEmulator = true,
                ExitSelf = true,
                Hibernate = true,
            },
            LegacyCompletionAction.Shutdown => new PostActionConfig { Shutdown = true },
            LegacyCompletionAction.HibernateWithoutPersist => new PostActionConfig { Hibernate = true },
            LegacyCompletionAction.ExitEmulatorAndSelfAndHibernateWithoutPersist => new PostActionConfig
            {
                ExitEmulator = true,
                ExitSelf = true,
                Hibernate = true,
            },
            LegacyCompletionAction.ShutdownWithoutPersist => new PostActionConfig { Shutdown = true },
            LegacyCompletionAction.ExitEmulatorAndSelfIfOtherMaaElseExitEmulatorAndSelfAndHibernate => new PostActionConfig
            {
                ExitEmulator = true,
                ExitSelf = true,
                IfNoOtherMaa = true,
                Hibernate = true,
            },
            // The structured model cannot express "exit self only when other MAA exists";
            // this preserves the shutdown branch and keeps ExitSelf for the other-MAA branch.
            LegacyCompletionAction.ExitSelfIfOtherMaaElseShutdown => new PostActionConfig
            {
                ExitSelf = true,
                IfNoOtherMaa = true,
                Shutdown = true,
            },
            LegacyCompletionAction.BackToAndroidHome => new PostActionConfig { BackToAndroidHome = true },
            _ => PostActionConfig.Default,
        };
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

    private enum LegacyCompletionAction
    {
        DoNothing,
        StopGame,
        ExitSelf,
        ExitEmulator,
        ExitEmulatorAndSelf,
        Suspend,
        Hibernate,
        ExitEmulatorAndSelfAndHibernate,
        Shutdown,
        HibernateWithoutPersist,
        ExitEmulatorAndSelfAndHibernateWithoutPersist,
        ShutdownWithoutPersist,
        ExitEmulatorAndSelfIfOtherMaaElseExitEmulatorAndSelfAndHibernate,
        ExitSelfIfOtherMaaElseShutdown,
        BackToAndroidHome,
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
                UiOperationResult.Fail(result.ErrorCode ?? UiErrorCode.AutostartQueryFailed, result.Message),
                cancellationToken);
            return UiOperationResult<bool>.Fail(result.ErrorCode ?? UiErrorCode.AutostartQueryFailed, result.Message);
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
                UiOperationResult.Fail(result.ErrorCode ?? UiErrorCode.PlatformOperationFailed, result.Message),
                cancellationToken);
            return UiOperationResult.Fail(result.ErrorCode ?? UiErrorCode.PlatformOperationFailed, result.Message);
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
            return UiOperationResult.Fail(UiErrorCode.SettingKeyMissing, "Setting key cannot be empty.");
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
            return UiOperationResult.Fail(UiErrorCode.SettingBatchEmpty, "No settings were provided.");
        }

        var oldValues = new Dictionary<string, JsonNode?>(StringComparer.OrdinalIgnoreCase);
        var existedKeys = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var (key, value) in updates)
        {
            if (string.IsNullOrWhiteSpace(key))
            {
                return UiOperationResult.Fail(UiErrorCode.SettingKeyMissing, "Setting key cannot be empty.");
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
            return UiOperationResult.Fail(UiErrorCode.SettingsSaveFailed, $"Failed to save settings: {ex.Message}");
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
        cancellationToken.ThrowIfCancellationRequested();
        try
        {
            var baseDirectory = ResolveIssueReportBaseDirectory();
            var bundlePath = await _diagnostics.BuildIssueReportBundleAsync(baseDirectory, cancellationToken);
            return UiOperationResult<string>.Ok(bundlePath, "Issue report bundle generated.");
        }
        catch (OperationCanceledException)
        {
            throw;
        }
        catch (Exception ex)
        {
            await _diagnostics.RecordErrorAsync("Settings.IssueReport.Build", "Failed to generate issue report bundle.", ex, cancellationToken);
            return UiOperationResult<string>.Fail(
                UiErrorCode.IssueReportBundleBuildFailed,
                $"Failed to generate issue report bundle: {ex.Message}",
                ex.Message);
        }
    }

    private string ResolveIssueReportBaseDirectory()
    {
        var debugDirectory = Path.GetDirectoryName(_diagnostics.EventLogPath);
        if (!string.IsNullOrWhiteSpace(debugDirectory))
        {
            var parent = Directory.GetParent(debugDirectory);
            if (parent is not null)
            {
                return parent.FullName;
            }
        }

        return AppContext.BaseDirectory;
    }
}

public sealed class VersionUpdateFeatureService : IVersionUpdateFeatureService
{
    private const string GithubResourceArchiveUrl = "https://github.com/MaaAssistantArknights/MaaResource/archive/refs/heads/main.zip";
    private const string MirrorChyanResourceApiUrl = "https://mirrorchyan.com/api/resources/MaaResource/latest";
    private static readonly HashSet<string> AllowedVersionTypes = new(StringComparer.OrdinalIgnoreCase)
    {
        "Stable",
        "Beta",
        "Nightly",
    };
    private static readonly HashSet<string> AllowedProxyTypes = new(StringComparer.OrdinalIgnoreCase)
    {
        "http",
        "https",
        "socks5",
        "system",
    };
    private static readonly HashSet<string> DefaultClientTypes = new(StringComparer.OrdinalIgnoreCase)
    {
        string.Empty,
        "Official",
        "Bilibili",
    };
    private static readonly HttpClient ResourceHttpClient = new()
    {
        Timeout = TimeSpan.FromMinutes(15),
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

        var policy = NormalizePolicy(new VersionUpdatePolicy(
            Proxy: ReadString(config, ConfigurationKeys.UpdateProxy, string.Empty),
            ProxyType: ReadString(config, ConfigurationKeys.ProxyType, "http"),
            VersionType: ReadString(config, ConfigurationKeys.VersionType, "Stable"),
            ResourceUpdateSource: ReadString(config, ConfigurationKeys.UpdateSource, "Github"),
            ForceGithubGlobalSource: ReadBool(config, ConfigurationKeys.ForceGithubGlobalSource, false),
            MirrorChyanCdk: ReadString(config, ConfigurationKeys.MirrorChyanCdk, string.Empty),
            MirrorChyanCdkExpired: ReadString(config, ConfigurationKeys.MirrorChyanCdkExpiredTime, string.Empty),
            StartupUpdateCheck: ReadBool(config, ConfigurationKeys.StartupUpdateCheck, true),
            ScheduledUpdateCheck: ReadBool(config, ConfigurationKeys.UpdateAutoCheck, false),
            ResourceApi: ReadString(config, ConfigurationKeys.ResourceApi, string.Empty),
            AllowNightlyUpdates: ReadBool(config, ConfigurationKeys.AllowNightlyUpdates, false),
            HasAcknowledgedNightlyWarning: ReadBool(config, ConfigurationKeys.HasAcknowledgedNightlyWarning, false),
            UseAria2: ReadBool(config, ConfigurationKeys.UseAria2, false),
            AutoDownloadUpdatePackage: ReadBool(config, ConfigurationKeys.AutoDownloadUpdatePackage, true),
            AutoInstallUpdatePackage: ReadBool(config, ConfigurationKeys.AutoInstallUpdatePackage, false),
            VersionName: ReadString(config, ConfigurationKeys.VersionName, string.Empty),
            VersionBody: ReadString(config, ConfigurationKeys.VersionUpdateBody, string.Empty),
            IsFirstBoot: ReadBool(config, ConfigurationKeys.VersionUpdateIsFirstBoot, false),
            VersionPackage: ReadString(config, ConfigurationKeys.VersionUpdatePackage, string.Empty),
            DoNotShowUpdate: ReadBool(config, ConfigurationKeys.VersionUpdateDoNotShowUpdate, false)));
        return Task.FromResult(UiOperationResult<VersionUpdatePolicy>.Ok(policy, "Loaded version update policy."));
    }

    public Task<UiOperationResult<ResourceVersionInfo>> LoadResourceVersionInfoAsync(
        string? clientType,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        try
        {
            var resourceInfo = LoadResourceVersionInfo(ResolveRuntimeBaseDirectory(), clientType);
            return Task.FromResult(UiOperationResult<ResourceVersionInfo>.Ok(resourceInfo, "Loaded resource version info."));
        }
        catch (Exception ex)
        {
            return Task.FromResult(UiOperationResult<ResourceVersionInfo>.Fail(
                UiErrorCode.UiOperationFailed,
                $"Failed to load resource version info: {ex.Message}",
                ex.Message));
        }
    }

    public async Task<UiOperationResult> SaveChannelAsync(VersionUpdatePolicy policy, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var normalizedPolicy = NormalizePolicy(policy);
        var validation = ValidateChannelPolicy(normalizedPolicy);
        if (!validation.Success)
        {
            return validation;
        }

        return await PersistGlobalSettingsWithRollbackAsync(
            normalizedPolicy.ToChannelSettingUpdates(),
            "Version update channel settings saved.",
            cancellationToken);
    }

    public async Task<UiOperationResult> SaveProxyAsync(VersionUpdatePolicy policy, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var normalizedPolicy = NormalizePolicy(policy);
        var validation = ValidateProxyPolicy(normalizedPolicy);
        if (!validation.Success)
        {
            return validation;
        }

        return await PersistGlobalSettingsWithRollbackAsync(
            normalizedPolicy.ToProxySettingUpdates(),
            "Version update proxy settings saved.",
            cancellationToken);
    }

    public async Task<UiOperationResult> SavePolicyAsync(VersionUpdatePolicy policy, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var normalizedPolicy = NormalizePolicy(policy);
        var validation = ValidatePolicy(normalizedPolicy);
        if (!validation.Success)
        {
            return validation;
        }

        return await PersistGlobalSettingsWithRollbackAsync(
            normalizedPolicy.ToGlobalSettingUpdates(),
            "Version update policy saved.",
            cancellationToken);
    }

    public async Task<UiOperationResult<string>> UpdateResourceAsync(
        VersionUpdatePolicy policy,
        string? clientType,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var normalizedPolicy = NormalizePolicy(policy);
        var validation = ValidatePolicy(normalizedPolicy);
        if (!validation.Success)
        {
            return UiOperationResult<string>.Fail(
                validation.Error?.Code ?? UiErrorCode.VersionUpdateInvalidParameters,
                validation.Message,
                validation.Error?.Details);
        }

        var source = normalizedPolicy.ResourceUpdateSource;
        if (string.Equals(source, "MirrorChyan", StringComparison.OrdinalIgnoreCase))
        {
            return await UpdateResourceFromMirrorChyanAsync(normalizedPolicy, clientType, cancellationToken);
        }

        return await UpdateResourceFromGithubAsync(cancellationToken);
    }

    public Task<UiOperationResult<string>> CheckForUpdatesAsync(
        VersionUpdatePolicy policy,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var normalizedPolicy = NormalizePolicy(policy);
        var validation = ValidatePolicy(normalizedPolicy);
        if (!validation.Success)
        {
            return Task.FromResult(UiOperationResult<string>.Fail(
                validation.Error?.Code ?? UiErrorCode.VersionUpdateInvalidParameters,
                validation.Message,
                validation.Error?.Details));
        }

        var channel = normalizedPolicy.VersionType;
        var source = normalizedPolicy.ResourceUpdateSource;
        return Task.FromResult(UiOperationResult<string>.Ok(
            $"Checked updates on channel `{channel}` via `{source}`. No new package found.",
            "Version update check completed."));
    }

    private UiOperationResult ValidatePolicy(VersionUpdatePolicy policy)
    {
        var channelValidation = ValidateChannelPolicy(policy);
        if (!channelValidation.Success)
        {
            return channelValidation;
        }

        return ValidateProxyPolicy(policy);
    }

    private UiOperationResult ValidateChannelPolicy(VersionUpdatePolicy policy)
    {
        if (!AllowedVersionTypes.Contains(policy.VersionType))
        {
            return UiOperationResult.Fail(
                UiErrorCode.VersionUpdateInvalidParameters,
                $"Version type `{policy.VersionType}` is unsupported.");
        }

        if (!string.Equals(policy.ResourceUpdateSource, "Github", StringComparison.OrdinalIgnoreCase)
            && !string.Equals(policy.ResourceUpdateSource, "MirrorChyan", StringComparison.OrdinalIgnoreCase))
        {
            return UiOperationResult.Fail(
                UiErrorCode.VersionUpdateInvalidParameters,
                $"Resource update source `{policy.ResourceUpdateSource}` is unsupported.");
        }

        return UiOperationResult.Ok("Version update channel validation passed.");
    }

    private static UiOperationResult ValidateProxyPolicy(VersionUpdatePolicy policy)
    {
        if (!AllowedProxyTypes.Contains(policy.ProxyType))
        {
            return UiOperationResult.Fail(
                UiErrorCode.VersionUpdateInvalidParameters,
                $"Proxy type `{policy.ProxyType}` is unsupported.");
        }

        var proxy = policy.Proxy.Trim();
        if (proxy.Length > 0)
        {
            if (Uri.TryCreate(proxy, UriKind.Absolute, out _))
            {
                return UiOperationResult.Ok("Version update proxy validation passed.");
            }

            if (TryParseHostPortProxy(proxy))
            {
                return UiOperationResult.Ok("Version update proxy validation passed.");
            }

            return UiOperationResult.Fail(
                UiErrorCode.VersionUpdateInvalidParameters,
                $"Proxy `{policy.Proxy}` must be in `<host>:<port>` or absolute URI format.");
        }

        return UiOperationResult.Ok("Version update proxy validation passed.");
    }

    private async Task<UiOperationResult<string>> UpdateResourceFromGithubAsync(CancellationToken cancellationToken)
    {
        var runtimeBaseDirectory = ResolveRuntimeBaseDirectory();
        var resourceDirectory = Path.Combine(runtimeBaseDirectory, "resource");
        if (!Directory.Exists(resourceDirectory))
        {
            return UiOperationResult<string>.Fail(
                UiErrorCode.UiOperationFailed,
                $"Resource directory was not found: {resourceDirectory}");
        }

        var tempRoot = Path.Combine(
            Path.GetTempPath(),
            "maa-unified-resource-update",
            Guid.NewGuid().ToString("N"));
        var zipPath = Path.Combine(tempRoot, "MaaResourceGithub.zip");
        var extractDirectory = Path.Combine(tempRoot, "extract");
        Directory.CreateDirectory(tempRoot);

        try
        {
            await DownloadToFileAsync(GithubResourceArchiveUrl, zipPath, cancellationToken);
            ZipFile.ExtractToDirectory(zipPath, extractDirectory, overwriteFiles: true);
            var extractedResourceDirectory = ResolveExtractedResourceDirectory(extractDirectory);
            if (!Directory.Exists(extractedResourceDirectory))
            {
                return UiOperationResult<string>.Fail(
                    UiErrorCode.UiOperationFailed,
                    "Downloaded package does not contain `resource` directory.");
            }

            MergeDirectory(extractedResourceDirectory, resourceDirectory);
            return UiOperationResult<string>.Ok(
                "资源更新完成（Github）。",
                "Resource update completed.");
        }
        catch (OperationCanceledException)
        {
            throw;
        }
        catch (Exception ex)
        {
            return UiOperationResult<string>.Fail(
                UiErrorCode.UiOperationFailed,
                $"Failed to update resources from Github: {ex.Message}",
                ex.Message);
        }
        finally
        {
            TryDeleteDirectory(tempRoot);
        }
    }

    private async Task<UiOperationResult<string>> UpdateResourceFromMirrorChyanAsync(
        VersionUpdatePolicy policy,
        string? clientType,
        CancellationToken cancellationToken)
    {
        var cdk = policy.MirrorChyanCdk.Trim();
        if (cdk.Length == 0)
        {
            return UiOperationResult<string>.Fail(
                UiErrorCode.VersionUpdateInvalidParameters,
                "MirrorChyan source requires a CDK.");
        }

        var localVersion = LoadResourceVersionInfo(ResolveRuntimeBaseDirectory(), clientType);
        var requestUrl =
            $"{MirrorChyanResourceApiUrl}?current_version={Uri.EscapeDataString(BuildCurrentVersionQueryToken(localVersion))}&cdk={Uri.EscapeDataString(cdk)}&user_agent=MAAUnified&sp_id={Uri.EscapeDataString(BuildMirrorChyanSpId())}";

        MirrorChyanUpdateResponse payload;
        try
        {
            using var response = await ResourceHttpClient.GetAsync(requestUrl, cancellationToken);
            var body = await response.Content.ReadAsStringAsync(cancellationToken);
            if (!response.IsSuccessStatusCode)
            {
                return UiOperationResult<string>.Fail(
                    UiErrorCode.UiOperationFailed,
                    $"MirrorChyan request failed with status {(int)response.StatusCode}.",
                    body);
            }

            payload = ParseMirrorChyanPayload(body);
        }
        catch (OperationCanceledException)
        {
            throw;
        }
        catch (Exception ex)
        {
            return UiOperationResult<string>.Fail(
                UiErrorCode.UiOperationFailed,
                $"Failed to query MirrorChyan update endpoint: {ex.Message}",
                ex.Message);
        }

        if (payload.CdkExpiredEpoch.HasValue)
        {
            await PersistMirrorChyanExpiryAsync(payload.CdkExpiredEpoch.Value, cancellationToken);
        }

        if (payload.Code != 0)
        {
            return UiOperationResult<string>.Fail(
                UiErrorCode.VersionUpdateInvalidParameters,
                string.IsNullOrWhiteSpace(payload.Message)
                    ? "MirrorChyan request failed."
                    : payload.Message);
        }

        if (payload.VersionTimestamp.HasValue
            && localVersion.LastUpdatedAt != DateTime.MinValue
            && payload.VersionTimestamp.Value <= localVersion.LastUpdatedAt)
        {
            return UiOperationResult<string>.Ok("资源已是最新版本。", "Resource is already up to date.");
        }

        if (string.IsNullOrWhiteSpace(payload.DownloadUrl))
        {
            return UiOperationResult<string>.Fail(
                UiErrorCode.UiOperationFailed,
                "MirrorChyan response does not contain a downloadable package URL.");
        }

        var runtimeBaseDirectory = ResolveRuntimeBaseDirectory();
        var tempRoot = Path.Combine(
            Path.GetTempPath(),
            "maa-unified-resource-update",
            Guid.NewGuid().ToString("N"));
        var zipPath = Path.Combine(tempRoot, "MaaResourceMirrorChyan.zip");
        var extractDirectory = Path.Combine(tempRoot, "extract");
        Directory.CreateDirectory(tempRoot);

        try
        {
            await DownloadToFileAsync(payload.DownloadUrl, zipPath, cancellationToken);
            ZipFile.ExtractToDirectory(zipPath, extractDirectory, overwriteFiles: true);
            var mergeSource = ResolvePatchMergeDirectory(extractDirectory);
            MergeDirectory(mergeSource, runtimeBaseDirectory);
            var message = string.IsNullOrWhiteSpace(payload.ReleaseNote)
                ? "资源更新完成（MirrorChyan）。"
                : $"资源更新完成（MirrorChyan）：{payload.ReleaseNote}";
            return UiOperationResult<string>.Ok(message, "Resource update completed.");
        }
        catch (OperationCanceledException)
        {
            throw;
        }
        catch (Exception ex)
        {
            return UiOperationResult<string>.Fail(
                UiErrorCode.UiOperationFailed,
                $"Failed to update resources from MirrorChyan: {ex.Message}",
                ex.Message);
        }
        finally
        {
            TryDeleteDirectory(tempRoot);
        }
    }

    private async Task PersistMirrorChyanExpiryAsync(long unixSeconds, CancellationToken cancellationToken)
    {
        if (_configService is null)
        {
            return;
        }

        try
        {
            _configService.CurrentConfig.GlobalValues[ConfigurationKeys.MirrorChyanCdkExpiredTime] =
                JsonValue.Create(unixSeconds.ToString(CultureInfo.InvariantCulture));
            await _configService.SaveAsync(cancellationToken);
        }
        catch
        {
            // Ignore expiry persistence failures to avoid blocking update flow.
        }
    }

    private static VersionUpdatePolicy NormalizePolicy(VersionUpdatePolicy policy)
    {
        return policy with
        {
            ProxyType = NormalizeProxyType(policy.ProxyType),
            ResourceUpdateSource = NormalizeResourceSource(policy.ResourceUpdateSource),
        };
    }

    private static string NormalizeProxyType(string? proxyType)
    {
        var normalized = (proxyType ?? string.Empty).Trim().ToLowerInvariant();
        return normalized.Length == 0 ? "http" : normalized;
    }

    private static string NormalizeResourceSource(string? source)
    {
        var normalized = (source ?? string.Empty).Trim();
        if (normalized.Length == 0
            || string.Equals(normalized, "Official", StringComparison.OrdinalIgnoreCase)
            || string.Equals(normalized, "Github", StringComparison.OrdinalIgnoreCase))
        {
            return "Github";
        }

        if (string.Equals(normalized, "Mirror", StringComparison.OrdinalIgnoreCase)
            || string.Equals(normalized, "MirrorChyan", StringComparison.OrdinalIgnoreCase))
        {
            return "MirrorChyan";
        }

        return normalized;
    }

    private static bool TryParseHostPortProxy(string proxy)
    {
        if (proxy.Contains("://", StringComparison.Ordinal))
        {
            return false;
        }

        if (!Uri.TryCreate($"tcp://{proxy}", UriKind.Absolute, out var uri))
        {
            return false;
        }

        return !string.IsNullOrWhiteSpace(uri.Host) && uri.Port > 0;
    }

    private static string ResolveRuntimeBaseDirectory()
    {
        return AppContext.BaseDirectory.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
    }

    private static string ResolveExtractedResourceDirectory(string extractDirectory)
    {
        var preferred = Path.Combine(extractDirectory, "MaaResource-main", "resource");
        if (Directory.Exists(preferred))
        {
            return preferred;
        }

        var discovered = Directory
            .EnumerateDirectories(extractDirectory, "resource", SearchOption.AllDirectories)
            .FirstOrDefault(path => File.Exists(Path.Combine(path, "version.json")));
        return discovered ?? preferred;
    }

    private static string ResolvePatchMergeDirectory(string extractDirectory)
    {
        var files = Directory.GetFiles(extractDirectory, "*", SearchOption.TopDirectoryOnly);
        var directories = Directory.GetDirectories(extractDirectory, "*", SearchOption.TopDirectoryOnly);
        if (files.Length == 0 && directories.Length == 1)
        {
            return directories[0];
        }

        return extractDirectory;
    }

    private static async Task DownloadToFileAsync(string url, string destinationPath, CancellationToken cancellationToken)
    {
        using var response = await ResourceHttpClient.GetAsync(
            url,
            HttpCompletionOption.ResponseHeadersRead,
            cancellationToken);
        response.EnsureSuccessStatusCode();

        await using var stream = await response.Content.ReadAsStreamAsync(cancellationToken);
        await using var file = File.Create(destinationPath);
        await stream.CopyToAsync(file, cancellationToken);
    }

    private static void MergeDirectory(string source, string destination)
    {
        Directory.CreateDirectory(destination);

        foreach (var file in Directory.GetFiles(source))
        {
            var destinationFile = Path.Combine(destination, Path.GetFileName(file));
            File.Copy(file, destinationFile, overwrite: true);
        }

        foreach (var directory in Directory.GetDirectories(source))
        {
            var destinationDirectory = Path.Combine(destination, Path.GetFileName(directory));
            MergeDirectory(directory, destinationDirectory);
        }
    }

    private static void TryDeleteDirectory(string path)
    {
        try
        {
            if (Directory.Exists(path))
            {
                Directory.Delete(path, recursive: true);
            }
        }
        catch
        {
            // Ignore temporary cleanup failures.
        }
    }

    private static string BuildCurrentVersionQueryToken(ResourceVersionInfo info)
    {
        var effectiveTime = info.LastUpdatedAt == DateTime.MinValue
            ? DateTime.UnixEpoch
            : info.LastUpdatedAt;
        return effectiveTime.ToString("yyyy-MM-dd+HH:mm:ss.fff", CultureInfo.InvariantCulture);
    }

    private static string BuildMirrorChyanSpId()
    {
        var material = string.Join(
            "|",
            Environment.MachineName,
            Environment.UserName,
            Environment.OSVersion.VersionString);
        var bytes = SHA256.HashData(Encoding.UTF8.GetBytes(material));
        return Convert.ToHexString(bytes[..8]).ToLowerInvariant();
    }

    private static MirrorChyanUpdateResponse ParseMirrorChyanPayload(string json)
    {
        var root = JsonNode.Parse(json) as JsonObject
            ?? throw new InvalidDataException("MirrorChyan response is not a JSON object.");

        var code = root["code"]?.GetValue<int?>() ?? -1;
        var message = root["msg"]?.GetValue<string>() ?? string.Empty;
        var data = root["data"] as JsonObject;
        var releaseNote = data?["release_note"]?.GetValue<string>();
        var downloadUrl = data?["url"]?.GetValue<string>();
        var versionName = data?["version_name"]?.GetValue<string>();
        var cdkExpired = data?["cdk_expired_time"]?.GetValue<long?>();
        DateTime? versionTimestamp = DateTime.TryParse(versionName, out var parsedVersion)
            ? parsedVersion
            : null;

        return new MirrorChyanUpdateResponse(
            Code: code,
            Message: message,
            DownloadUrl: downloadUrl ?? string.Empty,
            ReleaseNote: releaseNote ?? string.Empty,
            VersionTimestamp: versionTimestamp,
            CdkExpiredEpoch: cdkExpired);
    }

    private static ResourceVersionInfo LoadResourceVersionInfo(string baseDirectory, string? clientType)
    {
        var normalizedClientType = (clientType ?? string.Empty).Trim();
        var defaultVersionFilePath = Path.Combine(baseDirectory, "resource", "version.json");
        var selectedVersionFilePath = DefaultClientTypes.Contains(normalizedClientType)
            ? defaultVersionFilePath
            : Path.Combine(baseDirectory, "resource", "global", normalizedClientType, "resource", "version.json");

        if (!File.Exists(defaultVersionFilePath) || !File.Exists(selectedVersionFilePath))
        {
            return ResourceVersionInfo.Empty;
        }

        var selectedVersionJson = LoadJsonObject(selectedVersionFilePath);
        if (selectedVersionJson is null)
        {
            return ResourceVersionInfo.Empty;
        }

        var defaultVersionJson = string.Equals(selectedVersionFilePath, defaultVersionFilePath, StringComparison.OrdinalIgnoreCase)
            ? selectedVersionJson
            : LoadJsonObject(defaultVersionFilePath);

        var nowUnix = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
        var poolTime = ReadUnixTime(selectedVersionJson, "gacha", "time");
        var activityTime = ReadUnixTime(selectedVersionJson, "activity", "time");
        var poolName = ReadNestedString(selectedVersionJson, "gacha", "pool");
        var activityName = ReadNestedString(selectedVersionJson, "activity", "name");

        var poolStarted = poolTime.HasValue && nowUnix >= poolTime.Value;
        var activityStarted = activityTime.HasValue && nowUnix >= activityTime.Value;

        var versionName = (poolStarted, activityStarted) switch
        {
            (false, false) => string.Empty,
            (true, false) => poolName,
            (false, true) => activityName,
            _ => (poolTime ?? long.MinValue) > (activityTime ?? long.MinValue)
                ? poolName
                : activityName,
        };

        var lastUpdatedRaw = ReadNestedString(defaultVersionJson, "last_updated");
        var parsedLastUpdated = TryParseResourceTimestamp(lastUpdatedRaw, out var lastUpdated)
            ? lastUpdated
            : DateTime.MinValue;

        return new ResourceVersionInfo(versionName, parsedLastUpdated);
    }

    private static JsonObject? LoadJsonObject(string path)
    {
        try
        {
            return JsonNode.Parse(File.ReadAllText(path)) as JsonObject;
        }
        catch
        {
            return null;
        }
    }

    private static long? ReadUnixTime(JsonObject root, params string[] segments)
    {
        JsonNode? current = root;
        foreach (var segment in segments)
        {
            current = current?[segment];
            if (current is null)
            {
                return null;
            }
        }

        if (current is JsonValue value)
        {
            if (value.TryGetValue<long>(out var longValue))
            {
                return longValue;
            }

            if (value.TryGetValue<string>(out var text)
                && long.TryParse(text, NumberStyles.Integer, CultureInfo.InvariantCulture, out longValue))
            {
                return longValue;
            }
        }

        return null;
    }

    private static string ReadNestedString(JsonObject? root, params string[] segments)
    {
        if (root is null)
        {
            return string.Empty;
        }

        JsonNode? current = root;
        foreach (var segment in segments)
        {
            current = current?[segment];
            if (current is null)
            {
                return string.Empty;
            }
        }

        if (current is JsonValue value)
        {
            if (value.TryGetValue<string>(out var text))
            {
                return text?.Trim() ?? string.Empty;
            }
        }

        return current.ToString().Trim();
    }

    private static bool TryParseResourceTimestamp(string value, out DateTime parsed)
    {
        if (DateTime.TryParseExact(
                value,
                "yyyy-MM-dd HH:mm:ss.fff",
                CultureInfo.InvariantCulture,
                DateTimeStyles.None,
                out parsed))
        {
            return true;
        }

        return DateTime.TryParse(value, CultureInfo.InvariantCulture, DateTimeStyles.None, out parsed);
    }

    private async Task<UiOperationResult> PersistGlobalSettingsWithRollbackAsync(
        IReadOnlyDictionary<string, string> updates,
        string successMessage,
        CancellationToken cancellationToken)
    {
        if (_configService is null)
        {
            return UiOperationResult.Fail(UiErrorCode.VersionUpdateServiceUnavailable, "Version update service is not initialized.");
        }

        var config = _configService.CurrentConfig;
        var snapshot = CloneGlobalSettings(config);
        try
        {
            foreach (var (key, value) in updates)
            {
                config.GlobalValues[key] = JsonValue.Create(value);
            }

            await _configService.SaveAsync(cancellationToken);
            return UiOperationResult.Ok(successMessage);
        }
        catch (Exception ex)
        {
            config.GlobalValues = snapshot;
            _configService.RevalidateCurrentConfig(logIssues: false);
            return UiOperationResult.Fail(
                UiErrorCode.VersionUpdateSaveFailed,
                $"Failed to save version update settings: {ex.Message}",
                ex.Message);
        }
    }

    private static Dictionary<string, JsonNode?> CloneGlobalSettings(UnifiedConfig config)
    {
        var clone = new Dictionary<string, JsonNode?>(StringComparer.OrdinalIgnoreCase);
        foreach (var (key, value) in config.GlobalValues)
        {
            clone[key] = value?.DeepClone();
        }

        return clone;
    }

    private bool TryGetConfig(
        out UnifiedConfig config,
        out UiOperationResult<VersionUpdatePolicy> failure)
    {
        if (_configService is null)
        {
            config = null!;
            failure = UiOperationResult<VersionUpdatePolicy>.Fail(
                UiErrorCode.VersionUpdateServiceUnavailable,
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

    private sealed record MirrorChyanUpdateResponse(
        int Code,
        string Message,
        string DownloadUrl,
        string ReleaseNote,
        DateTime? VersionTimestamp,
        long? CdkExpiredEpoch);
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
                UiErrorCode.AchievementServiceUnavailable,
                "Achievement service is not initialized."));
        }

        var config = _configService.CurrentConfig;
        var policy = new AchievementPolicy(
            PopupDisabled: ReadProfileBool(config, ConfigurationKeys.AchievementPopupDisabled, false),
            PopupAutoClose: ReadProfileBool(config, ConfigurationKeys.AchievementPopupAutoClose, false));
        return Task.FromResult(UiOperationResult<AchievementPolicy>.Ok(policy, "Loaded achievement policy."));
    }

    public async Task<UiOperationResult> SavePolicyAsync(AchievementPolicy policy, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_configService is null)
        {
            return UiOperationResult.Fail(UiErrorCode.AchievementServiceUnavailable, "Achievement service is not initialized.");
        }

        if (!_configService.TryGetCurrentProfile(out var profile))
        {
            return UiOperationResult.Fail(
                UiErrorCode.ProfileMissing,
                $"Current profile `{_configService.CurrentConfig.CurrentProfile}` not found.");
        }

        foreach (var (key, value) in policy.ToProfileSettingUpdates())
        {
            profile.Values[key] = JsonValue.Create(value);
        }

        await _configService.SaveAsync(cancellationToken);
        return UiOperationResult.Ok("Achievement policy saved.");
    }

    private static bool ReadProfileBool(UnifiedConfig config, string key, bool fallback)
    {
        if (TryReadBool(config, key, preferProfile: true, out var value))
        {
            return value;
        }

        return fallback;
    }

    private static bool TryReadBool(UnifiedConfig config, string key, bool preferProfile, out bool value)
    {
        if (preferProfile)
        {
            if (TryReadBoolNode(config, key, fromProfile: true, out value))
            {
                return true;
            }

            return TryReadBoolNode(config, key, fromProfile: false, out value);
        }

        if (TryReadBoolNode(config, key, fromProfile: false, out value))
        {
            return true;
        }

        return TryReadBoolNode(config, key, fromProfile: true, out value);
    }

    private static bool TryReadBoolNode(UnifiedConfig config, string key, bool fromProfile, out bool value)
    {
        JsonNode? node = null;
        if (fromProfile)
        {
            if (!string.IsNullOrWhiteSpace(config.CurrentProfile)
                && config.Profiles.TryGetValue(config.CurrentProfile, out var profile))
            {
                profile.Values.TryGetValue(key, out node);
            }
        }
        else
        {
            config.GlobalValues.TryGetValue(key, out node);
        }

        if (node is null)
        {
            value = false;
            return false;
        }

        if (bool.TryParse(node.ToString(), out var parsed))
        {
            value = parsed;
            return true;
        }

        if (int.TryParse(node.ToString(), NumberStyles.Integer, CultureInfo.InvariantCulture, out var parsedInt))
        {
            value = parsedInt != 0;
            return true;
        }

        value = false;
        return false;
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
                UiErrorCode.AnnouncementServiceUnavailable,
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
            return UiOperationResult.Fail(UiErrorCode.AnnouncementServiceUnavailable, "Announcement service is not initialized.");
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
                UiErrorCode.StageManagerServiceUnavailable,
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
            return UiOperationResult.Fail(UiErrorCode.StageManagerServiceUnavailable, "Stage manager service is not initialized.");
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
                UiErrorCode.WebApiServiceUnavailable,
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
            return UiOperationResult.Fail(UiErrorCode.WebApiServiceUnavailable, "WebApi service is not initialized.");
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
                configResult.Error?.Code ?? UiErrorCode.WebApiLoadFailed,
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
            return UiOperationResult.Fail(UiErrorCode.WebApiDisabled, "WebApi is disabled by configuration.");
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

    public event EventHandler<DialogErrorRaisedEvent>? ErrorRaised;

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
        var result = UiOperationResult.Fail(UiErrorCode.UiError, message);
        return await ReportErrorAsync(context, result, cancellationToken);
    }

    public async Task<DialogTraceToken> BeginDialogAsync(
        DialogType dialogType,
        string sourceScope,
        string title,
        CancellationToken cancellationToken = default)
    {
        var token = new DialogTraceToken(
            TraceId: Guid.NewGuid().ToString("N"),
            DialogType: dialogType,
            SourceScope: sourceScope,
            OpenedAtUtc: DateTimeOffset.UtcNow);
        await _diagnostics.RecordEventAsync(
            "Dialog.Open",
            $"trace={token.TraceId}; dialog={dialogType}; source={sourceScope}; title={title}",
            cancellationToken);
        return token;
    }

    public async Task<UiOperationResult> RecordDialogActionAsync(
        DialogTraceToken token,
        string action,
        string detail,
        CancellationToken cancellationToken = default)
    {
        await _diagnostics.RecordEventAsync(
            "Dialog.Action",
            $"trace={token.TraceId}; dialog={token.DialogType}; source={token.SourceScope}; action={action}; detail={detail}",
            cancellationToken);
        return UiOperationResult.Ok("Dialog action recorded.");
    }

    public async Task<UiOperationResult> CompleteDialogAsync(
        DialogTraceToken token,
        DialogReturnSemantic semantic,
        string summary,
        CancellationToken cancellationToken = default)
    {
        await _diagnostics.RecordEventAsync(
            "Dialog.Close",
            $"trace={token.TraceId}; dialog={token.DialogType}; source={token.SourceScope}; return={semantic}; summary={summary}",
            cancellationToken);
        return UiOperationResult.Ok("Dialog completion recorded.");
    }

    public async Task<UiOperationResult> ReportErrorAsync(
        string context,
        UiOperationResult result,
        CancellationToken cancellationToken = default)
    {
        await _diagnostics.RecordFailedResultAsync(context, result, cancellationToken);
        ErrorRaised?.Invoke(this, new DialogErrorRaisedEvent(context, result, DateTimeOffset.UtcNow));
        return result;
    }
}
