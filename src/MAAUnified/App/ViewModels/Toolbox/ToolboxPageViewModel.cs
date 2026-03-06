using System.Collections.ObjectModel;
using System.Text.Json;
using System.Text.Json.Nodes;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;
using LegacyConfigurationKeys = MAAUnified.Compat.Constants.ConfigurationKeys;

namespace MAAUnified.App.ViewModels.Toolbox;

public sealed class ToolboxPageViewModel : PageViewModelBase
{
    private const int MaxHistoryCount = 30;
    private const string ToolboxExecutionHistoryKey = "Toolbox.ExecutionHistory";
    private const string ToolboxHistoryLoadScope = "Toolbox.History.Load";
    private const string ToolboxHistorySaveScope = "Toolbox.History.Save";
    private const string ToolboxLegacyResultScope = "Toolbox.LegacyResult";
    private const string DisclaimerRequiredMessage = "请先确认免责声明。";
    private const string MiniGameSecretFrontTaskName = "MiniGame@SecretFront";

    private static readonly IReadOnlyDictionary<int, ToolboxToolKind> ToolByTabIndex = new Dictionary<int, ToolboxToolKind>
    {
        [0] = ToolboxToolKind.Recruit,
        [1] = ToolboxToolKind.OperBox,
        [2] = ToolboxToolKind.Depot,
        [3] = ToolboxToolKind.Gacha,
        [4] = ToolboxToolKind.VideoRecognition,
        [5] = ToolboxToolKind.MiniGame,
    };

    private static readonly IReadOnlySet<string> OperBoxModes = new HashSet<string>(StringComparer.Ordinal)
    {
        "owned",
        "all",
    };

    private static readonly IReadOnlySet<string> DepotFormats = new HashSet<string>(StringComparer.Ordinal)
    {
        "summary",
        "json",
    };

    private static readonly IReadOnlySet<string> MiniGameSecretFrontEndings = new HashSet<string>(StringComparer.Ordinal)
    {
        "A",
        "B",
        "C",
        "D",
        "E",
    };

    private int _selectedTabIndex;
    private string _resultText = "等待执行工具。";
    private bool _disclaimerAccepted;
    private string _currentToolParameters = string.Empty;
    private ToolboxExecutionState _executionState;
    private string _lastExecutionErrorCode = string.Empty;
    private DateTimeOffset? _lastExecutionAt;

    private string _recruitLevel3TimeInput = "540";
    private string _recruitLevel4TimeInput = "540";
    private string _recruitLevel5TimeInput = "540";
    private bool _recruitAutoSetTime = true;

    private string _operBoxMode = "owned";

    private string _depotFormat = "summary";
    private string _depotTopNInput = "50";

    private string _gachaDrawCountInput = "10";
    private bool _gachaShowDisclaimerNoMore;

    private string _videoRecognitionTargetFpsInput = "20";

    private string _miniGameTaskName = "SS@Store@Begin";
    private string _miniGameSecretFrontEnding = "A";
    private string _miniGameSecretFrontEvent = string.Empty;

    public ToolboxPageViewModel(MAAUnifiedRuntime runtime)
        : base(runtime)
    {
        Tabs =
        [
            "招募识别",
            "干员识别",
            "仓库识别",
            "抽卡",
            "窥屏",
            "小游戏",
        ];
        ExecutionHistory = new ObservableCollection<ToolExecutionRecord>();

        OperBoxModeOptions =
        [
            "owned",
            "all",
        ];
        DepotFormatOptions =
        [
            "summary",
            "json",
        ];
        GachaDrawCountOptions =
        [
            "1",
            "10",
        ];
        MiniGameTaskNameOptions =
        [
            "SS@Store@Begin",
            MiniGameSecretFrontTaskName,
        ];
        MiniGameSecretFrontEndingOptions =
        [
            "A",
            "B",
            "C",
            "D",
            "E",
        ];

        RefreshCurrentToolParametersPreview();
    }

    public IReadOnlyList<string> Tabs { get; }

    public ObservableCollection<ToolExecutionRecord> ExecutionHistory { get; }

    public IReadOnlyList<string> OperBoxModeOptions { get; }

    public IReadOnlyList<string> DepotFormatOptions { get; }

    public IReadOnlyList<string> GachaDrawCountOptions { get; }

    public IReadOnlyList<string> MiniGameTaskNameOptions { get; }

    public IReadOnlyList<string> MiniGameSecretFrontEndingOptions { get; }

    public int SelectedTabIndex
    {
        get => _selectedTabIndex;
        set
        {
            var normalized = Math.Clamp(value, 0, Tabs.Count - 1);
            if (SetProperty(ref _selectedTabIndex, normalized))
            {
                RefreshCurrentToolParametersPreview();
            }
        }
    }

    public string ResultText
    {
        get => _resultText;
        set => SetProperty(ref _resultText, value);
    }

    public bool DisclaimerAccepted
    {
        get => _disclaimerAccepted;
        set => SetProperty(ref _disclaimerAccepted, value);
    }

    public ToolboxExecutionState ExecutionState
    {
        get => _executionState;
        private set
        {
            if (SetProperty(ref _executionState, value))
            {
                OnPropertyChanged(nameof(IsExecuting));
            }
        }
    }

    public string CurrentToolParameters
    {
        get => _currentToolParameters;
        set => SetProperty(ref _currentToolParameters, value ?? string.Empty);
    }

    public bool IsExecuting => ExecutionState == ToolboxExecutionState.Executing;

    public string LastExecutionErrorCode
    {
        get => _lastExecutionErrorCode;
        private set => SetProperty(ref _lastExecutionErrorCode, value ?? string.Empty);
    }

    public DateTimeOffset? LastExecutionAt
    {
        get => _lastExecutionAt;
        private set => SetProperty(ref _lastExecutionAt, value);
    }

    public string RecruitLevel3TimeInput
    {
        get => _recruitLevel3TimeInput;
        set => SetParameterInput(ref _recruitLevel3TimeInput, value);
    }

    public string RecruitLevel4TimeInput
    {
        get => _recruitLevel4TimeInput;
        set => SetParameterInput(ref _recruitLevel4TimeInput, value);
    }

    public string RecruitLevel5TimeInput
    {
        get => _recruitLevel5TimeInput;
        set => SetParameterInput(ref _recruitLevel5TimeInput, value);
    }

    public bool RecruitAutoSetTime
    {
        get => _recruitAutoSetTime;
        set => SetParameterInput(ref _recruitAutoSetTime, value);
    }

    public string OperBoxMode
    {
        get => _operBoxMode;
        set => SetParameterInput(ref _operBoxMode, value);
    }

    public string DepotFormat
    {
        get => _depotFormat;
        set => SetParameterInput(ref _depotFormat, value);
    }

    public string DepotTopNInput
    {
        get => _depotTopNInput;
        set => SetParameterInput(ref _depotTopNInput, value);
    }

    public string GachaDrawCountInput
    {
        get => _gachaDrawCountInput;
        set => SetParameterInput(ref _gachaDrawCountInput, value);
    }

    public bool GachaShowDisclaimerNoMore
    {
        get => _gachaShowDisclaimerNoMore;
        set => SetParameterInput(ref _gachaShowDisclaimerNoMore, value);
    }

    public string VideoRecognitionTargetFpsInput
    {
        get => _videoRecognitionTargetFpsInput;
        set => SetParameterInput(ref _videoRecognitionTargetFpsInput, value);
    }

    public string MiniGameTaskName
    {
        get => _miniGameTaskName;
        set
        {
            if (SetParameterInput(ref _miniGameTaskName, value))
            {
                OnPropertyChanged(nameof(IsMiniGameSecretFront));
            }
        }
    }

    public string MiniGameSecretFrontEnding
    {
        get => _miniGameSecretFrontEnding;
        set => SetParameterInput(ref _miniGameSecretFrontEnding, value);
    }

    public string MiniGameSecretFrontEvent
    {
        get => _miniGameSecretFrontEvent;
        set => SetParameterInput(ref _miniGameSecretFrontEvent, value);
    }

    public bool IsMiniGameSecretFront => string.Equals(MiniGameTaskName, MiniGameSecretFrontTaskName, StringComparison.Ordinal);

    public async Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        LoadBridgeSettings();
        await LoadExecutionHistoryAsync(cancellationToken);
        RefreshCurrentToolParametersPreview();
        await Runtime.DiagnosticsService.RecordEventAsync("Toolbox", "Toolbox page initialized.", cancellationToken);
    }

    public void ApplySuccessPresetForCurrentTool()
    {
        if (!TryResolveTool(SelectedTabIndex, out var tool))
        {
            return;
        }

        switch (tool)
        {
            case ToolboxToolKind.Recruit:
                RecruitLevel3TimeInput = "540";
                RecruitLevel4TimeInput = "540";
                RecruitLevel5TimeInput = "540";
                RecruitAutoSetTime = true;
                break;
            case ToolboxToolKind.OperBox:
                OperBoxMode = "owned";
                break;
            case ToolboxToolKind.Depot:
                DepotFormat = "summary";
                DepotTopNInput = "50";
                break;
            case ToolboxToolKind.Gacha:
                GachaDrawCountInput = "10";
                break;
            case ToolboxToolKind.VideoRecognition:
                VideoRecognitionTargetFpsInput = "20";
                break;
            case ToolboxToolKind.MiniGame:
                MiniGameTaskName = "SS@Store@Begin";
                MiniGameSecretFrontEnding = "A";
                MiniGameSecretFrontEvent = string.Empty;
                break;
            default:
                throw new ArgumentOutOfRangeException(nameof(tool), tool, "Unsupported toolbox tool kind.");
        }

        RefreshCurrentToolParametersPreview();
    }

    public void ApplyFailurePresetForCurrentTool()
    {
        if (!TryResolveTool(SelectedTabIndex, out var tool))
        {
            return;
        }

        switch (tool)
        {
            case ToolboxToolKind.Recruit:
                RecruitLevel3TimeInput = "55";
                break;
            case ToolboxToolKind.OperBox:
                OperBoxMode = "invalid";
                break;
            case ToolboxToolKind.Depot:
                DepotTopNInput = "0";
                break;
            case ToolboxToolKind.Gacha:
                GachaDrawCountInput = "3";
                break;
            case ToolboxToolKind.VideoRecognition:
                VideoRecognitionTargetFpsInput = "0";
                break;
            case ToolboxToolKind.MiniGame:
                MiniGameTaskName = MiniGameSecretFrontTaskName;
                MiniGameSecretFrontEnding = string.Empty;
                break;
            default:
                throw new ArgumentOutOfRangeException(nameof(tool), tool, "Unsupported toolbox tool kind.");
        }

        RefreshCurrentToolParametersPreview();
    }

    public async Task ExecuteCurrentToolAsync(CancellationToken cancellationToken = default)
    {
        if (!TryResolveTool(SelectedTabIndex, out var tool))
        {
            await ApplyFailureAsync(
                null,
                UiOperationResult<ToolboxExecuteResult>.Fail(
                    UiErrorCode.ToolNotSupported,
                    $"Tool tab index `{SelectedTabIndex}` is not supported."),
                "resolve",
                cancellationToken);
            return;
        }

        CurrentToolParameters = BuildCurrentParameterText(tool);

        if (!DisclaimerAccepted)
        {
            await ApplyFailureAsync(
                tool,
                UiOperationResult<ToolboxExecuteResult>.Fail(
                    UiErrorCode.ToolboxDisclaimerNotAccepted,
                    DisclaimerRequiredMessage),
                "disclaimer",
                cancellationToken);
            return;
        }

        if (IsExecuting)
        {
            return;
        }

        var validation = ValidateCurrentToolParameters(tool);
        if (!validation.Success)
        {
            await ApplyFailureAsync(
                tool,
                UiOperationResult<ToolboxExecuteResult>.Fail(
                    UiErrorCode.ToolboxInvalidParameters,
                    validation.Message),
                "validation",
                cancellationToken);
            return;
        }

        await PersistBridgeSettingsForToolAsync(tool, cancellationToken);

        TransitionToExecuting();
        try
        {
            var request = new ToolboxExecuteRequest(tool, CurrentToolParameters);
            var result = await Runtime.ToolboxFeatureService.ExecuteToolAsync(
                request,
                cancellationToken);

            if (!result.Success || result.Value is null)
            {
                await ApplyFailureAsync(tool, result, "service", cancellationToken);
                return;
            }

            await ApplySuccessAsync(tool, result, result.Value, cancellationToken);
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            await ApplyFailureAsync(
                tool,
                UiOperationResult<ToolboxExecuteResult>.Fail(
                    UiErrorCode.ToolboxExecutionCancelled,
                    "Tool execution cancelled by caller."),
                "cancelled",
                CancellationToken.None);
        }
        catch (Exception ex)
        {
            await ApplyFailureAsync(
                tool,
                UiOperationResult<ToolboxExecuteResult>.Fail(
                    UiErrorCode.ToolboxExecutionFailed,
                    $"Tool execution failed: {ex.Message}",
                    ex.ToString()),
                "exception",
                CancellationToken.None);
        }
        finally
        {
            if (ExecutionState == ToolboxExecutionState.Executing)
            {
                ExecutionState = ToolboxExecutionState.Failed;
            }
        }
    }

    private async Task ApplySuccessAsync(
        ToolboxToolKind tool,
        UiOperationResult<ToolboxExecuteResult> result,
        ToolboxExecuteResult payload,
        CancellationToken cancellationToken)
    {
        ResultText = payload.ResultText;
        ExecutionState = ToolboxExecutionState.Succeeded;
        LastExecutionErrorCode = string.Empty;
        LastExecutionAt = payload.CompletedAt;
        await ApplyResultAsync(result, ScopeOf(tool), cancellationToken);
        ExecutionHistory.Insert(0, ToolExecutionRecord.Succeeded(
            ToolNameOf(tool),
            payload.ParameterSummary,
            BuildResultSummary(payload.ResultText)));
        TrimExecutionHistory();
        await PersistExecutionHistoryAsync(cancellationToken);
        await PersistLegacyToolResultAsync(tool, payload.ResultText, cancellationToken);
    }

    private async Task ApplyFailureAsync(
        ToolboxToolKind? tool,
        UiOperationResult<ToolboxExecuteResult> result,
        string stage,
        CancellationToken cancellationToken)
    {
        var errorCode = string.IsNullOrWhiteSpace(result.Error?.Code)
            ? UiErrorCode.ToolboxExecutionFailed
            : result.Error!.Code;
        var formatted = FormatFailureMessage(errorCode, result.Message);
        var details = MergeDetails(
            BuildFailureContextDetails(tool, errorCode, stage),
            result.Error?.Details);
        var normalized = UiOperationResult<ToolboxExecuteResult>.Fail(errorCode, formatted, details);
        _ = await ApplyResultAsync(normalized, ScopeOf(tool), cancellationToken);

        ResultText = formatted;
        LastErrorMessage = formatted;
        LastExecutionErrorCode = errorCode;
        LastExecutionAt = DateTimeOffset.Now;
        ExecutionState = ToolboxExecutionState.Failed;

        ExecutionHistory.Insert(0, ToolExecutionRecord.Failed(
            ToolNameOf(tool),
            BuildParameterSummary(CurrentToolParameters),
            BuildResultSummary(formatted),
            errorCode));
        TrimExecutionHistory();
        await PersistExecutionHistoryAsync(cancellationToken);
    }

    private void TransitionToExecuting()
    {
        ExecutionState = ToolboxExecutionState.Executing;
        LastExecutionErrorCode = string.Empty;
        LastExecutionAt = null;
    }

    private bool SetParameterInput<T>(ref T storage, T value)
    {
        if (!SetProperty(ref storage, value))
        {
            return false;
        }

        RefreshCurrentToolParametersPreview();
        return true;
    }

    private void RefreshCurrentToolParametersPreview()
    {
        if (!TryResolveTool(SelectedTabIndex, out var tool))
        {
            CurrentToolParameters = string.Empty;
            return;
        }

        CurrentToolParameters = BuildCurrentParameterText(tool);
    }

    private UiOperationResult ValidateCurrentToolParameters(ToolboxToolKind tool)
    {
        switch (tool)
        {
            case ToolboxToolKind.Recruit:
            {
                if (!TryParseRecruitMinutes(RecruitLevel3TimeInput, out _))
                {
                    return UiOperationResult.Fail(UiErrorCode.ToolboxInvalidParameters, "招募三星时间必须是 60 到 540 之间的 10 分钟整数倍。");
                }

                if (!TryParseRecruitMinutes(RecruitLevel4TimeInput, out _))
                {
                    return UiOperationResult.Fail(UiErrorCode.ToolboxInvalidParameters, "招募四星时间必须是 60 到 540 之间的 10 分钟整数倍。");
                }

                if (!TryParseRecruitMinutes(RecruitLevel5TimeInput, out _))
                {
                    return UiOperationResult.Fail(UiErrorCode.ToolboxInvalidParameters, "招募五星时间必须是 60 到 540 之间的 10 分钟整数倍。");
                }

                return UiOperationResult.Ok("Recruit parameters validated.");
            }
            case ToolboxToolKind.OperBox:
            {
                var mode = NormalizeToken(OperBoxMode);
                return OperBoxModes.Contains(mode)
                    ? UiOperationResult.Ok("OperBox parameters validated.")
                    : UiOperationResult.Fail(UiErrorCode.ToolboxInvalidParameters, "干员识别模式仅支持 owned 或 all。");
            }
            case ToolboxToolKind.Depot:
            {
                var format = NormalizeToken(DepotFormat);
                if (!DepotFormats.Contains(format))
                {
                    return UiOperationResult.Fail(UiErrorCode.ToolboxInvalidParameters, "仓库结果格式仅支持 summary 或 json。");
                }

                if (!TryParseInt(DepotTopNInput, 1, 500, out _))
                {
                    return UiOperationResult.Fail(UiErrorCode.ToolboxInvalidParameters, "仓库 TopN 必须在 1 到 500 之间。");
                }

                return UiOperationResult.Ok("Depot parameters validated.");
            }
            case ToolboxToolKind.Gacha:
            {
                if (!TryParseInt(GachaDrawCountInput, 1, 10, out var drawCount) || (drawCount != 1 && drawCount != 10))
                {
                    return UiOperationResult.Fail(UiErrorCode.ToolboxInvalidParameters, "抽卡次数仅支持 1 或 10。");
                }

                return UiOperationResult.Ok("Gacha parameters validated.");
            }
            case ToolboxToolKind.VideoRecognition:
            {
                if (!TryParseInt(VideoRecognitionTargetFpsInput, 1, 60, out _))
                {
                    return UiOperationResult.Fail(UiErrorCode.ToolboxInvalidParameters, "窥屏目标帧率必须在 1 到 60 之间。");
                }

                return UiOperationResult.Ok("VideoRecognition parameters validated.");
            }
            case ToolboxToolKind.MiniGame:
            {
                var taskName = NormalizeToken(MiniGameTaskName);
                if (string.IsNullOrWhiteSpace(taskName))
                {
                    return UiOperationResult.Fail(UiErrorCode.ToolboxInvalidParameters, "小游戏任务名不能为空。");
                }

                if (string.Equals(taskName, MiniGameSecretFrontTaskName, StringComparison.Ordinal)
                    && !MiniGameSecretFrontEndings.Contains(NormalizeToken(MiniGameSecretFrontEnding)))
                {
                    return UiOperationResult.Fail(UiErrorCode.ToolboxInvalidParameters, "当小游戏任务为 MiniGame@SecretFront 时，结局必须为 A~E 之一。");
                }

                return UiOperationResult.Ok("MiniGame parameters validated.");
            }
            default:
                return UiOperationResult.Fail(UiErrorCode.ToolNotSupported, $"Tool `{tool}` is not supported.");
        }
    }

    private async Task PersistBridgeSettingsForToolAsync(ToolboxToolKind tool, CancellationToken cancellationToken)
    {
        var updates = BuildBridgeUpdates(tool);
        if (updates.Count == 0)
        {
            return;
        }

        var result = await Runtime.SettingsFeatureService.SaveGlobalSettingsAsync(updates, cancellationToken);
        if (!result.Success)
        {
            var details = MergeDetails(
                JsonSerializer.Serialize(new
                {
                    tool = tool.ToString(),
                    updateCount = updates.Count,
                    keys = updates.Keys.OrderBy(static key => key, StringComparer.Ordinal).ToArray(),
                }),
                result.Error?.Details);
            var failed = UiOperationResult.Fail(
                result.Error?.Code ?? UiErrorCode.SettingsSaveFailed,
                result.Message,
                details);
            await RecordFailedResultAsync("Toolbox.ConfigBridge", failed, cancellationToken);
            return;
        }

        await RecordEventAsync(
            "Toolbox.ConfigBridge",
            $"Saved {updates.Count} toolbox bridge setting(s) for `{tool}`.",
            cancellationToken);
    }

    private IReadOnlyDictionary<string, string> BuildBridgeUpdates(ToolboxToolKind tool)
    {
        var updates = new Dictionary<string, string>(StringComparer.Ordinal);
        switch (tool)
        {
            case ToolboxToolKind.Recruit:
                updates[LegacyConfigurationKeys.ToolBoxChooseLevel3Time] = NormalizeToken(RecruitLevel3TimeInput);
                updates[LegacyConfigurationKeys.ToolBoxChooseLevel4Time] = NormalizeToken(RecruitLevel4TimeInput);
                updates[LegacyConfigurationKeys.ToolBoxChooseLevel5Time] = NormalizeToken(RecruitLevel5TimeInput);
                break;
            case ToolboxToolKind.Gacha:
                updates[LegacyConfigurationKeys.GachaShowDisclaimerNoMore] = GachaShowDisclaimerNoMore.ToString();
                break;
            case ToolboxToolKind.VideoRecognition:
                updates[LegacyConfigurationKeys.PeepTargetFps] = NormalizeToken(VideoRecognitionTargetFpsInput);
                break;
            case ToolboxToolKind.MiniGame:
                updates[LegacyConfigurationKeys.MiniGameTaskName] = NormalizeToken(MiniGameTaskName);
                updates[LegacyConfigurationKeys.MiniGameSecretFrontEnding] = NormalizeToken(MiniGameSecretFrontEnding);
                updates[LegacyConfigurationKeys.MiniGameSecretFrontEvent] = NormalizeToken(MiniGameSecretFrontEvent);
                break;
        }

        return updates;
    }

    private async Task LoadExecutionHistoryAsync(CancellationToken cancellationToken)
    {
        ExecutionHistory.Clear();
        if (!TryReadPersistedHistoryPayload(out var payload))
        {
            return;
        }

        if (!TryDeserializeExecutionHistory(payload, out var history, out var warning))
        {
            var failed = UiOperationResult.Fail(
                UiErrorCode.ToolboxExecutionFailed,
                warning,
                BuildHistoryContextDetails("load"));
            await RecordFailedResultAsync(ToolboxHistoryLoadScope, failed, cancellationToken);
            return;
        }

        foreach (var record in history)
        {
            ExecutionHistory.Add(record);
        }

        TrimExecutionHistory();
        await RecordEventAsync(
            ToolboxHistoryLoadScope,
            $"Loaded {ExecutionHistory.Count} toolbox execution history entrie(s).",
            cancellationToken);
    }

    private bool TryReadPersistedHistoryPayload(out string payload)
    {
        payload = string.Empty;
        if (!Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue(ToolboxExecutionHistoryKey, out var node)
            || node is null)
        {
            return false;
        }

        if (node is JsonValue jsonValue && jsonValue.TryGetValue(out string? raw))
        {
            payload = raw ?? string.Empty;
            return !string.IsNullOrWhiteSpace(payload);
        }

        if (node is JsonArray || node is JsonObject)
        {
            payload = node.ToJsonString();
            return !string.IsNullOrWhiteSpace(payload);
        }

        return false;
    }

    private static bool TryDeserializeExecutionHistory(
        string payload,
        out List<ToolExecutionRecord> history,
        out string warning)
    {
        history = [];
        warning = string.Empty;
        try
        {
            var entries = JsonSerializer.Deserialize<List<PersistedToolExecutionRecord>>(payload);
            if (entries is null)
            {
                warning = "读取执行历史失败：配置为空。";
                return false;
            }

            if (entries.Count == 0)
            {
                return true;
            }

            foreach (var entry in entries)
            {
                if (entry is null || string.IsNullOrWhiteSpace(entry.ToolName))
                {
                    continue;
                }

                history.Add(new ToolExecutionRecord(
                    entry.ExecutedAt,
                    entry.ToolName.Trim(),
                    BuildParameterSummary(entry.ParameterSummary),
                    entry.Success,
                    BuildResultSummary(entry.ResultSummary),
                    NormalizeToken(entry.ErrorCode)));
            }
        }
        catch (Exception ex)
        {
            warning = $"读取执行历史失败：配置不是合法 JSON。{ex.Message}";
            return false;
        }

        if (history.Count == 0)
        {
            warning = "读取执行历史失败：未找到有效记录。";
            return false;
        }

        history = history
            .OrderByDescending(static record => record.ExecutedAt)
            .Take(MaxHistoryCount)
            .ToList();
        return true;
    }

    private async Task PersistExecutionHistoryAsync(CancellationToken cancellationToken)
    {
        var payload = SerializeExecutionHistoryPayload();
        var result = await Runtime.SettingsFeatureService.SaveGlobalSettingAsync(
            ToolboxExecutionHistoryKey,
            payload,
            cancellationToken);
        if (!result.Success)
        {
            var failed = UiOperationResult.Fail(
                result.Error?.Code ?? UiErrorCode.SettingsSaveFailed,
                result.Message,
                MergeDetails(BuildHistoryContextDetails("save"), result.Error?.Details));
            await RecordFailedResultAsync(ToolboxHistorySaveScope, failed, cancellationToken);
            return;
        }

        await RecordEventAsync(
            ToolboxHistorySaveScope,
            $"Persisted {ExecutionHistory.Count} toolbox execution history entrie(s).",
            cancellationToken);
    }

    private string SerializeExecutionHistoryPayload()
    {
        var entries = ExecutionHistory
            .Take(MaxHistoryCount)
            .Select(static record => new PersistedToolExecutionRecord(
                record.ExecutedAt,
                record.ToolName,
                record.ParameterSummary,
                record.Success,
                record.ResultSummary,
                record.ErrorCode))
            .ToArray();
        return JsonSerializer.Serialize(entries);
    }

    private async Task PersistLegacyToolResultAsync(
        ToolboxToolKind tool,
        string resultText,
        CancellationToken cancellationToken)
    {
        var key = ResolveLegacyResultKey(tool);
        if (key is null)
        {
            return;
        }

        var result = await Runtime.SettingsFeatureService.SaveGlobalSettingAsync(
            key,
            resultText ?? string.Empty,
            cancellationToken);
        if (!result.Success)
        {
            var failed = UiOperationResult.Fail(
                result.Error?.Code ?? UiErrorCode.SettingsSaveFailed,
                result.Message,
                MergeDetails(
                    JsonSerializer.Serialize(new
                    {
                        tool = tool.ToString(),
                        key,
                        resultSummary = BuildResultSummary(resultText),
                    }),
                    result.Error?.Details));
            await RecordFailedResultAsync(ToolboxLegacyResultScope, failed, cancellationToken);
        }
    }

    private static string? ResolveLegacyResultKey(ToolboxToolKind tool)
    {
        return tool switch
        {
            ToolboxToolKind.OperBox => LegacyConfigurationKeys.OperBoxData,
            ToolboxToolKind.Depot => LegacyConfigurationKeys.DepotResult,
            _ => null,
        };
    }

    private string BuildFailureContextDetails(ToolboxToolKind? tool, string errorCode, string stage)
    {
        return JsonSerializer.Serialize(new
        {
            tool = ToolNameOf(tool),
            selectedTabIndex = SelectedTabIndex,
            executionState = ExecutionState.ToString(),
            parameterSummary = BuildParameterSummary(CurrentToolParameters),
            rawParameterLength = CurrentToolParameters.Length,
            errorCode,
            stage,
            occurredAt = DateTimeOffset.Now,
        });
    }

    private string BuildHistoryContextDetails(string stage)
    {
        return JsonSerializer.Serialize(new
        {
            stage,
            count = ExecutionHistory.Count,
            max = MaxHistoryCount,
            latestTool = ExecutionHistory.FirstOrDefault()?.ToolName,
            latestAt = ExecutionHistory.FirstOrDefault()?.ExecutedAt,
        });
    }

    private string BuildCurrentParameterText(ToolboxToolKind tool)
    {
        return tool switch
        {
            ToolboxToolKind.Recruit => string.Join(
                ';',
                [
                    $"autoSetTime={RecruitAutoSetTime.ToString().ToLowerInvariant()}",
                    $"level3Time={NormalizeToken(RecruitLevel3TimeInput)}",
                    $"level4Time={NormalizeToken(RecruitLevel4TimeInput)}",
                    $"level5Time={NormalizeToken(RecruitLevel5TimeInput)}",
                ]),
            ToolboxToolKind.OperBox => $"mode={NormalizeToken(OperBoxMode)}",
            ToolboxToolKind.Depot => string.Join(
                ';',
                [
                    $"format={NormalizeToken(DepotFormat)}",
                    $"topN={NormalizeToken(DepotTopNInput)}",
                ]),
            ToolboxToolKind.Gacha => string.Join(
                ';',
                [
                    $"drawCount={NormalizeToken(GachaDrawCountInput)}",
                    $"showDisclaimerNoMore={GachaShowDisclaimerNoMore.ToString().ToLowerInvariant()}",
                ]),
            ToolboxToolKind.VideoRecognition => $"targetFps={NormalizeToken(VideoRecognitionTargetFpsInput)}",
            ToolboxToolKind.MiniGame => string.Join(
                ';',
                [
                    $"taskName={NormalizeToken(MiniGameTaskName)}",
                    $"secretFrontEnding={NormalizeToken(MiniGameSecretFrontEnding)}",
                    $"secretFrontEvent={NormalizeToken(MiniGameSecretFrontEvent)}",
                ]),
            _ => string.Empty,
        };
    }

    private void LoadBridgeSettings()
    {
        RecruitLevel3TimeInput = ReadIntSetting(LegacyConfigurationKeys.ToolBoxChooseLevel3Time, 540).ToString();
        RecruitLevel4TimeInput = ReadIntSetting(LegacyConfigurationKeys.ToolBoxChooseLevel4Time, 540).ToString();
        RecruitLevel5TimeInput = ReadIntSetting(LegacyConfigurationKeys.ToolBoxChooseLevel5Time, 540).ToString();

        GachaShowDisclaimerNoMore = ReadBoolSetting(LegacyConfigurationKeys.GachaShowDisclaimerNoMore, false);
        VideoRecognitionTargetFpsInput = ReadIntSetting(LegacyConfigurationKeys.PeepTargetFps, 20).ToString();

        MiniGameTaskName = ReadStringSetting(LegacyConfigurationKeys.MiniGameTaskName, "SS@Store@Begin");
        MiniGameSecretFrontEnding = ReadStringSetting(LegacyConfigurationKeys.MiniGameSecretFrontEnding, "A");
        MiniGameSecretFrontEvent = ReadStringSetting(LegacyConfigurationKeys.MiniGameSecretFrontEvent, string.Empty);
    }

    private int ReadIntSetting(string key, int fallback)
    {
        if (!Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue(key, out JsonNode? node) || node is null)
        {
            return fallback;
        }

        var normalized = NormalizeToken(node.ToString());
        return int.TryParse(normalized, out var parsed)
            ? parsed
            : fallback;
    }

    private bool ReadBoolSetting(string key, bool fallback)
    {
        if (!Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue(key, out JsonNode? node) || node is null)
        {
            return fallback;
        }

        var normalized = NormalizeToken(node.ToString());
        if (bool.TryParse(normalized, out var parsed))
        {
            return parsed;
        }

        if (int.TryParse(normalized, out var number))
        {
            return number != 0;
        }

        return fallback;
    }

    private string ReadStringSetting(string key, string fallback)
    {
        if (!Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue(key, out JsonNode? node) || node is null)
        {
            return fallback;
        }

        var normalized = NormalizeToken(node.ToString());
        return string.IsNullOrWhiteSpace(normalized)
            ? fallback
            : normalized;
    }

    private static bool TryResolveTool(int tabIndex, out ToolboxToolKind tool)
    {
        return ToolByTabIndex.TryGetValue(tabIndex, out tool);
    }

    private static string ToolNameOf(ToolboxToolKind? tool)
    {
        return tool?.ToString() ?? "Unknown";
    }

    private static string ScopeOf(ToolboxToolKind? tool)
    {
        return tool is null ? "Toolbox.Unknown" : $"Toolbox.{tool}";
    }

    private void TrimExecutionHistory()
    {
        while (ExecutionHistory.Count > MaxHistoryCount)
        {
            ExecutionHistory.RemoveAt(ExecutionHistory.Count - 1);
        }
    }

    private static bool TryParseRecruitMinutes(string value, out int parsed)
    {
        if (!TryParseInt(value, 60, 540, out parsed))
        {
            return false;
        }

        return parsed % 10 == 0;
    }

    private static bool TryParseInt(string text, int min, int max, out int value)
    {
        value = 0;
        if (!int.TryParse(NormalizeToken(text), out var parsed))
        {
            return false;
        }

        if (parsed < min || parsed > max)
        {
            return false;
        }

        value = parsed;
        return true;
    }

    private static string NormalizeToken(string? text)
    {
        return string.IsNullOrWhiteSpace(text)
            ? string.Empty
            : text.Trim();
    }

    private static string BuildParameterSummary(string? text)
    {
        return BuildTextSummary(text, 180);
    }

    private static string BuildResultSummary(string? text)
    {
        return BuildTextSummary(text, 240);
    }

    private static string BuildTextSummary(string? text, int maxLength)
    {
        if (string.IsNullOrWhiteSpace(text))
        {
            return "none";
        }

        var normalized = text.Trim()
            .Replace("\r\n", "\n", StringComparison.Ordinal)
            .Replace('\n', ';')
            .Replace('\t', ' ');
        return normalized.Length <= maxLength
            ? normalized
            : normalized[..(maxLength - 3)] + "...";
    }

    private static string MergeDetails(string context, string? details)
    {
        if (string.IsNullOrWhiteSpace(details))
        {
            return context;
        }

        return $"{context} | {details}";
    }

    private static string FormatFailureMessage(string code, string message)
    {
        var fallback = string.IsNullOrWhiteSpace(message) ? "工具执行失败。" : message.Trim();
        return code switch
        {
            UiErrorCode.ToolboxDisclaimerNotAccepted => $"{DisclaimerRequiredMessage} ({UiErrorCode.ToolboxDisclaimerNotAccepted})",
            UiErrorCode.ToolboxInvalidParameters => $"工具参数错误：{fallback} ({UiErrorCode.ToolboxInvalidParameters})",
            UiErrorCode.ToolboxExecutionTimedOut => $"工具执行超时：{fallback} ({UiErrorCode.ToolboxExecutionTimedOut})",
            UiErrorCode.ToolboxExecutionCancelled => $"工具执行已取消：{fallback} ({UiErrorCode.ToolboxExecutionCancelled})",
            UiErrorCode.ToolNotSupported => $"当前工具不受支持：{fallback} ({UiErrorCode.ToolNotSupported})",
            _ when string.IsNullOrWhiteSpace(code) => $"工具执行失败：{fallback}",
            _ => $"工具执行失败：{fallback} ({code})",
        };
    }
}

public sealed record ToolExecutionRecord(
    DateTimeOffset ExecutedAt,
    string ToolName,
    string ParameterSummary,
    bool Success,
    string ResultSummary,
    string ErrorCode)
{
    public static ToolExecutionRecord Succeeded(string toolName, string parameterSummary, string resultSummary)
        => new(DateTimeOffset.Now, toolName, parameterSummary, true, resultSummary, string.Empty);

    public static ToolExecutionRecord Failed(string toolName, string parameterSummary, string resultSummary, string errorCode)
        => new(DateTimeOffset.Now, toolName, parameterSummary, false, resultSummary, errorCode);
}

public sealed record PersistedToolExecutionRecord(
    DateTimeOffset ExecutedAt,
    string ToolName,
    string ParameterSummary,
    bool Success,
    string ResultSummary,
    string ErrorCode);

public enum ToolboxExecutionState
{
    Idle = 0,
    Executing = 1,
    Succeeded = 2,
    Failed = 3,
}
