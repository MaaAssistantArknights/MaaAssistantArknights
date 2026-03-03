using System.Text.Json.Nodes;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Application.Services.Features;

public interface IConnectFeatureService
{
    Task<CoreResult<bool>> ValidateAndConnectAsync(string address, string config, string? adbPath, CancellationToken cancellationToken = default);

    Task<UiOperationResult> ConnectAsync(string address, string config, string? adbPath, CancellationToken cancellationToken = default);

    Task<UiOperationResult> StartAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> StopAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> WaitAndStopAsync(TimeSpan wait, CancellationToken cancellationToken = default);

    Task<UiOperationResult<ImportReport>> ImportLegacyConfigAsync(ImportSource source, bool manualImport, CancellationToken cancellationToken = default);
}

public interface ITaskQueueFeatureService
{
    Task<CoreResult<int>> QueueEnabledTasksAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult<IReadOnlyList<UnifiedTaskItem>>> GetCurrentTaskQueueAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> AddTaskAsync(string type, string name, bool enabled = true, CancellationToken cancellationToken = default);

    Task<UiOperationResult> RenameTaskAsync(int index, string newName, CancellationToken cancellationToken = default);

    Task<UiOperationResult> RemoveTaskAsync(int index, CancellationToken cancellationToken = default);

    Task<UiOperationResult> MoveTaskAsync(int fromIndex, int toIndex, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SetTaskEnabledAsync(int index, bool? enabled, CancellationToken cancellationToken = default);

    Task<UiOperationResult<JsonObject>> GetTaskParamsAsync(int index, CancellationToken cancellationToken = default);

    Task<UiOperationResult> UpdateTaskParamsAsync(
        int index,
        JsonObject parameters,
        bool persistImmediately = false,
        CancellationToken cancellationToken = default);

    Task<UiOperationResult<StartUpTaskParamsDto>> GetStartUpParamsAsync(int index, CancellationToken cancellationToken = default);

    Task<UiOperationResult<FightTaskParamsDto>> GetFightParamsAsync(int index, CancellationToken cancellationToken = default);

    Task<UiOperationResult<RecruitTaskParamsDto>> GetRecruitParamsAsync(int index, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SaveStartUpParamsAsync(int index, StartUpTaskParamsDto dto, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SaveFightParamsAsync(int index, FightTaskParamsDto dto, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SaveRecruitParamsAsync(int index, RecruitTaskParamsDto dto, CancellationToken cancellationToken = default);

    Task<UiOperationResult<IReadOnlyList<TaskValidationIssue>>> ValidateTaskAsync(int index, CancellationToken cancellationToken = default);

    Task<UiOperationResult> FlushTaskParamWritesAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> SaveAsync(CancellationToken cancellationToken = default);
}

public interface ICopilotFeatureService
{
    Task<string> ImportCopilotAsync(string source, CancellationToken cancellationToken = default);

    Task<UiOperationResult> ImportFromFileAsync(string filePath, CancellationToken cancellationToken = default);

    Task<UiOperationResult> ImportFromClipboardAsync(string payload, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SubmitFeedbackAsync(string copilotId, bool like, CancellationToken cancellationToken = default);
}

public interface IToolboxFeatureService
{
    Task<string> RunToolAsync(string toolName, CancellationToken cancellationToken = default);

    Task<UiOperationResult<string>> ExecuteToolAsync(string toolName, CancellationToken cancellationToken = default);
}

public interface IRemoteControlFeatureService
{
    Task<CoreResult<bool>> StartRemotePollingAsync(CancellationToken cancellationToken = default);
}

public interface IOverlayFeatureService
{
    Task<string> GetOverlayModeAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult<IReadOnlyList<OverlayTarget>>> GetOverlayTargetsAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> SelectOverlayTargetAsync(string targetId, CancellationToken cancellationToken = default);

    Task<UiOperationResult> ToggleOverlayVisibilityAsync(bool visible, CancellationToken cancellationToken = default);
}

public interface INotificationProviderFeatureService
{
    Task<string[]> GetAvailableProvidersAsync(CancellationToken cancellationToken = default);
}

public interface ISettingsFeatureService
{
    Task<UiOperationResult> SaveGlobalSettingAsync(string key, string value, CancellationToken cancellationToken = default);

    Task<UiOperationResult> TestNotificationAsync(string title, string message, CancellationToken cancellationToken = default);

    Task<UiOperationResult> RegisterHotkeyAsync(string name, string gesture, CancellationToken cancellationToken = default);

    Task<UiOperationResult<bool>> GetAutostartStatusAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> SetAutostartAsync(bool enabled, CancellationToken cancellationToken = default);

    Task<UiOperationResult<string>> BuildIssueReportAsync(CancellationToken cancellationToken = default);
}

public interface IPlatformCapabilityService
{
    event EventHandler<TrayCommandEvent>? TrayCommandInvoked;

    event EventHandler<GlobalHotkeyTriggeredEvent>? GlobalHotkeyTriggered;

    Task<UiOperationResult<PlatformCapabilitySnapshot>> GetSnapshotAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> InitializeTrayAsync(string appTitle, TrayMenuText? menuText, CancellationToken cancellationToken = default);

    Task<UiOperationResult> InitializeTrayAsync(string appTitle, CancellationToken cancellationToken = default)
        => InitializeTrayAsync(appTitle, null, cancellationToken);

    Task<UiOperationResult> ShutdownTrayAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> ShowTrayMessageAsync(string title, string message, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SetTrayVisibleAsync(bool visible, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SetTrayMenuStateAsync(TrayMenuState state, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SendSystemNotificationAsync(string title, string message, CancellationToken cancellationToken = default);

    Task<UiOperationResult> RegisterGlobalHotkeyAsync(string name, string gesture, CancellationToken cancellationToken = default);

    Task<UiOperationResult> UnregisterGlobalHotkeyAsync(string name, CancellationToken cancellationToken = default);

    Task<UiOperationResult<bool>> GetAutostartEnabledAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> SetAutostartEnabledAsync(bool enabled, CancellationToken cancellationToken = default);

    Task<UiOperationResult> BindOverlayHostAsync(nint hostWindowHandle, bool clickThrough, double opacity, CancellationToken cancellationToken = default);

    Task<UiOperationResult<IReadOnlyList<OverlayTarget>>> QueryOverlayTargetsAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> SelectOverlayTargetAsync(string targetId, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SetOverlayVisibleAsync(bool visible, CancellationToken cancellationToken = default);
}

public interface IDialogFeatureService
{
    Task<string> PrepareDialogPayloadAsync(string dialogType, CancellationToken cancellationToken = default);

    Task<UiOperationResult> ReportErrorAsync(string context, string message, CancellationToken cancellationToken = default);
}

public interface IPostActionFeatureService
{
    Task<UiOperationResult<PostActionConfig>> LoadAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> SaveAsync(PostActionConfig config, CancellationToken cancellationToken = default);

    Task<UiOperationResult<PostActionPreview>> GetCapabilityPreviewAsync(PostActionConfig config, CancellationToken cancellationToken = default);

    Task<UiOperationResult<PostActionPreview>> ValidateSelectionAsync(PostActionConfig config, CancellationToken cancellationToken = default);

    Task<UiOperationResult> ExecuteAfterCompletionAsync(
        PostActionExecutionContext context,
        PostActionConfig? configOverride = null,
        CancellationToken cancellationToken = default);
}
