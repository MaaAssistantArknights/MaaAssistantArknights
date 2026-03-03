using System.Text.Json.Nodes;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
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

    public Task<UiOperationResult<IReadOnlyList<TaskValidationIssue>>> ValidateTaskAsync(int index, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!TryGetTaskByIndex(index, out var task, out var error))
        {
            return Task.FromResult(UiOperationResult<IReadOnlyList<TaskValidationIssue>>.Fail("TaskNotFound", error));
        }

        if (!TryGetProfile(out var profile, out error))
        {
            return Task.FromResult(UiOperationResult<IReadOnlyList<TaskValidationIssue>>.Fail("ProfileMissing", error));
        }

        var compiled = TaskParamCompiler.CompileTask(task, profile, _configService.CurrentConfig, strict: true);
        var issues = compiled.Issues.ToList();
        var message = issues.Count == 0
            ? $"Task `{task.Name}` passed validation."
            : BuildIssueMessage(issues);
        return Task.FromResult(UiOperationResult<IReadOnlyList<TaskValidationIssue>>.Ok(issues, message));
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
        var enabledFightTasks = profile.TaskQueue
            .Where(t => t.IsEnabled && string.Equals(TaskModuleTypes.Normalize(t.Type), TaskModuleTypes.Fight, StringComparison.OrdinalIgnoreCase))
            .ToList();
        if (enabledFightTasks.Count == 0)
        {
            return;
        }

        var hasEmptyFightStage = enabledFightTasks.Any(t => !HasNonEmptyStage(t.Params));
        if (!hasEmptyFightStage)
        {
            return;
        }

        foreach (var mallTask in profile.TaskQueue.Where(t =>
                     t.IsEnabled && string.Equals(TaskModuleTypes.Normalize(t.Type), TaskModuleTypes.Mall, StringComparison.OrdinalIgnoreCase)))
        {
            var mallParams = mallTask.Params;
            if (!TryReadBool(mallParams, "credit_fight", out var enabledCreditFight) || !enabledCreditFight)
            {
                continue;
            }

            mallParams["credit_fight"] = false;
            _configService.LogService.Warn($"Mall credit fight disabled for `{mallTask.Name}` because enabled Fight task has empty stage.");
        }
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
               || string.Equals(type, TaskModuleTypes.Recruit, StringComparison.OrdinalIgnoreCase);
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

        return Task.FromResult(UiOperationResult.Ok($"Copilot file imported: {filePath}"));
    }

    public Task<UiOperationResult> ImportFromClipboardAsync(string payload, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (string.IsNullOrWhiteSpace(payload))
        {
            return Task.FromResult(UiOperationResult.Fail("CopilotClipboardEmpty", "Clipboard payload is empty."));
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

    public Task<UiOperationResult<string>> ExecuteToolAsync(string toolName, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!_supportedTools.Contains(toolName))
        {
            return Task.FromResult(UiOperationResult<string>.Fail("ToolNotSupported", $"Tool `{toolName}` is not supported."));
        }

        return Task.FromResult(UiOperationResult<string>.Ok(
            $"`{toolName}` execution started.",
            $"Tool `{toolName}` dispatched."));
    }
}

public sealed class RemoteControlFeatureService : IRemoteControlFeatureService
{
    public Task<CoreResult<bool>> StartRemotePollingAsync(CancellationToken cancellationToken = default)
    {
        return Task.FromResult(CoreResult<bool>.Ok(true));
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
    public Task<string[]> GetAvailableProvidersAsync(CancellationToken cancellationToken = default)
    {
        return Task.FromResult(new[]
        {
            "Smtp",
            "ServerChan",
            "Bark",
            "Discord",
            "Telegram",
            "Qmsg",
            "Gotify",
            "CustomWebhook",
        });
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

        _configService.CurrentConfig.GlobalValues[key] = JsonValue.Create(value);
        await _configService.SaveAsync(cancellationToken);
        await _diagnostics.RecordEventAsync("Settings", $"Saved setting: {key}", cancellationToken);
        return UiOperationResult.Ok($"Setting `{key}` updated.");
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
