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

public interface IShellFeatureService
{
    Task<UiOperationResult> ConnectAsync(string address, string config, string? adbPath, CancellationToken cancellationToken = default);

    Task<UiOperationResult<ImportReport>> ImportLegacyConfigAsync(ImportSource source, bool manualImport, CancellationToken cancellationToken = default);

    Task<UiOperationResult<string>> SwitchLanguageAsync(
        string currentLanguage,
        string? targetLanguage = null,
        CancellationToken cancellationToken = default);

    IReadOnlyList<string> GetSupportedLanguages();
}

public interface ITaskQueueFeatureService
{
    Task<CoreResult<int>> QueueEnabledTasksAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult<IReadOnlyList<TaskQueuePrecheckWarning>>> GetStartPrecheckWarningsAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult<IReadOnlyList<TaskQueuePrecheckWarning>>> ApplyStartPrecheckDowngradesAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult<IReadOnlyList<UnifiedTaskItem>>> GetCurrentTaskQueueAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> AddTaskAsync(string type, string name, bool enabled = true, CancellationToken cancellationToken = default);

    Task<UiOperationResult> RenameTaskAsync(int index, string newName, CancellationToken cancellationToken = default);

    Task<UiOperationResult> RemoveTaskAsync(int index, CancellationToken cancellationToken = default);

    Task<UiOperationResult> MoveTaskAsync(int fromIndex, int toIndex, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SetTaskEnabledAsync(int index, bool? enabled, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SetAllTasksEnabledAsync(bool enabled, CancellationToken cancellationToken = default);

    Task<UiOperationResult> InvertTasksEnabledAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult<JsonObject>> GetTaskParamsAsync(int index, CancellationToken cancellationToken = default);

    Task<UiOperationResult> UpdateTaskParamsAsync(
        int index,
        JsonObject parameters,
        bool persistImmediately = false,
        CancellationToken cancellationToken = default);

    Task<UiOperationResult<StartUpTaskParamsDto>> GetStartUpParamsAsync(int index, CancellationToken cancellationToken = default);

    Task<UiOperationResult<FightTaskParamsDto>> GetFightParamsAsync(int index, CancellationToken cancellationToken = default);

    Task<UiOperationResult<RecruitTaskParamsDto>> GetRecruitParamsAsync(int index, CancellationToken cancellationToken = default);

    Task<UiOperationResult<RoguelikeTaskParamsDto>> GetRoguelikeParamsAsync(int index, CancellationToken cancellationToken = default);

    Task<UiOperationResult<ReclamationTaskParamsDto>> GetReclamationParamsAsync(int index, CancellationToken cancellationToken = default);

    Task<UiOperationResult<CustomTaskParamsDto>> GetCustomParamsAsync(int index, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SaveStartUpParamsAsync(int index, StartUpTaskParamsDto dto, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SaveFightParamsAsync(int index, FightTaskParamsDto dto, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SaveRecruitParamsAsync(int index, RecruitTaskParamsDto dto, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SaveRoguelikeParamsAsync(int index, RoguelikeTaskParamsDto dto, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SaveReclamationParamsAsync(int index, ReclamationTaskParamsDto dto, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SaveCustomParamsAsync(int index, CustomTaskParamsDto dto, CancellationToken cancellationToken = default);

    Task<UiOperationResult<TaskValidationReport>> ValidateTaskAsync(int index, CancellationToken cancellationToken = default);

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
    Task<UiOperationResult<ToolboxExecuteResult>> ExecuteToolAsync(
        ToolboxExecuteRequest request,
        CancellationToken cancellationToken = default);
}

public interface IRemoteControlFeatureService
{
    Task<CoreResult<bool>> StartRemotePollingAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult<RemoteControlConnectivityResult>> TestConnectivityAsync(
        RemoteControlConnectivityRequest request,
        CancellationToken cancellationToken = default);
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

    Task<UiOperationResult> ValidateProviderParametersAsync(
        NotificationProviderRequest request,
        CancellationToken cancellationToken = default);

    Task<UiOperationResult> SendTestAsync(
        NotificationProviderTestRequest request,
        CancellationToken cancellationToken = default);
}

public interface ISettingsFeatureService
{
    Task<UiOperationResult> SaveGlobalSettingAsync(string key, string value, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SaveGlobalSettingsAsync(
        IReadOnlyDictionary<string, string> updates,
        CancellationToken cancellationToken = default);

    Task<UiOperationResult> TestNotificationAsync(string title, string message, CancellationToken cancellationToken = default);

    Task<UiOperationResult> RegisterHotkeyAsync(string name, string gesture, CancellationToken cancellationToken = default);

    Task<UiOperationResult<bool>> GetAutostartStatusAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> SetAutostartAsync(bool enabled, CancellationToken cancellationToken = default);

    Task<UiOperationResult<string>> BuildIssueReportAsync(CancellationToken cancellationToken = default);
}

public interface IConfigurationProfileFeatureService
{
    Task<UiOperationResult<ConfigurationProfileState>> LoadStateAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult<ConfigurationProfileState>> AddProfileAsync(
        string profileName,
        string? copyFrom = null,
        CancellationToken cancellationToken = default);

    Task<UiOperationResult<ConfigurationProfileState>> DeleteProfileAsync(
        string profileName,
        CancellationToken cancellationToken = default);

    Task<UiOperationResult<ConfigurationProfileState>> MoveProfileAsync(
        string profileName,
        int offset,
        CancellationToken cancellationToken = default);

    Task<UiOperationResult<ConfigurationProfileState>> SwitchProfileAsync(
        string profileName,
        CancellationToken cancellationToken = default);
}

public interface IVersionUpdateFeatureService
{
    Task<UiOperationResult<VersionUpdatePolicy>> LoadPolicyAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> SaveChannelAsync(VersionUpdatePolicy policy, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SaveProxyAsync(VersionUpdatePolicy policy, CancellationToken cancellationToken = default);

    Task<UiOperationResult> SavePolicyAsync(VersionUpdatePolicy policy, CancellationToken cancellationToken = default);

    Task<UiOperationResult<string>> CheckForUpdatesAsync(
        VersionUpdatePolicy policy,
        CancellationToken cancellationToken = default);
}

public interface IAchievementFeatureService
{
    Task<UiOperationResult<AchievementPolicy>> LoadPolicyAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> SavePolicyAsync(AchievementPolicy policy, CancellationToken cancellationToken = default);
}

public interface IAnnouncementFeatureService
{
    Task<UiOperationResult<AnnouncementState>> LoadStateAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> SaveStateAsync(AnnouncementState state, CancellationToken cancellationToken = default);
}

public interface IStageManagerFeatureService
{
    Task<UiOperationResult<StageManagerConfig>> LoadConfigAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> SaveConfigAsync(StageManagerConfig config, CancellationToken cancellationToken = default);

    Task<UiOperationResult<IReadOnlyList<string>>> ValidateStageCodesAsync(
        string stageCodesText,
        CancellationToken cancellationToken = default);
}

public interface IWebApiFeatureService
{
    Task<UiOperationResult<WebApiConfig>> LoadConfigAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> SaveConfigAsync(WebApiConfig config, CancellationToken cancellationToken = default);

    Task<UiOperationResult<bool>> GetRunningStatusAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> StartAsync(CancellationToken cancellationToken = default);

    Task<UiOperationResult> StopAsync(CancellationToken cancellationToken = default);
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
    event EventHandler<DialogErrorRaisedEvent>? ErrorRaised;

    Task<string> PrepareDialogPayloadAsync(string dialogType, CancellationToken cancellationToken = default);

    Task<UiOperationResult> ReportErrorAsync(string context, string message, CancellationToken cancellationToken = default);

    Task<DialogTraceToken> BeginDialogAsync(
        DialogType dialogType,
        string sourceScope,
        string title,
        CancellationToken cancellationToken = default);

    Task<UiOperationResult> RecordDialogActionAsync(
        DialogTraceToken token,
        string action,
        string detail,
        CancellationToken cancellationToken = default);

    Task<UiOperationResult> CompleteDialogAsync(
        DialogTraceToken token,
        DialogReturnSemantic semantic,
        string summary,
        CancellationToken cancellationToken = default);

    Task<UiOperationResult> ReportErrorAsync(
        string context,
        UiOperationResult result,
        CancellationToken cancellationToken = default);
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
