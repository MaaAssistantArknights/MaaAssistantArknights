using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.RegularExpressions;
using Avalonia.Media.Imaging;
using Avalonia.Threading;
using MAAUnified.App.Features.Dialogs;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Localization;
using MAAUnified.Application.Services.TaskParams;
using MAAUnified.Compat.Constants;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class TaskQueuePageViewModel : PageViewModelBase
{
    private const string TaskQueueRunOwner = "TaskQueue";
    private const string TaskSelectedIndexConfigKey = "TaskSelectedIndex";
    private static readonly string[] WpfDefaultTaskOrder =
    [
        TaskModuleTypes.StartUp,
        TaskModuleTypes.Fight,
        TaskModuleTypes.Infrast,
        TaskModuleTypes.Recruit,
        TaskModuleTypes.Mall,
        TaskModuleTypes.Award,
        TaskModuleTypes.Roguelike,
        TaskModuleTypes.Reclamation,
    ];

    private static readonly string[] AddableTaskModules =
    [
        TaskModuleTypes.StartUp,
        TaskModuleTypes.Fight,
        TaskModuleTypes.Infrast,
        TaskModuleTypes.Recruit,
        TaskModuleTypes.Mall,
        TaskModuleTypes.Award,
        TaskModuleTypes.Roguelike,
        TaskModuleTypes.Reclamation,
        TaskModuleTypes.Custom,
    ];

    private static readonly IReadOnlyDictionary<string, string[]> LegacyTaskNameAliases =
        new Dictionary<string, string[]>(StringComparer.OrdinalIgnoreCase)
        {
            [TaskModuleTypes.StartUp] = ["StartUp", "Start Up", "WakeUp", "Wake Up"],
            [TaskModuleTypes.Fight] = ["Fight", "Combat"],
            [TaskModuleTypes.Infrast] = ["Infrast", "Infrastructure", "Base"],
            [TaskModuleTypes.Recruit] = ["Recruit", "Auto Recruit"],
            [TaskModuleTypes.Mall] = ["Mall", "Credit"],
            [TaskModuleTypes.Award] = ["Award", "Awards"],
            [TaskModuleTypes.Roguelike] = ["Roguelike", "IS"],
            [TaskModuleTypes.Reclamation] = ["Reclamation"],
            [TaskModuleTypes.Custom] = ["Custom", "Custom Task"],
            [TaskModuleTypes.PostAction] = ["PostAction", "Post Action", "After Completion"],
        };

    private const int MaxLogCards = 180;
    private const int MaxOverlayLogs = 200;
    private const string UiMallCreditFightLastTime = "_ui_mall_credit_fight_last_time";
    private const string UiMallVisitFriendsLastTime = "_ui_mall_visit_friends_last_time";
    private static readonly Regex LeadingLogTimestampPattern = new(
        "^(?:\\[?\\d{2}:\\d{2}:\\d{2}\\]?\\s+)+",
        RegexOptions.Compiled);

    private readonly SemaphoreSlim _taskBindingLock = new(1, 1);
    private readonly SemaphoreSlim _logThumbnailSemaphore = new(1, 1);
    private readonly SemaphoreSlim _queueMutationLock = new(1, 1);
    private readonly SemaphoreSlim _runTransitionLock = new(1, 1);
    private readonly object _pendingBindingGate = new();
    private readonly object _moduleAutoSaveGate = new();
    private readonly OverlaySharedState _overlaySharedState;
    private readonly ConnectionGameSharedStateViewModel _connectionGameSharedState;
    private readonly Action<LocalizationFallbackInfo>? _localizationFallbackReporter;
    private readonly IAppDialogService _dialogService;
    private readonly Action<string>? _navigateToSettingsSection;
    private readonly Func<CancellationToken, Task<bool>>? _ensureCoreReadyForExecutionAsync;
    private Task _pendingBindingTask = Task.CompletedTask;
    private CancellationTokenSource? _pendingBindingCts;
    private CancellationTokenSource? _moduleAutoSaveCts;
    private int _pendingBindingVersion;
    private bool _suppressTaskEnabledSync;
    private bool _suppressModuleAutoSave;
    private SessionState _currentSessionState;
    private bool _hasBlockingConfigIssues;
    private int _blockingConfigIssueCount;
    private bool _isCoreReady = true;
    private bool _isAdvancedSettingsSelected;
    private bool _isPostActionPanelSelected;
    private bool _isWaitingForStop;
    private SelectionBatchMode _selectionBatchMode = SelectionBatchMode.Clear;
    private bool _showBatchModeToggle;
    private bool _clearTaskStatusesWhenStopped;
    private string _dailyStageHint = string.Empty;
    private string _selectedTaskModule = TaskModuleTypes.StartUp;
    private TaskModuleOption? _selectedTaskModuleOption;
    private string _renameTargetName = string.Empty;
    private string _overlayStatusText = string.Empty;
    private OverlayTarget? _selectedOverlayTarget = new("preview", "Preview + Logs", true);
    private bool _overlayVisible;
    private OverlayRuntimeMode _overlayMode = OverlayRuntimeMode.Hidden;
    private string _currentRunId = "-";
    private string _lastPostActionRunId = string.Empty;
    private string _selectedTaskValidationSummary = string.Empty;
    private bool _selectedTaskHasBlockingValidationIssues;
    private int _selectedTaskValidationIssueCount;
    private string _coreInitializationMessage = string.Empty;
    private string _startPrecheckWarningMessage = string.Empty;
    private string _noTaskSelectedHint = "Select a task from the left list to edit its settings.";
    private TaskQueueLogEntryViewModel _downloadLogEntry = new(string.Empty, string.Empty, "INFO");
    private TaskRuntimeStatusSnapshot? _lastRuntimeStatus;
    private TaskQueueItemViewModel? _selectedTask;
    private bool _roguelikeInCombat;
    private DateTimeOffset? _runStartedAt;
    private DateTimeOffset? _fightSanityReportTime;
    private int? _fightSanityCurrent;
    private int? _fightSanityMax;
    private int? _fightSanityCost;
    private int? _fightSeries;
    private int? _fightTimesFinished;
    private int _medicineUsedTimes;
    private int _expiringMedicineUsedTimes;
    private int _stoneUsedTimes;

    public TaskQueuePageViewModel(
        MAAUnifiedRuntime runtime,
        ConnectionGameSharedStateViewModel connectionGameSharedState,
        Action<LocalizationFallbackInfo>? localizationFallbackReporter = null,
        IAppDialogService? dialogService = null,
        Action<string>? navigateToSettingsSection = null,
        Func<CancellationToken, Task<bool>>? ensureCoreReadyForExecutionAsync = null)
        : base(runtime)
    {
        _connectionGameSharedState = connectionGameSharedState;
        _localizationFallbackReporter = localizationFallbackReporter;
        _dialogService = dialogService ?? NoOpAppDialogService.Instance;
        _navigateToSettingsSection = navigateToSettingsSection;
        _ensureCoreReadyForExecutionAsync = ensureCoreReadyForExecutionAsync;
        TaskModules = new ObservableCollection<TaskModuleOption>();
        Tasks = new ObservableCollection<TaskQueueItemViewModel>();
        LogCards = new ObservableCollection<TaskQueueLogCardViewModel>();
        OverlayLogs = new ObservableCollection<TaskQueueLogEntryViewModel>();
        OverlayTargets = new ObservableCollection<OverlayTarget>();
        _overlaySharedState = OverlaySharedStateRegistry.Get(runtime);
        _overlayVisible = _overlaySharedState.Visible;
        _overlayMode = _overlaySharedState.Mode;

        Texts = new LocalizedTextMap
        {
            Language = ResolveLanguage(),
        };
        RootTexts = new RootLocalizationTextMap("Root.Localization.TaskQueue")
        {
            Language = ResolveLanguage(),
        };
        RootTexts.FallbackReported += info => _localizationFallbackReporter?.Invoke(info);
        _dailyStageHint = Texts.GetOrDefault("TaskQueue.DailyStageHintDefault", "Daily stage hints will be shown after resources are loaded.");
        _overlayStatusText = string.IsNullOrWhiteSpace(_overlaySharedState.StatusMessage)
            ? Texts.GetOrDefault("TaskQueue.OverlayDisconnected", "Overlay disconnected")
            : _overlaySharedState.StatusMessage;
        StartUpModule = new StartUpTaskModuleViewModel(
            runtime,
            Texts,
            _connectionGameSharedState,
            RunAccountSwitchManualAsync);
        FightModule = new FightTaskModuleViewModel(runtime, Texts);
        RecruitModule = new RecruitTaskModuleViewModel(runtime, Texts);
        InfrastModule = new InfrastModuleViewModel(runtime, Texts);
        MallModule = new MallModuleViewModel(runtime, Texts);
        AwardModule = new AwardModuleViewModel(runtime, Texts);
        RoguelikeModule = new RoguelikeModuleViewModel(runtime, Texts);
        ReclamationModule = new ReclamationModuleViewModel(runtime, Texts);
        CustomModule = new CustomModuleViewModel(runtime, Texts);
        PostActionModule = new PostActionModuleViewModel(runtime, Texts);
        ApplySettingsModeToTaskModules();
        StartUpModule.PropertyChanged += OnTypedModulePropertyChanged;
        FightModule.PropertyChanged += OnTypedModulePropertyChanged;
        RecruitModule.PropertyChanged += OnTypedModulePropertyChanged;
        RoguelikeModule.PropertyChanged += OnTypedModulePropertyChanged;
        ReclamationModule.PropertyChanged += OnTypedModulePropertyChanged;
        CustomModule.PropertyChanged += OnTypedModulePropertyChanged;
        PostActionModule.PropertyChanged += OnPostActionModulePropertyChanged;

        RebuildTaskModuleOptions();
        SelectedTaskModuleOption = TaskModules.FirstOrDefault();
        UpdatePostActionSummary();
        RefreshSelectionBatchModeFromConfig();

        runtime.LogService.LogReceived += log => Dispatcher.UIThread.Post(() => UpdateDownloadLog(log.Timestamp, log.Level, log.Message));

        runtime.SessionService.CallbackReceived += callback => _ = HandleCallbackAsync(callback);
        runtime.SessionService.SessionStateChanged += OnSessionStateChanged;
        runtime.ConfigurationService.ConfigChanged += _ =>
        {
            Dispatcher.UIThread.Post(() =>
            {
                SetLanguage(ResolveLanguage());
                RefreshSelectionBatchModeFromConfig();
                RefreshConfigValidationState(runtime.ConfigurationService.CurrentValidationIssues);
            });
        };
        _connectionGameSharedState.PropertyChanged += OnConnectionGameSharedStateChanged;
        _currentSessionState = runtime.SessionService.CurrentState;
        _overlaySharedState.PropertyChanged += OnOverlaySharedStateChanged;
        SyncOverlayPresentationFromSharedState();
    }

    public ObservableCollection<TaskModuleOption> TaskModules { get; }

    public ObservableCollection<TaskQueueItemViewModel> Tasks { get; }

    public ObservableCollection<TaskQueueLogCardViewModel> LogCards { get; }

    public ObservableCollection<TaskQueueLogEntryViewModel> OverlayLogs { get; }

    public TaskQueueLogEntryViewModel DownloadLogEntry
    {
        get => _downloadLogEntry;
        private set
        {
            if (SetProperty(ref _downloadLogEntry, value))
            {
                OnPropertyChanged(nameof(HasDownloadLog));
            }
        }
    }

    public bool HasDownloadLog => !string.IsNullOrWhiteSpace(DownloadLogEntry.Content);

    public ObservableCollection<OverlayTarget> OverlayTargets { get; }

    public LocalizedTextMap Texts { get; }

    public RootLocalizationTextMap RootTexts { get; }

    public StartUpTaskModuleViewModel StartUpModule { get; }

    public FightTaskModuleViewModel FightModule { get; }

    public RecruitTaskModuleViewModel RecruitModule { get; }

    public InfrastModuleViewModel InfrastModule { get; }

    public MallModuleViewModel MallModule { get; }

    public AwardModuleViewModel AwardModule { get; }

    public RoguelikeModuleViewModel RoguelikeModule { get; }

    public ReclamationModuleViewModel ReclamationModule { get; }

    public CustomModuleViewModel CustomModule { get; }

    public PostActionModuleViewModel PostActionModule { get; }

    public TaskRuntimeStatusSnapshot? LastRuntimeStatus
    {
        get => _lastRuntimeStatus;
        private set => SetProperty(ref _lastRuntimeStatus, value);
    }

    public string SelectedTaskValidationSummary
    {
        get => _selectedTaskValidationSummary;
        private set
        {
            if (SetProperty(ref _selectedTaskValidationSummary, value))
            {
                OnPropertyChanged(nameof(HasSelectedTaskValidationSummary));
            }
        }
    }

    public bool SelectedTaskHasBlockingValidationIssues
    {
        get => _selectedTaskHasBlockingValidationIssues;
        private set => SetProperty(ref _selectedTaskHasBlockingValidationIssues, value);
    }

    public int SelectedTaskValidationIssueCount
    {
        get => _selectedTaskValidationIssueCount;
        private set
        {
            if (SetProperty(ref _selectedTaskValidationIssueCount, value))
            {
                OnPropertyChanged(nameof(SelectedTaskHasValidationIssues));
            }
        }
    }

    public bool HasSelectedTaskValidationSummary => !string.IsNullOrWhiteSpace(SelectedTaskValidationSummary);

    public bool SelectedTaskHasValidationIssues => SelectedTaskValidationIssueCount > 0;

    public string StartPrecheckWarningMessage
    {
        get => _startPrecheckWarningMessage;
        private set
        {
            if (SetProperty(ref _startPrecheckWarningMessage, value))
            {
                OnPropertyChanged(nameof(HasStartPrecheckWarningMessage));
            }
        }
    }

    public bool HasStartPrecheckWarningMessage => !string.IsNullOrWhiteSpace(StartPrecheckWarningMessage);

    public string CoreInitializationMessage
    {
        get => _coreInitializationMessage;
        private set
        {
            if (SetProperty(ref _coreInitializationMessage, value))
            {
                OnPropertyChanged(nameof(HasCoreInitializationMessage));
            }
        }
    }

    public bool HasCoreInitializationMessage => !string.IsNullOrWhiteSpace(CoreInitializationMessage);

    public string NoTaskSelectedHint
    {
        get => _noTaskSelectedHint;
        private set => SetProperty(ref _noTaskSelectedHint, value);
    }

    public bool IsNoTaskSelected => SelectedTask is null;

    public bool IsStartUpTaskSelected => IsSelectedTaskType(TaskModuleTypes.StartUp);

    public bool IsFightTaskSelected => IsSelectedTaskType(TaskModuleTypes.Fight);

    public bool IsRecruitTaskSelected => IsSelectedTaskType(TaskModuleTypes.Recruit);

    public bool IsInfrastTaskSelected => IsSelectedTaskType(TaskModuleTypes.Infrast);

    public bool IsMallTaskSelected => IsSelectedTaskType(TaskModuleTypes.Mall);

    public bool IsAwardTaskSelected => IsSelectedTaskType(TaskModuleTypes.Award);

    public bool IsRoguelikeTaskSelected => IsSelectedTaskType(TaskModuleTypes.Roguelike);

    public bool IsReclamationTaskSelected => IsSelectedTaskType(TaskModuleTypes.Reclamation);

    public bool IsCustomTaskSelected => IsSelectedTaskType(TaskModuleTypes.Custom);

    public bool IsPostActionTaskSelected => IsSelectedTaskType(TaskModuleTypes.PostAction);

    public TaskQueueItemViewModel? SelectedTask
    {
        get => _selectedTask;
        set
        {
            if (!SetProperty(ref _selectedTask, value))
            {
                return;
            }

            SelectedTaskModule = value is null
                ? TaskModuleTypes.StartUp
                : TaskModuleTypes.Normalize(value.Type);
            RenameTargetName = value?.Name ?? string.Empty;
            if (value is not null)
            {
                IsPostActionPanelSelected = false;
            }
            RememberSelectedTaskIndex();
            RaiseSelectedTaskProjectionChanged();
            ResetSettingsModeForSelectedTask();
            ScheduleBindSelectedTask();
        }
    }

    public string SelectedTaskModule
    {
        get => _selectedTaskModule;
        set
        {
            var normalized = TaskModuleTypes.Normalize(value);
            if (!SetProperty(ref _selectedTaskModule, normalized))
            {
                return;
            }

            var matched = TaskModules.FirstOrDefault(
                option => string.Equals(option.Type, normalized, StringComparison.OrdinalIgnoreCase));
            if (matched is not null && !Equals(_selectedTaskModuleOption, matched))
            {
                _selectedTaskModuleOption = matched;
                OnPropertyChanged(nameof(SelectedTaskModuleOption));
            }
        }
    }

    public TaskModuleOption? SelectedTaskModuleOption
    {
        get => _selectedTaskModuleOption;
        set
        {
            if (!SetProperty(ref _selectedTaskModuleOption, value))
            {
                return;
            }

            if (value is not null)
            {
                SelectedTaskModule = value.Type;
            }
        }
    }

    public string RenameTargetName
    {
        get => _renameTargetName;
        set => SetProperty(ref _renameTargetName, value);
    }

    public bool IsPostActionPanelSelected
    {
        get => _isPostActionPanelSelected;
        private set
        {
            if (!SetProperty(ref _isPostActionPanelSelected, value))
            {
                return;
            }

            if (value)
            {
                SetAdvancedSettingsSelected(false);
            }

            RaiseSelectedTaskProjectionChanged();
        }
    }

    public SessionState CurrentSessionState
    {
        get => _currentSessionState;
        private set
        {
            if (SetProperty(ref _currentSessionState, value))
            {
                OnPropertyChanged(nameof(IsRunning));
                OnPropertyChanged(nameof(CanEdit));
                OnPropertyChanged(nameof(RunButtonText));
                OnPropertyChanged(nameof(CanToggleRun));
                OnPropertyChanged(nameof(CanWaitAndStop));
            }
        }
    }

    public bool IsRunning => CurrentSessionState is SessionState.Running or SessionState.Stopping;

    public bool CanEdit => !IsRunning;

    public string RunButtonText => IsRunning
        ? RootTexts.GetOrDefault("TaskQueue.Root.Stop", "Stop")
        : RootTexts.GetOrDefault("TaskQueue.Root.LinkStart", "Link Start!");

    public bool HasBlockingConfigIssues
    {
        get => _hasBlockingConfigIssues;
        private set
        {
            if (SetProperty(ref _hasBlockingConfigIssues, value))
            {
                OnPropertyChanged(nameof(CanToggleRun));
            }
        }
    }

    public int BlockingConfigIssueCount
    {
        get => _blockingConfigIssueCount;
        private set => SetProperty(ref _blockingConfigIssueCount, value);
    }

    public bool IsCoreReady
    {
        get => _isCoreReady;
        private set
        {
            if (SetProperty(ref _isCoreReady, value))
            {
                OnPropertyChanged(nameof(CanToggleRun));
            }
        }
    }

    public bool CanToggleRun =>
        !IsWaitingForStop
        && CurrentSessionState != SessionState.Stopping;

    public bool CanWaitAndStop =>
        !IsWaitingForStop
        && CurrentSessionState == SessionState.Running;

    public bool IsWaitingForStop
    {
        get => _isWaitingForStop;
        private set
        {
            if (SetProperty(ref _isWaitingForStop, value))
            {
                OnPropertyChanged(nameof(CanToggleRun));
                OnPropertyChanged(nameof(CanWaitAndStop));
                OnPropertyChanged(nameof(WaitAndStopButtonText));
            }
        }
    }

    public string WaitAndStopButtonText => IsWaitingForStop
        ? RootTexts.GetOrDefault("TaskQueue.Root.Waiting", "Waiting...")
        : RootTexts.GetOrDefault("TaskQueue.Root.WaitAndStop", "Wait & Stop");

    public SelectionBatchMode SelectionBatchMode
    {
        get => _selectionBatchMode;
        private set
        {
            if (SetProperty(ref _selectionBatchMode, value))
            {
                OnPropertyChanged(nameof(BatchActionText));
                OnPropertyChanged(nameof(BatchToggleMenuText));
            }
        }
    }

    public bool ShowBatchModeToggle
    {
        get => _showBatchModeToggle;
        private set
        {
            if (SetProperty(ref _showBatchModeToggle, value))
            {
                OnPropertyChanged(nameof(BatchToggleMenuText));
            }
        }
    }

    public string BatchActionText => SelectionBatchMode == SelectionBatchMode.Inverse
        ? RootTexts.GetOrDefault("TaskQueue.Root.Inverse", "Inverse")
        : RootTexts.GetOrDefault("TaskQueue.Root.Clear", "Clear");

    public string BatchToggleMenuText
    {
        get
        {
            var targetText = SelectionBatchMode == SelectionBatchMode.Inverse
                ? RootTexts.GetOrDefault("TaskQueue.Root.Clear", "Clear")
                : RootTexts.GetOrDefault("TaskQueue.Root.Inverse", "Inverse");
            return string.Format(
                RootTexts.GetOrDefault("TaskQueue.Root.SwitchBatchMode", "Switch to {0}"),
                targetText);
        }
    }

    public string TaskMenuTooltipText =>
        $"{RootTexts.GetOrDefault("TaskQueue.Root.LeftClick", "Left click")}: {RootTexts.GetOrDefault("TaskQueue.Root.TaskSettings", "Task settings")}{Environment.NewLine}" +
        $"{RootTexts.GetOrDefault("TaskQueue.Root.RightClick", "Right click")}: " +
        $"{RootTexts.GetOrDefault("TaskQueue.Root.MoveUp", "Move up")} / " +
        $"{RootTexts.GetOrDefault("TaskQueue.Root.MoveDown", "Move down")} / " +
        $"{RootTexts.GetOrDefault("TaskQueue.Root.Rename", "Rename")} / " +
        $"{RootTexts.GetOrDefault("TaskQueue.Root.Delete", "Delete")}";

    public bool IsGeneralSettingsSelected
    {
        get => !_isAdvancedSettingsSelected;
        set
        {
            if (value)
            {
                SetAdvancedSettingsSelected(false);
            }
        }
    }

    public bool IsAdvancedSettingsSelected
    {
        get => _isAdvancedSettingsSelected;
        set => SetAdvancedSettingsSelected(value);
    }

    public bool CanUseAdvancedSettings =>
        IsFightTaskSelected
        || IsRecruitTaskSelected
        || IsInfrastTaskSelected
        || IsMallTaskSelected
        || IsRoguelikeTaskSelected
        || IsReclamationTaskSelected;

    public bool ShowSettingsModeSwitch =>
        !IsPostActionPanelSelected
        && !IsPostActionTaskSelected
        && SelectedTask is not null
        && !IsStartUpTaskSelected
        && !IsAwardTaskSelected;

    public bool ShowTaskConfigHint => !IsPostActionPanelSelected && IsNoTaskSelected;

    public bool ShowPostActionSettingsPanel => IsPostActionPanelSelected || IsPostActionTaskSelected;

    public object? SelectedTaskSettingsViewModel
    {
        get
        {
            if (ShowPostActionSettingsPanel)
            {
                return PostActionModule;
            }

            if (SelectedTask is null)
            {
                return null;
            }

            return SelectedTaskModule switch
            {
                TaskModuleTypes.StartUp => StartUpModule,
                TaskModuleTypes.Fight => FightModule,
                TaskModuleTypes.Recruit => RecruitModule,
                TaskModuleTypes.Infrast => InfrastModule,
                TaskModuleTypes.Mall => MallModule,
                TaskModuleTypes.Award => AwardModule,
                TaskModuleTypes.Roguelike => RoguelikeModule,
                TaskModuleTypes.Reclamation => ReclamationModule,
                TaskModuleTypes.Custom => CustomModule,
                _ => null,
            };
        }
    }

    public void SelectGeneralSettingsMode()
    {
        SetAdvancedSettingsSelected(false);
    }

    public void SelectAdvancedSettingsMode()
    {
        SetAdvancedSettingsSelected(true);
    }

    public void OpenPostActionPanel()
    {
        IsPostActionPanelSelected = true;
        SelectedTask = null;
    }

    public string PostActionActionTitle =>
        PostActionModule.Once
            ? string.Format(
                RootTexts.GetOrDefault("TaskQueue.Root.PostActionTitleWithOnce", "{0} ({1})"),
                RootTexts.GetOrDefault("TaskQueue.Root.PostActionTitle", "After Completion"),
                Texts.GetOrDefault("PostAction.Once", "Once"))
            : RootTexts.GetOrDefault("TaskQueue.Root.PostActionTitle", "After Completion");

    public string PostActionActionDescription
    {
        get
        {
            var actions = BuildPostActionActionLabels();
            if (actions.Count == 0)
            {
                return RootTexts.GetOrDefault("TaskQueue.Root.PostActionNone", "Do nothing");
            }

            return string.Join(" / ", actions);
        }
    }

    public string DailyStageHint
    {
        get => _dailyStageHint;
        set => SetProperty(ref _dailyStageHint, value);
    }

    public string OverlayStatusText
    {
        get => _overlayStatusText;
        set
        {
            if (SetProperty(ref _overlayStatusText, value))
            {
                OnPropertyChanged(nameof(OverlayButtonToolTip));
            }
        }
    }

    public OverlayTarget? SelectedOverlayTarget
    {
        get => _selectedOverlayTarget;
        set
        {
            if (!SetProperty(ref _selectedOverlayTarget, value))
            {
                return;
            }

            _overlaySharedState.SelectedTargetId = value?.Id ?? "preview";
            OnPropertyChanged(nameof(OverlayTargetSummaryText));
            OnPropertyChanged(nameof(OverlayButtonToolTip));
        }
    }

    public bool OverlayVisible
    {
        get => _overlayVisible;
        set
        {
            if (!SetProperty(ref _overlayVisible, value))
            {
                return;
            }

            _overlaySharedState.Visible = value;
        }
    }

    public OverlayRuntimeMode OverlayMode
    {
        get => _overlayMode;
        private set
        {
            if (!SetProperty(ref _overlayMode, value))
            {
                return;
            }

            OnPropertyChanged(nameof(IsOverlayHiddenMode));
            OnPropertyChanged(nameof(IsOverlayPreviewMode));
            OnPropertyChanged(nameof(IsOverlayNativeMode));
            OnPropertyChanged(nameof(OverlayButtonToolTip));
        }
    }

    public bool IsOverlayHiddenMode => OverlayMode == OverlayRuntimeMode.Hidden;

    public bool IsOverlayPreviewMode => OverlayMode == OverlayRuntimeMode.Preview;

    public bool IsOverlayNativeMode => OverlayMode == OverlayRuntimeMode.Native;

    public string OverlayTargetSummaryText
    {
        get
        {
            if (!string.IsNullOrWhiteSpace(SelectedOverlayTarget?.DisplayName))
            {
                return SelectedOverlayTarget.DisplayName;
            }

            return string.Equals(_overlaySharedState.SelectedTargetId, "preview", StringComparison.OrdinalIgnoreCase)
                ? "Preview + Logs"
                : _overlaySharedState.SelectedTargetId;
        }
    }

    public string OverlayButtonToolTip =>
        $"{RootTexts.GetOrDefault("TaskQueue.Root.LeftClick", "Left click")}: {RootTexts.GetOrDefault("TaskQueue.Root.ToggleOverlay", "Toggle overlay")}{Environment.NewLine}" +
        $"{RootTexts.GetOrDefault("TaskQueue.Root.RightClick", "Right click")}: {RootTexts.GetOrDefault("TaskQueue.Root.PickTarget", "Pick target")}{Environment.NewLine}" +
        $"{OverlayStatusText}{Environment.NewLine}" +
        $"{RootTexts.GetOrDefault("TaskQueue.Root.PickTarget", "Pick target")}: {OverlayTargetSummaryText}";

    public async Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        await InitializeFirstScreenAsync(cancellationToken);
        await InitializeDeferredStartupAsync(cancellationToken);
    }

    public async Task InitializeFirstScreenAsync(CancellationToken cancellationToken = default)
    {
        SetLanguage(ResolveLanguage());
        RefreshConfigValidationState(Runtime.ConfigurationService.CurrentValidationIssues);
        await ReloadTasksAsync(cancellationToken, preferProfileSelectedIndex: true);
        await InfrastModule.ReloadPersistentConfigAsync(cancellationToken);
        await RoguelikeModule.ReloadPersistentConfigAsync(cancellationToken);
        RefreshSelectionBatchModeFromConfig();
        RefreshStagePresentation();
        UpdatePostActionSummary();
    }

    public async Task InitializeDeferredStartupAsync(CancellationToken cancellationToken = default)
    {
        await ReloadOverlayTargetsAsync(cancellationToken);
        await PostActionModule.InitializeAsync(cancellationToken);
        UpdatePostActionSummary();
    }

    public async Task ReloadConfigurationContextAsync(
        bool forceReloadStageOptions = false,
        CancellationToken cancellationToken = default)
    {
        PrepareForConfigurationContextSwitch();
        await ReloadTasksAsync(cancellationToken, preferProfileSelectedIndex: true);
        await InfrastModule.ReloadPersistentConfigAsync(cancellationToken);
        await RoguelikeModule.ReloadPersistentConfigAsync(cancellationToken);
        RefreshSelectionBatchModeFromConfig();
        await PostActionModule.InitializeAsync(cancellationToken);
        RefreshStagePresentation(forceReloadStageOptions);
        UpdatePostActionSummary();
    }

    public void SetLanguage(string language)
    {
        var normalized = UiLanguageCatalog.Normalize(language);
        Texts.Language = normalized;
        RootTexts.Language = normalized;
        DailyStageHint = FightTaskModuleViewModel.BuildDailyResourceHint(
            Texts.Language,
            _connectionGameSharedState.ClientType,
            Runtime.ConfigurationService.CurrentConfig);
        NoTaskSelectedHint = Texts.GetOrDefault(
            "TaskQueue.SelectionHint",
            "Select a task from the left list to edit its settings.");
        OverlayStatusText = Texts.GetOrDefault("TaskQueue.OverlayDisconnected", "Overlay disconnected");
        FightModule.RefreshStageOptions(_connectionGameSharedState.ClientType);
        RebuildTaskModuleOptions();
        RefreshTaskItemsLocalization();
        UpdatePostActionSummary();
        OnPropertyChanged(nameof(RunButtonText));
        OnPropertyChanged(nameof(WaitAndStopButtonText));
        OnPropertyChanged(nameof(BatchActionText));
        OnPropertyChanged(nameof(BatchToggleMenuText));
        OnPropertyChanged(nameof(TaskMenuTooltipText));
        OnPropertyChanged(nameof(OverlayMode));
        OnPropertyChanged(nameof(IsOverlayHiddenMode));
        OnPropertyChanged(nameof(IsOverlayPreviewMode));
        OnPropertyChanged(nameof(IsOverlayNativeMode));
        OnPropertyChanged(nameof(OverlayTargetSummaryText));
        OnPropertyChanged(nameof(OverlayButtonToolTip));
        _ = RefreshOverlayStatusTextAsync();
    }

    public void SetCoreAvailability(bool isReady, string? message = null)
    {
        IsCoreReady = isReady;
        CoreInitializationMessage = isReady ? string.Empty : (message?.Trim() ?? string.Empty);
    }

    public void RefreshStagePresentation(bool forceReloadStageOptions = false)
    {
        FightModule.RefreshStageOptions(_connectionGameSharedState.ClientType, forceReloadStageOptions);
        DailyStageHint = FightTaskModuleViewModel.BuildDailyResourceHint(
            Texts.Language,
            _connectionGameSharedState.ClientType,
            Runtime.ConfigurationService.CurrentConfig);
    }

    private void OnConnectionGameSharedStateChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (!string.Equals(e.PropertyName, nameof(ConnectionGameSharedStateViewModel.ClientType), StringComparison.Ordinal))
        {
            return;
        }

        RefreshStagePresentation();
    }

    private void RebuildTaskModuleOptions()
    {
        var currentType = TaskModuleTypes.Normalize(_selectedTaskModule);
        var options = AddableTaskModules
            .Select(moduleType => new TaskModuleOption(moduleType, ResolveModuleDisplayName(moduleType)))
            .ToArray();

        TaskModules.Clear();
        foreach (var option in options)
        {
            TaskModules.Add(option);
        }

        var next = options.FirstOrDefault(
                       option => option is not null
                                 && string.Equals(option.Type, currentType, StringComparison.OrdinalIgnoreCase))
                   ?? options.FirstOrDefault();
        _selectedTaskModuleOption = next;
        _selectedTaskModule = next?.Type ?? TaskModuleTypes.StartUp;
        OnPropertyChanged(nameof(SelectedTaskModuleOption));
        OnPropertyChanged(nameof(SelectedTaskModule));
    }

    private string ResolveModuleDisplayName(string moduleType)
    {
        var normalized = TaskModuleTypes.Normalize(moduleType);
        var key = $"TaskQueue.Module.{normalized}";
        return RootTexts.GetOrDefault(key, normalized);
    }

    private string ResolveStatusDisplayName(string status)
    {
        return status switch
        {
            TaskQueueItemStatus.Running => RootTexts.GetOrDefault("TaskQueue.Status.Running", "Running"),
            TaskQueueItemStatus.Success => RootTexts.GetOrDefault("TaskQueue.Status.Success", "Completed"),
            TaskQueueItemStatus.Error => RootTexts.GetOrDefault("TaskQueue.Status.Error", "Error"),
            TaskQueueItemStatus.Skipped => RootTexts.GetOrDefault("TaskQueue.Status.Skipped", "Skipped"),
            TaskQueueItemStatus.Idle => RootTexts.GetOrDefault("TaskQueue.Status.Idle", "Idle"),
            _ => RootTexts.GetOrDefault("TaskQueue.Status.Observed", status),
        };
    }

    private void RefreshTaskItemLocalization(TaskQueueItemViewModel item)
    {
        item.RefreshLocalizedText(ResolveModuleDisplayName, ResolveStatusDisplayName);
        item.DisplayName = ResolveTaskDisplayName(item);
        item.RefreshToolTipText();
    }

    private void RefreshTaskItemsLocalization()
    {
        // Take a snapshot to avoid enumeration invalidation when callbacks mutate Tasks.
        foreach (var task in Tasks.ToArray())
        {
            RefreshTaskItemLocalization(task);
        }
    }

    private string ResolveTaskDisplayName(TaskQueueItemViewModel item)
    {
        var moduleDisplay = ResolveModuleDisplayName(item.Type);
        var name = (item.Name ?? string.Empty).Trim();
        if (string.IsNullOrWhiteSpace(name))
        {
            return moduleDisplay;
        }

        if (IsDefaultTaskName(name, item.Type, moduleDisplay))
        {
            return moduleDisplay;
        }

        return name;
    }

    private static bool IsDefaultTaskName(string name, string moduleType, string localizedDisplayName)
    {
        var normalizedModuleType = TaskModuleTypes.Normalize(moduleType);
        if (string.Equals(name, normalizedModuleType, StringComparison.OrdinalIgnoreCase)
            || string.Equals(name, localizedDisplayName, StringComparison.OrdinalIgnoreCase))
        {
            return true;
        }

        return LegacyTaskNameAliases.TryGetValue(normalizedModuleType, out var aliases)
               && aliases.Any(alias => string.Equals(name, alias, StringComparison.OrdinalIgnoreCase));
    }

    private void OnPostActionModulePropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (string.Equals(e.PropertyName, nameof(PostActionModuleViewModel.StatusMessage), StringComparison.Ordinal)
            || string.Equals(e.PropertyName, nameof(PostActionModuleViewModel.LastErrorMessage), StringComparison.Ordinal))
        {
            return;
        }

        UpdatePostActionSummary();
    }

    private void UpdatePostActionSummary()
    {
        OnPropertyChanged(nameof(PostActionActionTitle));
        OnPropertyChanged(nameof(PostActionActionDescription));
    }

    private List<string> BuildPostActionActionLabels()
    {
        var actions = new List<string>();
        if (PostActionModule.BackToAndroidHome)
        {
            actions.Add(Texts.GetOrDefault("PostAction.BackToHome", "Back to home"));
        }

        if (PostActionModule.ExitArknights)
        {
            actions.Add(Texts.GetOrDefault("PostAction.ExitArknights", "Exit Arknights"));
        }

        if (PostActionModule.ExitEmulator)
        {
            actions.Add(Texts.GetOrDefault("PostAction.ExitEmulator", "Exit emulator"));
        }

        if (PostActionModule.ExitSelf)
        {
            actions.Add(Texts.GetOrDefault("PostAction.ExitSelf", "Exit MAA"));
        }

        if (PostActionModule.IfNoOtherMaa)
        {
            actions.Add(Texts.GetOrDefault("PostAction.IfNoOther", "Only when no other MAA"));
        }

        if (PostActionModule.Sleep)
        {
            actions.Add(Texts.GetOrDefault("PostAction.Sleep", "Sleep"));
        }

        if (PostActionModule.Hibernate)
        {
            actions.Add(Texts.GetOrDefault("PostAction.Hibernate", "Hibernate"));
        }

        if (PostActionModule.Shutdown)
        {
            actions.Add(Texts.GetOrDefault("PostAction.Shutdown", "Shutdown"));
        }

        return actions;
    }

    public async Task ReloadTasksAsync(
        CancellationToken cancellationToken = default,
        bool preferProfileSelectedIndex = false)
    {
        var previousSelectedIndex = SelectedTask is null ? -1 : Tasks.IndexOf(SelectedTask);
        var tasks = await ApplyResultAsync(
            await Runtime.TaskQueueFeatureService.GetCurrentTaskQueueAsync(cancellationToken),
            "TaskQueue.Reload",
            cancellationToken);

        if (tasks is null)
        {
            return;
        }

        if (tasks.Count == 0)
        {
            if (!await SeedDefaultTaskQueueAsync(cancellationToken))
            {
                return;
            }

            tasks = await ApplyResultAsync(
                await Runtime.TaskQueueFeatureService.GetCurrentTaskQueueAsync(cancellationToken),
                "TaskQueue.Reload",
                cancellationToken);
            if (tasks is null)
            {
                return;
            }
        }

        foreach (var task in Tasks)
        {
            task.PropertyChanged -= OnTaskPropertyChanged;
        }

        _suppressTaskEnabledSync = true;
        try
        {
            Tasks.Clear();
            foreach (var task in tasks)
            {
                var item = TaskQueueItemViewModel.FromUnifiedTask(task);
                RefreshTaskItemLocalization(item);
                item.PropertyChanged += OnTaskPropertyChanged;
                Tasks.Add(item);
            }
        }
        finally
        {
            _suppressTaskEnabledSync = false;
        }

        var reloadSelectionIndex = ResolveReloadSelectionIndex(preferProfileSelectedIndex, previousSelectedIndex, Tasks.Count);
        if (reloadSelectionIndex.HasValue)
        {
            SelectedTask = Tasks[reloadSelectionIndex.Value];
        }
        else
        {
            SelectedTask = preferProfileSelectedIndex
                ? null
                : Tasks.FirstOrDefault();
        }

        await WaitForPendingBindingAsync(cancellationToken);
    }

    public async Task WaitForPendingBindingAsync(CancellationToken cancellationToken = default)
    {
        while (true)
        {
            Task pending;
            lock (_pendingBindingGate)
            {
                pending = _pendingBindingTask;
            }

            await pending.WaitAsync(cancellationToken);

            lock (_pendingBindingGate)
            {
                if (ReferenceEquals(pending, _pendingBindingTask))
                {
                    return;
                }
            }
        }
    }

    private async Task<bool> ExecuteQueueMutationAsync(
        string scope,
        Func<CancellationToken, Task<UiOperationResult>> mutationAsync,
        Func<CancellationToken, Task>? onSuccessAsync = null,
        CancellationToken cancellationToken = default)
    {
        if (!await EnsureEditableAsync(scope, cancellationToken))
        {
            return false;
        }

        await _queueMutationLock.WaitAsync(cancellationToken);
        try
        {
            await WaitForPendingBindingAsync(cancellationToken);
            if (!await SaveBoundTaskModulesAsync(cancellationToken))
            {
                return false;
            }

            var result = await mutationAsync(cancellationToken);
            if (!await ApplyResultAsync(result, scope, cancellationToken))
            {
                return false;
            }

            await ReloadTasksAsync(cancellationToken);
            await WaitForPendingBindingAsync(cancellationToken);

            if (onSuccessAsync is not null)
            {
                await onSuccessAsync(cancellationToken);
                await WaitForPendingBindingAsync(cancellationToken);
            }

            return true;
        }
        finally
        {
            _queueMutationLock.Release();
        }
    }

    private async Task SelectTaskByIndexAsync(int index, CancellationToken cancellationToken)
    {
        if (index < 0 || index >= Tasks.Count)
        {
            return;
        }

        SelectedTask = Tasks[index];
        await WaitForPendingBindingAsync(cancellationToken);
    }

    private async void OnTaskPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (!string.Equals(e.PropertyName, nameof(TaskQueueItemViewModel.IsEnabled), StringComparison.Ordinal))
        {
            return;
        }

        if (sender is not TaskQueueItemViewModel task || _suppressTaskEnabledSync)
        {
            return;
        }

        try
        {
            await PersistTaskEnabledStateAsync(task);
        }
        catch (Exception ex)
        {
            await RecordUnhandledExceptionAsync(
                "TaskQueue.SetTaskEnabled",
                ex,
                UiErrorCode.UiOperationFailed,
                "Failed to persist task enabled state.");
        }
    }

    private async Task PersistTaskEnabledStateAsync(TaskQueueItemViewModel task, CancellationToken cancellationToken = default)
    {
        var desiredEnabled = task.IsEnabled;
        if (!await EnsureEditableAsync("TaskQueue.SetTaskEnabled", cancellationToken))
        {
            _suppressTaskEnabledSync = true;
            try
            {
                task.IsEnabled = !desiredEnabled;
            }
            finally
            {
                _suppressTaskEnabledSync = false;
            }

            return;
        }

        var index = Tasks.IndexOf(task);
        if (index < 0)
        {
            return;
        }

        await _queueMutationLock.WaitAsync(cancellationToken);
        try
        {
            await WaitForPendingBindingAsync(cancellationToken);
            if (!await SaveBoundTaskModulesAsync(cancellationToken))
            {
                return;
            }

            var result = await Runtime.TaskQueueFeatureService.SetTaskEnabledAsync(index, desiredEnabled, cancellationToken);
            if (!await ApplyResultAsync(result, "TaskQueue.SetTaskEnabled", cancellationToken))
            {
                _suppressTaskEnabledSync = true;
                try
                {
                    task.IsEnabled = !desiredEnabled;
                }
                finally
                {
                    _suppressTaskEnabledSync = false;
                }
            }
        }
        finally
        {
            _queueMutationLock.Release();
        }
    }

    public async Task AddTaskAsync(string? taskType = null, CancellationToken cancellationToken = default)
    {
        var normalizedTaskType = TaskModuleTypes.Normalize(taskType ?? SelectedTaskModuleOption?.Type ?? SelectedTaskModule);
        var taskName = ResolveModuleDisplayName(normalizedTaskType);
        await ExecuteQueueMutationAsync(
            "TaskQueue.AddTask",
            ct => Runtime.TaskQueueFeatureService.AddTaskAsync(normalizedTaskType, taskName, true, ct),
            cancellationToken: cancellationToken);
    }

    public async Task RemoveSelectedTaskAsync(CancellationToken cancellationToken = default)
    {
        if (SelectedTask is null)
        {
            LastErrorMessage = Texts["TaskQueue.Error.SelectTaskToRemove"];
            return;
        }

        var index = Tasks.IndexOf(SelectedTask);
        if (index < 0)
        {
            LastErrorMessage = Texts["TaskQueue.Error.SelectTaskToRemove"];
            return;
        }

        await ExecuteQueueMutationAsync(
            "TaskQueue.RemoveTask",
            ct => Runtime.TaskQueueFeatureService.RemoveTaskAsync(index, ct),
            cancellationToken: cancellationToken);
    }

    public async Task RenameSelectedTaskAsync(CancellationToken cancellationToken = default)
    {
        if (SelectedTask is null)
        {
            LastErrorMessage = Texts["TaskQueue.Error.SelectTaskToRename"];
            return;
        }

        var index = Tasks.IndexOf(SelectedTask);
        if (index < 0)
        {
            LastErrorMessage = Texts["TaskQueue.Error.SelectTaskToRename"];
            return;
        }

        await ExecuteQueueMutationAsync(
            "TaskQueue.RenameTask",
            ct => Runtime.TaskQueueFeatureService.RenameTaskAsync(index, RenameTargetName, ct),
            cancellationToken: cancellationToken);
    }

    public async Task RenameSelectedTaskWithDialogAsync(CancellationToken cancellationToken = default)
    {
        if (SelectedTask is null)
        {
            LastErrorMessage = Texts["TaskQueue.Error.SelectTaskToRename"];
            return;
        }

        var index = Tasks.IndexOf(SelectedTask);
        if (index < 0)
        {
            LastErrorMessage = Texts["TaskQueue.Error.SelectTaskToRename"];
            return;
        }

        var request = new TextDialogRequest(
            Title: $"Rename Task {index + 1}",
            Prompt: Texts.GetOrDefault("TaskQueue.Error.SelectTaskToRename", "Rename task"),
            DefaultText: SelectedTask.Name,
            MultiLine: false,
            ConfirmText: "Confirm",
            CancelText: "Cancel");
        var dialogResult = await _dialogService.ShowTextAsync(request, "TaskQueue.RenameTask.Dialog", cancellationToken);
        if (dialogResult.Return == DialogReturnSemantic.Confirm && dialogResult.Payload is not null)
        {
            var nextName = (dialogResult.Payload.Text ?? string.Empty).Trim();
            if (nextName.Length == 0)
            {
                LastErrorMessage = BuildBilingualMessage("任务名不能为空。", "Task name cannot be empty.");
                return;
            }

            RenameTargetName = nextName;
            await RenameSelectedTaskAsync(cancellationToken);
            return;
        }

        StatusMessage = dialogResult.Return == DialogReturnSemantic.Cancel
            ? "Rename cancelled."
            : "Rename dialog closed.";
    }

    public async Task MoveSelectedTaskAsync(int delta, CancellationToken cancellationToken = default)
    {
        if (SelectedTask is null)
        {
            LastErrorMessage = Texts["TaskQueue.Error.SelectTaskToMove"];
            return;
        }

        var from = Tasks.IndexOf(SelectedTask);
        if (from < 0)
        {
            return;
        }

        await MoveSelectedTaskToAsync(from + delta, cancellationToken);
    }

    public async Task MoveSelectedTaskToAsync(int targetIndex, CancellationToken cancellationToken = default)
    {
        if (SelectedTask is null)
        {
            LastErrorMessage = Texts["TaskQueue.Error.SelectTaskToMove"];
            return;
        }

        var from = Tasks.IndexOf(SelectedTask);
        var to = Math.Clamp(targetIndex, 0, Tasks.Count - 1);
        if (from < 0 || from == to)
        {
            return;
        }

        await ExecuteQueueMutationAsync(
            "TaskQueue.MoveTask",
            ct => Runtime.TaskQueueFeatureService.MoveTaskAsync(from, to, ct),
            ct => SelectTaskByIndexAsync(to, ct),
            cancellationToken);
    }

    public async Task SelectAllAsync(bool enabled, CancellationToken cancellationToken = default)
    {
        await ExecuteQueueMutationAsync(
            "TaskQueue.SelectAll",
            ct => Runtime.TaskQueueFeatureService.SetAllTasksEnabledAsync(enabled, ct),
            cancellationToken: cancellationToken);
    }

    public async Task InverseSelectionAsync(CancellationToken cancellationToken = default)
    {
        await ExecuteQueueMutationAsync(
            "TaskQueue.InverseSelection",
            ct => Runtime.TaskQueueFeatureService.InvertTasksEnabledAsync(ct),
            cancellationToken: cancellationToken);
    }

    public async Task ExecuteBatchActionAsync(CancellationToken cancellationToken = default)
    {
        if (SelectionBatchMode == SelectionBatchMode.Inverse)
        {
            await InverseSelectionAsync(cancellationToken);
            return;
        }

        await SelectAllAsync(false, cancellationToken);
    }

    public async Task ToggleSelectionBatchModeAsync(CancellationToken cancellationToken = default)
    {
        if (!ShowBatchModeToggle)
        {
            return;
        }

        SelectionBatchMode = SelectionBatchMode == SelectionBatchMode.Inverse
            ? SelectionBatchMode.Clear
            : SelectionBatchMode.Inverse;
        await PersistSelectionBatchModeAsync(cancellationToken);
    }

    public async Task SaveAsync(CancellationToken cancellationToken = default)
    {
        if (!await EnsureEditableAsync("TaskQueue.Save", cancellationToken))
        {
            return;
        }

        if (!await SaveBoundTaskModulesAsync(cancellationToken))
        {
            return;
        }

        await ApplyResultAsync(await Runtime.TaskQueueFeatureService.SaveAsync(cancellationToken), "TaskQueue.Save", cancellationToken);
    }

    private async Task<bool> SeedDefaultTaskQueueAsync(CancellationToken cancellationToken)
    {
        foreach (var moduleType in WpfDefaultTaskOrder)
        {
            var addResult = await Runtime.TaskQueueFeatureService.AddTaskAsync(
                moduleType,
                ResolveModuleDisplayName(moduleType),
                enabled: true,
                cancellationToken);
            if (!await ApplyResultAsync(addResult, "TaskQueue.SeedDefaults.Add", cancellationToken))
            {
                return false;
            }
        }

        return await ApplyResultAsync(
            await Runtime.TaskQueueFeatureService.SaveAsync(cancellationToken),
            "TaskQueue.SeedDefaults.Save",
            cancellationToken);
    }

    private bool IsSelectedTaskType(string moduleType)
    {
        return SelectedTask is not null
            && string.Equals(
                TaskModuleTypes.Normalize(SelectedTask.Type),
                moduleType,
                StringComparison.OrdinalIgnoreCase);
    }

    private void RaiseSelectedTaskProjectionChanged()
    {
        OnPropertyChanged(nameof(IsNoTaskSelected));
        OnPropertyChanged(nameof(ShowTaskConfigHint));
        OnPropertyChanged(nameof(IsStartUpTaskSelected));
        OnPropertyChanged(nameof(IsFightTaskSelected));
        OnPropertyChanged(nameof(IsRecruitTaskSelected));
        OnPropertyChanged(nameof(IsInfrastTaskSelected));
        OnPropertyChanged(nameof(IsMallTaskSelected));
        OnPropertyChanged(nameof(IsAwardTaskSelected));
        OnPropertyChanged(nameof(IsRoguelikeTaskSelected));
        OnPropertyChanged(nameof(IsReclamationTaskSelected));
        OnPropertyChanged(nameof(IsCustomTaskSelected));
        OnPropertyChanged(nameof(IsPostActionTaskSelected));
        OnPropertyChanged(nameof(ShowPostActionSettingsPanel));
        OnPropertyChanged(nameof(SelectedTaskSettingsViewModel));
        OnPropertyChanged(nameof(CanUseAdvancedSettings));
        OnPropertyChanged(nameof(ShowSettingsModeSwitch));
    }

    private void ResetSettingsModeForSelectedTask()
    {
        SetAdvancedSettingsSelected(false);
    }

    private void SetAdvancedSettingsSelected(bool selected)
    {
        var normalized = selected && CanUseAdvancedSettings;
        var changed = SetProperty(ref _isAdvancedSettingsSelected, normalized, nameof(IsAdvancedSettingsSelected));
        if (changed)
        {
            OnPropertyChanged(nameof(IsGeneralSettingsSelected));
        }

        ApplySettingsModeToTaskModules();
    }

    private void ApplySettingsModeToTaskModules()
    {
        StartUpModule.IsAdvancedMode = _isAdvancedSettingsSelected;
        FightModule.IsAdvancedMode = _isAdvancedSettingsSelected;
        RecruitModule.IsAdvancedMode = _isAdvancedSettingsSelected;
        InfrastModule.IsAdvancedMode = _isAdvancedSettingsSelected;
        MallModule.IsAdvancedMode = _isAdvancedSettingsSelected;
        AwardModule.IsAdvancedMode = _isAdvancedSettingsSelected;
        RoguelikeModule.IsAdvancedMode = _isAdvancedSettingsSelected;
        ReclamationModule.IsAdvancedMode = _isAdvancedSettingsSelected;
        CustomModule.IsAdvancedMode = _isAdvancedSettingsSelected;
    }

    private void OnTypedModulePropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (_suppressModuleAutoSave || !CanEdit || string.IsNullOrWhiteSpace(e.PropertyName))
        {
            return;
        }

        var shouldSave = sender switch
        {
            StartUpTaskModuleViewModel startUp => startUp.IsDirty,
            FightTaskModuleViewModel fight => fight.IsDirty,
            RecruitTaskModuleViewModel recruit => recruit.IsDirty,
            RoguelikeModuleViewModel roguelike => roguelike.IsDirty,
            ReclamationModuleViewModel reclamation => reclamation.IsDirty,
            CustomModuleViewModel custom => custom.IsDirty,
            _ => false,
        };
        if (!shouldSave
            || string.Equals(e.PropertyName, nameof(StartUpTaskModuleViewModel.StatusMessage), StringComparison.Ordinal)
            || string.Equals(e.PropertyName, nameof(StartUpTaskModuleViewModel.LastErrorMessage), StringComparison.Ordinal)
            || string.Equals(e.PropertyName, nameof(StartUpTaskModuleViewModel.IsTaskBound), StringComparison.Ordinal)
            || string.Equals(e.PropertyName, nameof(StartUpTaskModuleViewModel.IsDirty), StringComparison.Ordinal))
        {
            return;
        }

        ScheduleTypedModuleAutoSave();
    }

    private void ScheduleTypedModuleAutoSave()
    {
        lock (_moduleAutoSaveGate)
        {
            _moduleAutoSaveCts?.Cancel();
            _moduleAutoSaveCts?.Dispose();
            _moduleAutoSaveCts = new CancellationTokenSource();
            _ = PersistTypedModulesDebouncedAsync(_moduleAutoSaveCts.Token);
        }
    }

    private async Task PersistTypedModulesDebouncedAsync(CancellationToken cancellationToken)
    {
        try
        {
            await Task.Delay(450, cancellationToken);
            await _queueMutationLock.WaitAsync(cancellationToken);
            try
            {
                await WaitForPendingBindingAsync(cancellationToken);
                if (!CanEdit)
                {
                    return;
                }

                _suppressModuleAutoSave = true;
                _ = await SaveBoundTaskModulesAsync(cancellationToken);
            }
            finally
            {
                _suppressModuleAutoSave = false;
                _queueMutationLock.Release();
            }
        }
        catch (OperationCanceledException)
        {
            // Debounced autosave canceled by newer edits.
        }
        catch (Exception ex)
        {
            await RecordUnhandledExceptionAsync(
                "TaskQueue.AutoSave",
                ex,
                UiErrorCode.TaskParamFlushFailed,
                "TaskQueue autosave failed.");
        }
    }

    public async Task ToggleRunAsync(CancellationToken cancellationToken = default)
    {
        CurrentSessionState = Runtime.SessionService.CurrentState;
        if (CurrentSessionState == SessionState.Running)
        {
            await StopAsync(cancellationToken);
            return;
        }

        await StartAsync(cancellationToken);
    }

    public async Task StartAsync(CancellationToken cancellationToken = default)
    {
        await _runTransitionLock.WaitAsync(cancellationToken);
        try
        {
            ClearVisibleRuntimeLogs();
            if (!Runtime.SessionService.TryBeginRun(TaskQueueRunOwner, out var currentOwner))
            {
                var owner = string.IsNullOrWhiteSpace(currentOwner) ? "Unknown" : currentOwner;
                var message = $"TaskQueue start blocked by active run owner `{owner}`.";
                LastErrorMessage = message;
                AppendStartFailureLog(LastErrorMessage);
                await RecordFailedResultAsync(
                    "TaskQueue.Start.RunOwner",
                    UiOperationResult.Fail(UiErrorCode.TaskQueueEditBlocked, message),
                    cancellationToken);
                return;
            }

            var keepRunOwner = false;
            try
            {
                if (!await EnsureCoreReadyAsync("TaskQueue.Start.CoreWarmup", cancellationToken))
                {
                    return;
                }

                if (!await EnsureConnectedForLinkStartAsync("TaskQueue.Start", cancellationToken))
                {
                    return;
                }

                await WaitForPendingBindingAsync(cancellationToken);
                if (!await SaveBoundTaskModulesAsync(cancellationToken))
                {
                    return;
                }

                StartPrecheckWarningMessage = string.Empty;
                _clearTaskStatusesWhenStopped = false;

                var precheckWarnings = await Runtime.TaskQueueFeatureService.GetStartPrecheckWarningsAsync(cancellationToken);
                if (!precheckWarnings.Success)
                {
                    _ = await ApplyResultAsync(precheckWarnings, "TaskQueue.Start.Precheck", cancellationToken);
                    return;
                }

                var warnings = precheckWarnings.Value ?? [];
                if (warnings.Count > 0)
                {
                    StartPrecheckWarningMessage = string.Join(
                        " ",
                        warnings.Select(static warning => warning.Message));
                    await RecordEventAsync(
                        "TaskQueue.Start.PrecheckWarning",
                        StartPrecheckWarningMessage,
                        cancellationToken);
                }
                else
                {
                    StartPrecheckWarningMessage = string.Empty;
                }

                if (warnings.Any(static warning => string.Equals(
                        warning.Code,
                        UiErrorCode.MallCreditFightDowngraded,
                        StringComparison.Ordinal)))
                {
                    var downgradeResult = await Runtime.TaskQueueFeatureService.ApplyStartPrecheckDowngradesAsync(cancellationToken);
                    if (!downgradeResult.Success)
                    {
                        _ = await ApplyResultAsync(downgradeResult, "TaskQueue.Start.PrecheckApply", cancellationToken);
                        return;
                    }
                }

                if (!await ValidateEnabledTasksBeforeStartAsync(cancellationToken))
                {
                    return;
                }

                RefreshConfigValidationState(Runtime.ConfigurationService.RevalidateCurrentConfig());
                if (HasBlockingConfigIssues)
                {
                    var first = Runtime.ConfigurationService.CurrentValidationIssues.FirstOrDefault(i => i.Blocking);
                    LastErrorMessage = first is null
                        ? "Config validation has blocking issues."
                        : $"{first.Scope}:{first.Code}:{first.Field}:{first.Message}";
                    AppendStartPrecheckWarningMessage(LastErrorMessage);
                    await NavigateToFirstBlockingIssueAsync(first, cancellationToken);
                    AppendStartFailureLog(LastErrorMessage);

                    await RecordConfigValidationFailureAsync(first, cancellationToken);
                    return;
                }

                var appendResult = await Runtime.TaskQueueFeatureService.QueueEnabledTasksAsync(cancellationToken);
                if (!appendResult.Success)
                {
                    var error = UiOperationResult<int>.FromCore(appendResult, "Tasks queued.");
                    _ = await ApplyResultAsync(error, "TaskQueue.Append", cancellationToken);
                    return;
                }

                if (!await ApplyResultAsync(await Runtime.ConnectFeatureService.StartAsync(cancellationToken), "TaskQueue.Start", cancellationToken))
                {
                    return;
                }

                ResetRuntimeLogState();
                ResetAllTaskStatuses();
                CurrentSessionState = Runtime.SessionService.CurrentState;
                _currentRunId = Guid.NewGuid().ToString("N");
                _lastPostActionRunId = string.Empty;
                keepRunOwner = true;
            }
            finally
            {
                if (!keepRunOwner)
                {
                    Runtime.SessionService.EndRun(TaskQueueRunOwner);
                }
            }
        }
        finally
        {
            _runTransitionLock.Release();
        }
    }

    private async Task<bool> EnsureCoreReadyAsync(string scope, CancellationToken cancellationToken)
    {
        if (IsCoreReady)
        {
            return true;
        }

        if (_ensureCoreReadyForExecutionAsync is not null)
        {
            var warmed = await _ensureCoreReadyForExecutionAsync(cancellationToken);
            if (warmed)
            {
                return true;
            }
        }

        LastErrorMessage = string.IsNullOrWhiteSpace(CoreInitializationMessage)
            ? BuildBilingualMessage(
                "核心初始化尚未完成，请稍候重试。",
                "Core initialization is still in progress. Please wait a moment and try again.")
            : CoreInitializationMessage;
        AppendStartPrecheckWarningMessage(LastErrorMessage);
        AppendStartFailureLog(LastErrorMessage);
        await RecordFailedResultAsync(
            scope,
            UiOperationResult.Fail(UiErrorCode.SessionStateNotAllowed, LastErrorMessage),
            cancellationToken);
        return false;
    }

    private string BuildLinkStartStateNotAllowedMessage(SessionState state)
    {
        var zh = $"会话状态 `{state}` 不允许 LinkStart。请先前往“设置 > 连接设置”完成连接。";
        var en = $"Session state `{state}` does not allow LinkStart. Go to Settings > Connection and connect first.";
        return BuildBilingualMessage(zh, en);
    }

    private async Task<bool> EnsureConnectedForLinkStartAsync(string scope, CancellationToken cancellationToken)
    {
        if (!await EnsureCoreReadyAsync(scope, cancellationToken))
        {
            return false;
        }

        CurrentSessionState = Runtime.SessionService.CurrentState;
        Runtime.LogService.Debug($"LinkStart precheck: state={CurrentSessionState}");
        if (CurrentSessionState == SessionState.Connected)
        {
            return true;
        }

        if (CurrentSessionState is SessionState.Running or SessionState.Stopping)
        {
            LastErrorMessage = BuildLinkStartStateNotAllowedMessage(CurrentSessionState);
            AppendStartPrecheckWarningMessage(LastErrorMessage);
            AppendStartFailureLog(LastErrorMessage);
            await RecordFailedResultAsync(
                scope,
                UiOperationResult.Fail(UiErrorCode.SessionStateNotAllowed, LastErrorMessage),
                cancellationToken);
            return false;
        }

        var connectResult = await TryConnectWithRetryAsync(cancellationToken);
        CurrentSessionState = Runtime.SessionService.CurrentState;
        if (connectResult.Success && CurrentSessionState == SessionState.Connected)
        {
            return true;
        }

        var connectMessage = BuildConnectFailureMessage(connectResult);
        LastErrorMessage = connectMessage;
        NavigateToConnectionSettingsIfAvailable();
        AppendStartPrecheckWarningMessage(LastErrorMessage);
        AppendStartFailureLog(LastErrorMessage);
        await RecordFailedResultAsync(
            scope,
            UiOperationResult.Fail(
                UiErrorCode.SessionStateNotAllowed,
                LastErrorMessage,
                connectResult.Error?.Details),
            cancellationToken);
        return false;
    }

    private async Task<UiOperationResult> TryConnectWithRetryAsync(CancellationToken cancellationToken)
    {
        var connectResult = await TryConnectWithCurrentSettingsAsync(cancellationToken);
        if (connectResult.Success)
        {
            return connectResult;
        }

        var address = (_connectionGameSharedState.ConnectAddress ?? string.Empty).Trim();
        var adbExecutable = _connectionGameSharedState.ResolveEffectiveAdbPath(updateStateWhenResolved: true) ?? "adb";

        if (_connectionGameSharedState.RetryOnDisconnected)
        {
            AppendConnectionRecoveryAttemptLog("正在尝试启动模拟器。", "Trying to start the emulator.");
            _ = await TryStartEmulatorForReconnectAsync(cancellationToken);
            connectResult = await TryConnectWithCurrentSettingsAsync(cancellationToken);
            if (connectResult.Success)
            {
                return connectResult;
            }
        }

        if (!string.IsNullOrWhiteSpace(address)
            && AppendConnectionRecoveryAttemptLog("正在尝试通过 ADB 重新连接。", "Trying to reconnect by ADB.")
            && await TryReconnectByAdbAsync(adbExecutable, address, cancellationToken))
        {
            connectResult = await TryConnectWithCurrentSettingsAsync(cancellationToken);
            if (connectResult.Success)
            {
                return connectResult;
            }
        }

        if (_connectionGameSharedState.AllowAdbRestart
            && AppendConnectionRecoveryAttemptLog("正在尝试重启 ADB。", "Trying to restart ADB.")
            && await TryRestartAdbServerAsync(adbExecutable, cancellationToken))
        {
            connectResult = await TryConnectWithCurrentSettingsAsync(cancellationToken);
            if (connectResult.Success)
            {
                return connectResult;
            }
        }

        if (_connectionGameSharedState.AllowAdbHardRestart
            && AppendConnectionRecoveryAttemptLog("正在尝试强制重启 ADB。", "Trying to hard-restart ADB.")
            && await TryHardRestartAdbServerAsync(adbExecutable, cancellationToken))
        {
            connectResult = await TryConnectWithCurrentSettingsAsync(cancellationToken);
        }

        return connectResult;
    }

    private async Task<UiOperationResult> TryConnectWithCurrentSettingsAsync(CancellationToken cancellationToken)
    {
        var effectiveAdbPath = _connectionGameSharedState.ResolveEffectiveAdbPath(updateStateWhenResolved: true);
        var adbPath = string.IsNullOrWhiteSpace(effectiveAdbPath) ? null : effectiveAdbPath;
        var candidates = _connectionGameSharedState.BuildConnectAddressCandidates(includeConfiguredAddress: true);
        Runtime.LogService.Debug(
            $"TaskQueue connect candidates prepared: count={candidates.Count}, config={_connectionGameSharedState.ConnectConfig}, adb={adbPath ?? "<null>"}");
        UiOperationResult? lastFailure = null;

        foreach (var candidate in candidates)
        {
            Runtime.LogService.Debug($"TaskQueue trying connect candidate: {candidate}");
            var result = await Runtime.ConnectFeatureService.ConnectAsync(
                candidate,
                _connectionGameSharedState.ConnectConfig,
                adbPath,
                cancellationToken);
            if (result.Success)
            {
                Runtime.LogService.Debug($"TaskQueue connect succeeded: {candidate}");
                _connectionGameSharedState.ConnectAddress = candidate;
                return result;
            }

            Runtime.LogService.Debug(
                $"TaskQueue connect failed: {candidate}, code={result.Error?.Code}, message={result.Message}");
            lastFailure = result;
        }

        return lastFailure ?? UiOperationResult.Fail(UiErrorCode.UiOperationFailed, "Connection failed.");
    }

    private string BuildConnectFailureMessage(UiOperationResult connectResult)
    {
        var segments = new List<string>
        {
            BuildBilingualMessage(
                "连接失败。请“检查连接设置” -> “尝试重启模拟器与 ADB” -> “重启电脑”。",
                "Connection failed. Check connection settings -> try restarting the emulator and ADB -> reboot the computer."),
        };

        var settingsHint = _connectionGameSharedState.BuildConnectionSettingsHintMessage();
        if (!string.IsNullOrWhiteSpace(settingsHint))
        {
            segments.Add(settingsHint);
        }

        if (!string.IsNullOrWhiteSpace(connectResult.Message)
            && !string.Equals(connectResult.Message, "Connection failed.", StringComparison.OrdinalIgnoreCase))
        {
            segments.Add(BuildBilingualMessage(
                $"连接回调：{connectResult.Message}",
                $"Connection callback: {connectResult.Message}"));
        }

        return string.Join(Environment.NewLine, segments);
    }

    private async Task<bool> TryReconnectByAdbAsync(
        string adbExecutable,
        string address,
        CancellationToken cancellationToken)
    {
        if (string.IsNullOrWhiteSpace(address))
        {
            return false;
        }

        var disconnect = await RunProcessAsync(adbExecutable, $"disconnect {address}", cancellationToken);
        return disconnect.Success;
    }

    private async Task<bool> TryRestartAdbServerAsync(string adbExecutable, CancellationToken cancellationToken)
    {
        var killServer = await RunProcessAsync(adbExecutable, "kill-server", cancellationToken);
        var startServer = await RunProcessAsync(adbExecutable, "start-server", cancellationToken);
        return killServer.Success && startServer.Success;
    }

    private async Task<bool> TryHardRestartAdbServerAsync(string adbExecutable, CancellationToken cancellationToken)
    {
        if (OperatingSystem.IsWindows())
        {
            _ = await RunProcessAsync("taskkill", "/F /IM adb.exe", cancellationToken);
        }

        return await TryRestartAdbServerAsync(adbExecutable, cancellationToken);
    }

    private async Task<bool> TryStartEmulatorForReconnectAsync(CancellationToken cancellationToken)
    {
        var config = Runtime.ConfigurationService.CurrentConfig;
        var startEnabled = TryReadProfileBool(config, ConfigurationKeys.StartEmulator, false);
        if (!startEnabled)
        {
            return false;
        }

        var emulatorPath = TryReadProfileString(config, ConfigurationKeys.EmulatorPath, string.Empty).Trim();
        if (string.IsNullOrWhiteSpace(emulatorPath))
        {
            Runtime.LogService.Warn("Auto start emulator skipped because emulator path is empty.");
            return false;
        }

        try
        {
            var startInfo = new ProcessStartInfo
            {
                FileName = emulatorPath,
                Arguments = TryReadProfileString(config, ConfigurationKeys.EmulatorAddCommand, string.Empty).Trim(),
                UseShellExecute = true,
            };

            _ = Process.Start(startInfo);

            var waitSeconds = Math.Clamp(
                TryReadProfileInt(config, ConfigurationKeys.EmulatorWaitSeconds, 60),
                0,
                600);
            if (waitSeconds > 0)
            {
                await Task.Delay(TimeSpan.FromSeconds(waitSeconds), cancellationToken);
            }

            return true;
        }
        catch (Exception ex)
        {
            Runtime.LogService.Warn($"Auto start emulator failed: {ex.Message}");
            return false;
        }
    }

    private static async Task<(bool Success, string Message)> RunProcessAsync(
        string fileName,
        string arguments,
        CancellationToken cancellationToken)
    {
        try
        {
            using var process = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = fileName,
                    Arguments = arguments,
                    UseShellExecute = false,
                    RedirectStandardError = true,
                    RedirectStandardOutput = true,
                    CreateNoWindow = true,
                },
            };

            if (!process.Start())
            {
                return (false, $"Failed to start process `{fileName}`.");
            }

            await process.WaitForExitAsync(cancellationToken);
            if (process.ExitCode == 0)
            {
                return (true, "OK");
            }

            var error = await process.StandardError.ReadToEndAsync(cancellationToken);
            if (string.IsNullOrWhiteSpace(error))
            {
                error = await process.StandardOutput.ReadToEndAsync(cancellationToken);
            }

            return (false, $"ExitCode={process.ExitCode}; {error}".Trim());
        }
        catch (Exception ex)
        {
            return (false, ex.Message);
        }
    }

    private void NavigateToConnectionSettingsIfAvailable()
    {
        _navigateToSettingsSection?.Invoke("Connect");
    }

    private static string BuildSessionStateNotAllowedMessage(SessionState state, string actionZh, string actionEn)
    {
        return BuildBilingualMessage(
            $"会话状态 `{state}` 不允许{actionZh}。",
            $"Session state `{state}` does not allow {actionEn}.");
    }

    private static string BuildSessionAlreadyNonRunningMessage(SessionState state)
    {
        return BuildBilingualMessage(
            $"会话状态 `{state}` 已处于非运行状态。",
            $"Session state `{state}` is already non-running.");
    }

    private static string BuildRunOwnerBlockedMessage(string actionZh, string actionEn, string owner)
    {
        return BuildBilingualMessage(
            $"当前运行所有者 `{owner}` 正在占用，会阻止{actionZh}。",
            $"Active run owner `{owner}` blocks {actionEn}.");
    }

    private static string BuildBilingualMessage(string zh, string en)
    {
        return $"{zh}{Environment.NewLine}{en}";
    }

    private async Task RunAccountSwitchManualAsync(CancellationToken cancellationToken)
    {
        await _runTransitionLock.WaitAsync(cancellationToken);
        try
        {
            CurrentSessionState = Runtime.SessionService.CurrentState;
            if (CurrentSessionState != SessionState.Connected)
            {
                LastErrorMessage = BuildSessionStateNotAllowedMessage(CurrentSessionState, "账号切换", "account switch");
                await RecordFailedResultAsync(
                    "TaskQueue.AccountSwitch",
                    UiOperationResult.Fail(UiErrorCode.SessionStateNotAllowed, LastErrorMessage),
                    cancellationToken);
                return;
            }

            if (!Runtime.SessionService.TryBeginRun(TaskQueueRunOwner, out var currentOwner))
            {
                var owner = string.IsNullOrWhiteSpace(currentOwner) ? "Unknown" : currentOwner;
                var message = BuildRunOwnerBlockedMessage("账号切换", "account switch", owner);
                LastErrorMessage = message;
                await RecordFailedResultAsync(
                    "TaskQueue.AccountSwitch.RunOwner",
                    UiOperationResult.Fail(UiErrorCode.TaskQueueEditBlocked, message),
                    cancellationToken);
                return;
            }

            var keepRunOwner = false;
            try
            {
                await WaitForPendingBindingAsync(cancellationToken);
                if (!await SaveBoundTaskModulesAsync(cancellationToken))
                {
                    return;
                }

                if (!Runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
                {
                    LastErrorMessage = BuildBilingualMessage("当前配置档不存在。", "Current profile is missing.");
                    await RecordFailedResultAsync(
                        "TaskQueue.AccountSwitch.Profile",
                        UiOperationResult.Fail(UiErrorCode.ProfileMissing, LastErrorMessage),
                        cancellationToken);
                    return;
                }

                var startupIndex = ResolveAccountSwitchStartUpIndex(profile);
                if (startupIndex < 0 || startupIndex >= profile.TaskQueue.Count)
                {
                    LastErrorMessage = BuildBilingualMessage(
                        "账号切换所需的 StartUp 任务不存在。",
                        "StartUp task is missing for account switch.");
                    await RecordFailedResultAsync(
                        "TaskQueue.AccountSwitch.Task",
                        UiOperationResult.Fail(UiErrorCode.TaskNotFound, LastErrorMessage),
                        cancellationToken);
                    return;
                }

                var enabledSnapshot = profile.TaskQueue.Select(task => task.IsEnabled).ToArray();
                try
                {
                    for (var index = 0; index < profile.TaskQueue.Count; index++)
                    {
                        profile.TaskQueue[index].IsEnabled = index == startupIndex;
                    }

                    var appendResult = await Runtime.TaskQueueFeatureService.QueueEnabledTasksAsync(cancellationToken);
                    if (!appendResult.Success)
                    {
                        var error = UiOperationResult<int>.FromCore(appendResult, "Task queued.");
                        _ = await ApplyResultAsync(error, "TaskQueue.AccountSwitch.Append", cancellationToken);
                        return;
                    }
                }
                finally
                {
                    for (var index = 0; index < profile.TaskQueue.Count && index < enabledSnapshot.Length; index++)
                    {
                        profile.TaskQueue[index].IsEnabled = enabledSnapshot[index];
                    }
                }

                if (!await ApplyResultAsync(
                        await Runtime.ConnectFeatureService.StartAsync(cancellationToken),
                        "TaskQueue.AccountSwitch.Start",
                        cancellationToken))
                {
                    return;
                }

                CurrentSessionState = Runtime.SessionService.CurrentState;
                _currentRunId = Guid.NewGuid().ToString("N");
                _lastPostActionRunId = string.Empty;
                keepRunOwner = true;
            }
            finally
            {
                if (!keepRunOwner)
                {
                    Runtime.SessionService.EndRun(TaskQueueRunOwner);
                }
            }
        }
        finally
        {
            _runTransitionLock.Release();
        }
    }

    private int ResolveAccountSwitchStartUpIndex(UnifiedProfile profile)
    {
        if (SelectedTask is not null)
        {
            var selectedIndex = Tasks.IndexOf(SelectedTask);
            if (selectedIndex >= 0
                && selectedIndex < profile.TaskQueue.Count
                && string.Equals(
                    TaskModuleTypes.Normalize(profile.TaskQueue[selectedIndex].Type),
                    TaskModuleTypes.StartUp,
                    StringComparison.OrdinalIgnoreCase))
            {
                return selectedIndex;
            }
        }

        for (var index = 0; index < profile.TaskQueue.Count; index++)
        {
            if (string.Equals(
                    TaskModuleTypes.Normalize(profile.TaskQueue[index].Type),
                    TaskModuleTypes.StartUp,
                    StringComparison.OrdinalIgnoreCase))
            {
                return index;
            }
        }

        return -1;
    }

    public async Task StopAsync(CancellationToken cancellationToken = default)
    {
        await _runTransitionLock.WaitAsync(cancellationToken);
        try
        {
            CurrentSessionState = Runtime.SessionService.CurrentState;
            if (CurrentSessionState != SessionState.Running)
            {
                LastErrorMessage = BuildSessionAlreadyNonRunningMessage(CurrentSessionState);
                await RecordFailedResultAsync(
                    "TaskQueue.Stop",
                    UiOperationResult.Fail(UiErrorCode.OperationAlreadyStopped, LastErrorMessage),
                    cancellationToken);
                SyncStoppedUiStateIfSessionNotActive();

                return;
            }

            var currentOwner = Runtime.SessionService.CurrentRunOwner;
            if (!string.IsNullOrWhiteSpace(currentOwner) && !Runtime.SessionService.IsRunOwner(TaskQueueRunOwner))
            {
                var owner = currentOwner;
                LastErrorMessage = BuildRunOwnerBlockedMessage("停止任务", "task stop", owner);
                await RecordFailedResultAsync(
                    "TaskQueue.Stop.RunOwner",
                    UiOperationResult.Fail(UiErrorCode.TaskQueueEditBlocked, LastErrorMessage),
                    cancellationToken);
                return;
            }

            await WaitForPendingBindingAsync(cancellationToken);
            if (!await SaveBoundTaskModulesAsync(cancellationToken))
            {
                return;
            }

            if (ShouldDelayStopUntilRoguelikeCombatComplete())
            {
                await WaitUntilRoguelikeCombatCompleteAsync(cancellationToken);
                CurrentSessionState = Runtime.SessionService.CurrentState;
                if (CurrentSessionState != SessionState.Running)
                {
                    SyncStoppedUiStateIfSessionNotActive();
                    return;
                }
            }

            _clearTaskStatusesWhenStopped = true;
            if (!await ApplyResultAsync(await Runtime.ConnectFeatureService.StopAsync(cancellationToken), "TaskQueue.Stop", cancellationToken))
            {
                CurrentSessionState = Runtime.SessionService.CurrentState;
                if (CurrentSessionState is SessionState.Running or SessionState.Stopping)
                {
                    _clearTaskStatusesWhenStopped = false;
                }
                SyncStoppedUiStateIfSessionNotActive();

                return;
            }

            ResetAllTaskStatuses();
            CurrentSessionState = Runtime.SessionService.CurrentState;
            SyncStoppedUiStateIfSessionNotActive();
        }
        finally
        {
            _runTransitionLock.Release();
        }
    }

    public async Task WaitAndStopAsync(CancellationToken cancellationToken = default)
    {
        await _runTransitionLock.WaitAsync(cancellationToken);
        try
        {
            if (IsWaitingForStop)
            {
                LastErrorMessage = BuildBilingualMessage(
                    "等待并停止流程已在执行中。",
                    "WaitAndStop is already in progress.");
                await RecordFailedResultAsync(
                    "TaskQueue.WaitAndStop",
                    UiOperationResult.Fail(UiErrorCode.OperationAlreadyRunning, LastErrorMessage),
                    cancellationToken);
                return;
            }

            CurrentSessionState = Runtime.SessionService.CurrentState;
            if (CurrentSessionState != SessionState.Running)
            {
                LastErrorMessage = BuildSessionAlreadyNonRunningMessage(CurrentSessionState);
                await RecordFailedResultAsync(
                    "TaskQueue.WaitAndStop",
                    UiOperationResult.Fail(UiErrorCode.OperationAlreadyStopped, LastErrorMessage),
                    cancellationToken);
                SyncStoppedUiStateIfSessionNotActive();

                return;
            }

            var currentOwner = Runtime.SessionService.CurrentRunOwner;
            if (!string.IsNullOrWhiteSpace(currentOwner) && !Runtime.SessionService.IsRunOwner(TaskQueueRunOwner))
            {
                var owner = currentOwner;
                LastErrorMessage = BuildRunOwnerBlockedMessage("等待并停止", "wait-stop", owner);
                await RecordFailedResultAsync(
                    "TaskQueue.WaitAndStop.RunOwner",
                    UiOperationResult.Fail(UiErrorCode.TaskQueueEditBlocked, LastErrorMessage),
                    cancellationToken);
                return;
            }

            IsWaitingForStop = true;
            _clearTaskStatusesWhenStopped = true;
            if (!await ApplyResultAsync(
                    await Runtime.ConnectFeatureService.WaitAndStopAsync(TimeSpan.FromSeconds(15), cancellationToken),
                    "TaskQueue.WaitAndStop",
                    cancellationToken))
            {
                CurrentSessionState = Runtime.SessionService.CurrentState;
                if (CurrentSessionState is SessionState.Running or SessionState.Stopping)
                {
                    _clearTaskStatusesWhenStopped = false;
                }
                SyncStoppedUiStateIfSessionNotActive();

                return;
            }

            ResetAllTaskStatuses();
            CurrentSessionState = Runtime.SessionService.CurrentState;
            SyncStoppedUiStateIfSessionNotActive();
        }
        finally
        {
            IsWaitingForStop = false;
            _runTransitionLock.Release();
        }
    }

    public async Task ReloadOverlayTargetsAsync(CancellationToken cancellationToken = default)
    {
        var targets = await ApplyResultAsync(
            await Runtime.OverlayFeatureService.GetOverlayTargetsAsync(cancellationToken),
            "Overlay.QueryTargets",
            cancellationToken);

        if (targets is null)
        {
            return;
        }

        OverlayTargets.Clear();
        foreach (var target in targets)
        {
            OverlayTargets.Add(target);
        }

        if (OverlayTargets.Count > 0)
        {
            var resolvedSelection = OverlayTargetPersistence.ResolveSelection(
                OverlayTargets,
                Runtime.ConfigurationService.CurrentConfig.GlobalValues,
                _overlaySharedState.SelectedTargetId);
            if (OverlayTargetPersistence.ShouldDefaultToPreview(
                Runtime.ConfigurationService.CurrentConfig.GlobalValues,
                _overlaySharedState.SelectedTargetId))
            {
                resolvedSelection = OverlayTargets.FirstOrDefault(t => string.Equals(t.Id, "preview", StringComparison.Ordinal))
                    ?? resolvedSelection;
            }

            SelectedOverlayTarget = resolvedSelection
                ?? OverlayTargets.FirstOrDefault(t => t.IsPrimary)
                ?? OverlayTargets[0];
        }

        if (!string.IsNullOrWhiteSpace(_overlaySharedState.StatusMessage))
        {
            OverlayStatusText = _overlaySharedState.StatusMessage;
        }
        else
        {
            var snapshotResult = await Runtime.PlatformCapabilityService.GetSnapshotAsync(cancellationToken);
            if (snapshotResult.Success && snapshotResult.Value is not null)
            {
                OverlayStatusText = BuildCapabilityLine(PlatformCapabilityId.Overlay, snapshotResult.Value.Overlay);
            }
            else
            {
                OverlayStatusText = PlatformCapabilityTextMap.FormatSnapshotUnavailable(
                    Texts.Language,
                    snapshotResult.Message,
                    _localizationFallbackReporter);
            }
        }

        SyncOverlayPresentationFromSharedState();
    }

    public async Task PickOverlayTargetWithDialogAsync(CancellationToken cancellationToken = default)
    {
        if (OverlayTargets.Count == 0)
        {
            await ReloadOverlayTargetsAsync(cancellationToken);
        }

        if (OverlayTargets.Count == 0)
        {
            LastErrorMessage = BuildBilingualMessage("当前没有可用的 Overlay 目标。", "No overlay target is available.");
            return;
        }

        var request = new ProcessPickerDialogRequest(
            Title: "Overlay Target Picker",
            Items: OverlayTargets.Select(t => new ProcessPickerItem(t.Id, t.DisplayName, t.IsPrimary)).ToArray(),
            SelectedId: SelectedOverlayTarget?.Id,
            ConfirmText: "Select",
            CancelText: "Cancel");
        var dialogResult = await _dialogService.ShowProcessPickerAsync(request, "TaskQueue.Overlay.PickTarget", cancellationToken);
        if (dialogResult.Return != DialogReturnSemantic.Confirm || dialogResult.Payload is null)
        {
            StatusMessage = dialogResult.Return == DialogReturnSemantic.Cancel
                ? "Overlay target selection cancelled."
                : "Overlay target picker closed.";
            return;
        }

        await ApplyOverlayTargetAsync(dialogResult.Payload.SelectedId, cancellationToken);
    }

    public async Task ApplyOverlayTargetAsync(string targetId, CancellationToken cancellationToken = default)
    {
        if (string.IsNullOrWhiteSpace(targetId))
        {
            LastErrorMessage = BuildBilingualMessage("Overlay 目标 ID 为空。", "Overlay target id is missing.");
            return;
        }

        if (!await SelectAndPersistOverlayTargetAsync(targetId, cancellationToken))
        {
            return;
        }
    }

    public async Task ToggleOverlayAsync(CancellationToken cancellationToken = default)
    {
        if (!await SelectAndPersistOverlayTargetAsync(SelectedOverlayTarget?.Id ?? "preview", cancellationToken))
        {
            return;
        }

        var requestedVisible = !OverlayVisible;
        var visibleResult = await Runtime.OverlayFeatureService.ToggleOverlayVisibilityAsync(requestedVisible, cancellationToken);
        if (!await ApplyResultAsync(visibleResult, "Overlay.Toggle", cancellationToken))
        {
            return;
        }

        OverlayVisible = requestedVisible;
    }

    private string BuildCapabilityLine(PlatformCapabilityId capability, PlatformCapabilityStatus status)
    {
        return PlatformCapabilityTextMap.FormatCapabilityLine(
            Texts.Language,
            capability,
            status,
            _localizationFallbackReporter);
    }

    private void RefreshConfigValidationState(IReadOnlyList<ConfigValidationIssue> issues)
    {
        BlockingConfigIssueCount = issues.Count(i => i.Blocking);
        HasBlockingConfigIssues = BlockingConfigIssueCount > 0;
    }

    private void AppendStartPrecheckWarningMessage(string? message)
    {
        var summary = BuildStartPrecheckSummaryMessage(message);
        if (string.IsNullOrWhiteSpace(summary))
        {
            return;
        }

        if (string.IsNullOrWhiteSpace(StartPrecheckWarningMessage))
        {
            StartPrecheckWarningMessage = summary;
            return;
        }

        if (StartPrecheckWarningMessage.Contains(summary, StringComparison.Ordinal))
        {
            return;
        }

        StartPrecheckWarningMessage = $"{StartPrecheckWarningMessage} {summary}";
    }

    private void AppendStartFailureLog(string? message)
    {
        if (string.IsNullOrWhiteSpace(message))
        {
            return;
        }

        AppendLogEntry(
            timestamp: DateTimeOffset.Now,
            content: $"Link Start failed: {message}",
            level: "ERROR",
            splitMode: TaskQueueLogSplitMode.None,
            updateThumbnail: false);
    }

    private string? BuildStartPrecheckSummaryMessage(string? message)
    {
        if (string.IsNullOrWhiteSpace(message))
        {
            return null;
        }

        var trimmed = message.Trim();
        if (trimmed.Contains("连接失败", StringComparison.Ordinal)
            || trimmed.Contains("Connection failed", StringComparison.OrdinalIgnoreCase))
        {
            var settingsHint = _connectionGameSharedState.BuildConnectionSettingsHintMessage();
            if (!string.IsNullOrWhiteSpace(settingsHint))
            {
                if (settingsHint.Contains("连接地址为空", StringComparison.Ordinal)
                    || settingsHint.Contains("Connection address is empty", StringComparison.OrdinalIgnoreCase))
                {
                    return BuildBilingualMessage(
                        "连接失败，请检查当前配置的连接地址。",
                        "Connection failed. Check the connection address of the current profile.");
                }

                if (settingsHint.Contains("ADB 路径", StringComparison.Ordinal)
                    || settingsHint.Contains("ADB path", StringComparison.OrdinalIgnoreCase)
                    || settingsHint.Contains("当前系统是", StringComparison.Ordinal)
                    || settingsHint.Contains("Current system is", StringComparison.OrdinalIgnoreCase))
                {
                    return BuildBilingualMessage(
                        "连接失败，请检查当前配置的 ADB 路径。",
                        "Connection failed. Check the ADB path of the current profile.");
                }
            }

            return BuildBilingualMessage(
                "连接失败，请检查当前配置的连接设置。",
                "Connection failed. Check the connection settings of the current profile.");
        }

        var firstLine = trimmed
            .Split(new[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
            .FirstOrDefault();
        if (string.IsNullOrWhiteSpace(firstLine))
        {
            return null;
        }

        return firstLine.Length > 160
            ? $"{firstLine[..160].TrimEnd()}..."
            : firstLine;
    }

    private bool AppendConnectionRecoveryAttemptLog(string zh, string en)
    {
        AppendSystemLog(
            BuildBilingualMessage(
                $"连接失败，{zh}",
                $"Connection failed. {en}"),
            "WARN");
        return true;
    }

    private void ResetSelectedTaskValidationSummary()
    {
        SelectedTaskValidationIssueCount = 0;
        SelectedTaskHasBlockingValidationIssues = false;
        SelectedTaskValidationSummary = string.Empty;
    }

    private async Task RefreshSelectedTaskValidationSummaryAsync(int index, CancellationToken cancellationToken)
    {
        var result = await Runtime.TaskQueueFeatureService.ValidateTaskAsync(index, cancellationToken);
        if (!result.Success || result.Value is null)
        {
            SelectedTaskValidationIssueCount = 0;
            SelectedTaskHasBlockingValidationIssues = false;
            SelectedTaskValidationSummary = Texts.GetOrDefault(
                "TaskQueue.Validation.LoadFailed",
                "Failed to load validation report.");
            LastErrorMessage = result.Message;
            return;
        }

        UpdateSelectedTaskValidationSummary(result.Value);
    }

    private void UpdateSelectedTaskValidationSummary(TaskValidationReport report)
    {
        var blockingCount = report.Issues.Count(i => i.Blocking);
        SelectedTaskValidationIssueCount = blockingCount;
        SelectedTaskHasBlockingValidationIssues = blockingCount > 0;

        if (blockingCount == 0)
        {
            SelectedTaskValidationSummary = string.Empty;
            return;
        }

        SelectedTaskValidationSummary = string.Format(
            Texts.GetOrDefault("TaskQueue.Validation.BlockingCount", "{0} blocking issue(s)."),
            blockingCount);
    }

    private async Task<bool> ValidateEnabledTasksBeforeStartAsync(CancellationToken cancellationToken)
    {
        for (var index = 0; index < Tasks.Count; index++)
        {
            if (!Tasks[index].IsEnabled)
            {
                continue;
            }

            var result = await Runtime.TaskQueueFeatureService.ValidateTaskAsync(index, cancellationToken);
            if (!result.Success || result.Value is null)
            {
                LastErrorMessage = result.Message;
                AppendStartPrecheckWarningMessage(LastErrorMessage);
                await SelectTaskByIndexAsync(index, cancellationToken);
                AppendStartFailureLog(LastErrorMessage);
                await RecordFailedResultAsync(
                    "TaskQueue.ValidateTask",
                    UiOperationResult.Fail(
                        result.Error?.Code ?? UiErrorCode.TaskValidationFailed,
                        result.Message,
                        result.Error?.Details),
                    cancellationToken);
                return false;
            }

            var report = result.Value;
            if (SelectedTask is not null && Tasks.IndexOf(SelectedTask) == index)
            {
                UpdateSelectedTaskValidationSummary(report);
            }

            if (!report.HasBlockingIssues)
            {
                continue;
            }

            var firstBlocking = report.Issues.First(i => i.Blocking);
            var issueDetail = $"{firstBlocking.Code}:{firstBlocking.Field}:{firstBlocking.Message}";
            LastErrorMessage = string.Format(
                Texts.GetOrDefault(
                    "TaskQueue.Error.BlockingValidation",
                    "Task `{0}` blocked by validation: {1}"),
                report.TaskName,
                issueDetail);
            AppendStartPrecheckWarningMessage(LastErrorMessage);
            await SelectTaskByIndexAsync(index, cancellationToken);
            AppendStartFailureLog(LastErrorMessage);
            await RecordFailedResultAsync(
                "TaskQueue.ValidateTask",
                UiOperationResult.Fail(
                    UiErrorCode.TaskValidationFailed,
                    LastErrorMessage,
                    issueDetail),
                cancellationToken);
            return false;
        }

        return true;
    }

    private async Task NavigateToFirstBlockingIssueAsync(
        ConfigValidationIssue? issue,
        CancellationToken cancellationToken)
    {
        if (issue?.TaskIndex is int taskIndex)
        {
            await SelectTaskByIndexAsync(taskIndex, cancellationToken);
            return;
        }

        if (!string.IsNullOrWhiteSpace(issue?.TaskName))
        {
            for (var index = 0; index < Tasks.Count; index++)
            {
                if (string.Equals(Tasks[index].Name, issue.TaskName, StringComparison.OrdinalIgnoreCase))
                {
                    await SelectTaskByIndexAsync(index, cancellationToken);
                    return;
                }
            }
        }

        for (var index = 0; index < Tasks.Count; index++)
        {
            if (!Tasks[index].IsEnabled)
            {
                continue;
            }

            var result = await Runtime.TaskQueueFeatureService.ValidateTaskAsync(index, cancellationToken);
            if (result.Success && result.Value?.HasBlockingIssues == true)
            {
                await SelectTaskByIndexAsync(index, cancellationToken);
                return;
            }
        }
    }

    private Task BindSelectedTaskAsync(CancellationToken cancellationToken = default)
    {
        int expectedVersion;
        lock (_pendingBindingGate)
        {
            expectedVersion = _pendingBindingVersion;
        }

        return BindSelectedTaskCoreAsync(expectedVersion, cancellationToken);
    }

    private async Task BindSelectedTaskCoreAsync(int expectedVersion, CancellationToken cancellationToken = default)
    {
        await _taskBindingLock.WaitAsync(cancellationToken);
        try
        {
            if (!await SaveBoundTaskModulesAsync(cancellationToken))
            {
                return;
            }

            if (IsBindingStale(expectedVersion, cancellationToken))
            {
                return;
            }

            if (SelectedTask is null)
            {
                ClearTaskModuleBindings();
                ResetSelectedTaskValidationSummary();
                return;
            }

            var index = Tasks.IndexOf(SelectedTask);
            if (index < 0)
            {
                ResetSelectedTaskValidationSummary();
                return;
            }

            var moduleType = TaskModuleTypes.Normalize(SelectedTask.Type);
            if (string.Equals(moduleType, TaskModuleTypes.StartUp, StringComparison.OrdinalIgnoreCase))
            {
                await StartUpModule.BindAsync(index, cancellationToken);
                if (IsBindingStale(expectedVersion, cancellationToken))
                {
                    return;
                }

                FightModule.ClearBinding();
                RecruitModule.ClearBinding();
                InfrastModule.ClearBinding();
                MallModule.ClearBinding();
                AwardModule.ClearBinding();
                RoguelikeModule.ClearBinding();
                ReclamationModule.ClearBinding();
                CustomModule.ClearBinding();
                await RefreshSelectedTaskValidationSummaryAsync(index, cancellationToken);
                return;
            }

            if (string.Equals(moduleType, TaskModuleTypes.Fight, StringComparison.OrdinalIgnoreCase))
            {
                await FightModule.BindAsync(index, cancellationToken);
                if (IsBindingStale(expectedVersion, cancellationToken))
                {
                    return;
                }

                StartUpModule.ClearBinding();
                RecruitModule.ClearBinding();
                InfrastModule.ClearBinding();
                MallModule.ClearBinding();
                AwardModule.ClearBinding();
                RoguelikeModule.ClearBinding();
                ReclamationModule.ClearBinding();
                CustomModule.ClearBinding();
                await RefreshSelectedTaskValidationSummaryAsync(index, cancellationToken);
                return;
            }

            if (string.Equals(moduleType, TaskModuleTypes.Recruit, StringComparison.OrdinalIgnoreCase))
            {
                await RecruitModule.BindAsync(index, cancellationToken);
                if (IsBindingStale(expectedVersion, cancellationToken))
                {
                    return;
                }

                StartUpModule.ClearBinding();
                FightModule.ClearBinding();
                InfrastModule.ClearBinding();
                MallModule.ClearBinding();
                AwardModule.ClearBinding();
                RoguelikeModule.ClearBinding();
                ReclamationModule.ClearBinding();
                CustomModule.ClearBinding();
                await RefreshSelectedTaskValidationSummaryAsync(index, cancellationToken);
                return;
            }

            if (string.Equals(moduleType, TaskModuleTypes.Roguelike, StringComparison.OrdinalIgnoreCase))
            {
                await RoguelikeModule.BindAsync(index, cancellationToken);
                if (IsBindingStale(expectedVersion, cancellationToken))
                {
                    return;
                }

                StartUpModule.ClearBinding();
                FightModule.ClearBinding();
                RecruitModule.ClearBinding();
                InfrastModule.ClearBinding();
                MallModule.ClearBinding();
                AwardModule.ClearBinding();
                ReclamationModule.ClearBinding();
                CustomModule.ClearBinding();
                await RefreshSelectedTaskValidationSummaryAsync(index, cancellationToken);
                return;
            }

            if (string.Equals(moduleType, TaskModuleTypes.Reclamation, StringComparison.OrdinalIgnoreCase))
            {
                await ReclamationModule.BindAsync(index, cancellationToken);
                if (IsBindingStale(expectedVersion, cancellationToken))
                {
                    return;
                }

                StartUpModule.ClearBinding();
                FightModule.ClearBinding();
                RecruitModule.ClearBinding();
                InfrastModule.ClearBinding();
                MallModule.ClearBinding();
                AwardModule.ClearBinding();
                RoguelikeModule.ClearBinding();
                CustomModule.ClearBinding();
                await RefreshSelectedTaskValidationSummaryAsync(index, cancellationToken);
                return;
            }

            if (string.Equals(moduleType, TaskModuleTypes.Custom, StringComparison.OrdinalIgnoreCase))
            {
                await CustomModule.BindAsync(index, cancellationToken);
                if (IsBindingStale(expectedVersion, cancellationToken))
                {
                    return;
                }

                StartUpModule.ClearBinding();
                FightModule.ClearBinding();
                RecruitModule.ClearBinding();
                InfrastModule.ClearBinding();
                MallModule.ClearBinding();
                AwardModule.ClearBinding();
                RoguelikeModule.ClearBinding();
                ReclamationModule.ClearBinding();
                await RefreshSelectedTaskValidationSummaryAsync(index, cancellationToken);
                return;
            }

            StartUpModule.ClearBinding();
            FightModule.ClearBinding();
            RecruitModule.ClearBinding();
            RoguelikeModule.ClearBinding();
            ReclamationModule.ClearBinding();
            CustomModule.ClearBinding();

            var paramsResult = await Runtime.TaskQueueFeatureService.GetTaskParamsAsync(index, cancellationToken);
            if (!paramsResult.Success || paramsResult.Value is null)
            {
                LastErrorMessage = paramsResult.Message;
                ResetSelectedTaskValidationSummary();
                return;
            }

            if (string.Equals(moduleType, TaskModuleTypes.Infrast, StringComparison.OrdinalIgnoreCase))
            {
                await InfrastModule.BindAsync(index, paramsResult.Value, cancellationToken);
                if (IsBindingStale(expectedVersion, cancellationToken))
                {
                    return;
                }
            }
            else
            {
                InfrastModule.ClearBinding();
            }

            if (string.Equals(moduleType, TaskModuleTypes.Mall, StringComparison.OrdinalIgnoreCase))
            {
                await MallModule.BindAsync(index, paramsResult.Value, cancellationToken);
                if (IsBindingStale(expectedVersion, cancellationToken))
                {
                    return;
                }
            }
            else
            {
                MallModule.ClearBinding();
            }

            if (string.Equals(moduleType, TaskModuleTypes.Award, StringComparison.OrdinalIgnoreCase))
            {
                await AwardModule.BindAsync(index, paramsResult.Value, cancellationToken);
                if (IsBindingStale(expectedVersion, cancellationToken))
                {
                    return;
                }
            }
            else
            {
                AwardModule.ClearBinding();
            }

            await RefreshSelectedTaskValidationSummaryAsync(index, cancellationToken);
        }
        finally
        {
            _taskBindingLock.Release();
        }
    }

    private void ScheduleBindSelectedTask()
    {
        lock (_pendingBindingGate)
        {
            _pendingBindingVersion++;
            var expectedVersion = _pendingBindingVersion;

            _pendingBindingCts?.Cancel();
            _pendingBindingCts?.Dispose();
            _pendingBindingCts = new CancellationTokenSource();

            _pendingBindingTask = ExecuteTrackedBindingAsync(expectedVersion, _pendingBindingCts.Token);
        }
    }

    private async Task ExecuteTrackedBindingAsync(int expectedVersion, CancellationToken cancellationToken)
    {
        try
        {
            await BindSelectedTaskCoreAsync(expectedVersion, cancellationToken);
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            // no-op
        }
        catch (Exception ex)
        {
            await RecordUnhandledExceptionAsync(
                "TaskQueue.BindSelectedTask",
                ex,
                UiErrorCode.TaskLoadFailed,
                "Bind selected task failed.");
        }
    }

    private bool IsBindingVersionCurrent(int expectedVersion)
    {
        lock (_pendingBindingGate)
        {
            return expectedVersion == _pendingBindingVersion;
        }
    }

    private bool IsBindingStale(int expectedVersion, CancellationToken cancellationToken)
    {
        return cancellationToken.IsCancellationRequested || !IsBindingVersionCurrent(expectedVersion);
    }

    private void ClearTaskModuleBindings()
    {
        StartUpModule.ClearBinding();
        FightModule.ClearBinding();
        RecruitModule.ClearBinding();
        InfrastModule.ClearBinding();
        MallModule.ClearBinding();
        AwardModule.ClearBinding();
        RoguelikeModule.ClearBinding();
        ReclamationModule.ClearBinding();
        CustomModule.ClearBinding();
        ResetSelectedTaskValidationSummary();
    }

    private async Task<bool> SaveBoundTaskModulesAsync(CancellationToken cancellationToken = default)
    {
        if (!await StartUpModule.SaveIfDirtyAsync(cancellationToken))
        {
            LastErrorMessage = StartUpModule.LastErrorMessage;
            return false;
        }

        if (!await FightModule.SaveIfDirtyAsync(cancellationToken))
        {
            LastErrorMessage = FightModule.LastErrorMessage;
            return false;
        }

        if (!await RecruitModule.SaveIfDirtyAsync(cancellationToken))
        {
            LastErrorMessage = RecruitModule.LastErrorMessage;
            return false;
        }

        if (!await RoguelikeModule.SaveIfDirtyAsync(cancellationToken))
        {
            LastErrorMessage = RoguelikeModule.LastErrorMessage;
            return false;
        }

        if (!await ReclamationModule.SaveIfDirtyAsync(cancellationToken))
        {
            LastErrorMessage = ReclamationModule.LastErrorMessage;
            return false;
        }

        if (!await CustomModule.SaveIfDirtyAsync(cancellationToken))
        {
            LastErrorMessage = CustomModule.LastErrorMessage;
            return false;
        }

        if (!await InfrastModule.FlushPendingChangesAsync(cancellationToken))
        {
            LastErrorMessage = InfrastModule.LastErrorMessage;
            return false;
        }

        if (!await MallModule.FlushPendingChangesAsync(cancellationToken))
        {
            LastErrorMessage = MallModule.LastErrorMessage;
            return false;
        }

        if (!await AwardModule.FlushPendingChangesAsync(cancellationToken))
        {
            LastErrorMessage = AwardModule.LastErrorMessage;
            return false;
        }

        if (!await PostActionModule.FlushPendingChangesAsync(cancellationToken))
        {
            LastErrorMessage = PostActionModule.LastErrorMessage;
            return false;
        }

        var flushResult = await Runtime.TaskQueueFeatureService.FlushTaskParamWritesAsync(cancellationToken);
        if (!flushResult.Success)
        {
            LastErrorMessage = flushResult.Message;
            await RecordFailedResultAsync("TaskQueue.FlushParams", flushResult, cancellationToken);
            return false;
        }

        return true;
    }

    private void UpdateDownloadLog(DateTimeOffset timestamp, string level, string message)
    {
        if (message.Contains("download", StringComparison.OrdinalIgnoreCase)
            || message.Contains("下载", StringComparison.Ordinal))
        {
            DownloadLogEntry = new TaskQueueLogEntryViewModel(
                timestamp.ToLocalTime().ToString("HH:mm:ss"),
                message,
            NormalizeLogLevel(level));
        }
    }

    private void ClearVisibleRuntimeLogs()
    {
        foreach (var card in LogCards)
        {
            card.Thumbnail = null;
        }

        LogCards.Clear();
        OverlayLogs.Clear();
        DownloadLogEntry = new TaskQueueLogEntryViewModel(string.Empty, string.Empty, "INFO");
        LastRuntimeStatus = null;
    }

    public void AppendSystemLog(string message, string level = "INFO")
    {
        if (string.IsNullOrWhiteSpace(message))
        {
            return;
        }

        AppendLogEntry(
            DateTimeOffset.UtcNow,
            message,
            NormalizeLogLevel(level),
            TaskQueueLogSplitMode.None,
            updateThumbnail: false);
    }

    private string NormalizeLogLevel(string? level)
    {
        if (string.IsNullOrWhiteSpace(level))
        {
            return "INFO";
        }

        if (string.Equals(level, "WARN", StringComparison.OrdinalIgnoreCase)
            || string.Equals(level, "WARNING", StringComparison.OrdinalIgnoreCase))
        {
            return "WARN";
        }

        if (string.Equals(level, "ERROR", StringComparison.OrdinalIgnoreCase))
        {
            return "ERROR";
        }

        if (string.Equals(level, "SUCCESS", StringComparison.OrdinalIgnoreCase))
        {
            return "SUCCESS";
        }

        return "INFO";
    }

    private void AppendLogEntry(
        DateTimeOffset timestamp,
        string content,
        string level,
        TaskQueueLogSplitMode splitMode,
        bool updateThumbnail,
        bool forceScreenshot = false)
    {
        var logTime = timestamp.ToLocalTime().ToString("HH:mm:ss");
        var hasContent = !string.IsNullOrWhiteSpace(content);
        var needsBeforeSplit = splitMode is TaskQueueLogSplitMode.Before or TaskQueueLogSplitMode.Both;
        var needsAfterSplit = splitMode is TaskQueueLogSplitMode.After or TaskQueueLogSplitMode.Both;

        if (needsBeforeSplit)
        {
            CreateNewLogCard();
        }

        if (LogCards.Count == 0 && (hasContent || updateThumbnail))
        {
            CreateNewLogCard();
        }

        if (LogCards.Count == 0)
        {
            return;
        }

        var card = LogCards[^1];
        if (hasContent)
        {
            var entry = new TaskQueueLogEntryViewModel(
                logTime,
                NormalizeLogContent(content, logTime),
                level);
            card.Append(entry);
            OverlayLogs.Add(entry);
            TrimOverlayLogs();
        }

        if (updateThumbnail)
        {
            _ = AttachThumbnailToCardAsync(card, forceScreenshot);
        }

        if (needsAfterSplit)
        {
            CreateNewLogCard();
        }

        TrimLogCards();
    }

    private void CreateNewLogCard()
    {
        if (LogCards.Count > 0 && LogCards[^1].Items.Count == 0)
        {
            return;
        }

        LogCards.Add(new TaskQueueLogCardViewModel());
    }

    private void TrimOverlayLogs()
    {
        while (OverlayLogs.Count > MaxOverlayLogs)
        {
            OverlayLogs.RemoveAt(0);
        }
    }

    private async Task<bool> SelectAndPersistOverlayTargetAsync(string targetId, CancellationToken cancellationToken)
    {
        var targetResult = await Runtime.OverlayFeatureService.SelectOverlayTargetAsync(targetId, cancellationToken);
        if (!await ApplyResultAsync(targetResult, "Overlay.Select", cancellationToken))
        {
            return false;
        }

        SelectedOverlayTarget = OverlayTargets.FirstOrDefault(t => string.Equals(t.Id, targetId, StringComparison.Ordinal))
                                ?? SelectedOverlayTarget
                                ?? new OverlayTarget(targetId, targetId, false);

        await PersistOverlayTargetSelectionBestEffortAsync(SelectedOverlayTarget, cancellationToken);
        return true;
    }

    private async Task PersistOverlayTargetSelectionBestEffortAsync(
        OverlayTarget? selectedTarget,
        CancellationToken cancellationToken)
    {
        if (selectedTarget is null)
        {
            return;
        }

        var saveResult = await Runtime.SettingsFeatureService.SaveGlobalSettingsAsync(
            new Dictionary<string, string>(StringComparer.Ordinal)
            {
                [ConfigurationKeys.OverlayTarget] = OverlayTargetPersistence.Serialize(selectedTarget),
                [ConfigurationKeys.OverlayPreviewPinned] = OverlayTargetPersistence.SerializePreviewPreference(selectedTarget),
            },
            cancellationToken);
        if (saveResult.Success)
        {
            await RecordEventAsync("Overlay.SaveTarget", saveResult.Message, cancellationToken);
            return;
        }

        LastErrorMessage = saveResult.Message;
        await RecordFailedResultAsync("Overlay.SaveTarget", saveResult, cancellationToken);
    }

    private void OnOverlaySharedStateChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (string.Equals(e.PropertyName, nameof(OverlaySharedState.Visible), StringComparison.Ordinal)
            || string.Equals(e.PropertyName, nameof(OverlaySharedState.SelectedTargetId), StringComparison.Ordinal)
            || string.Equals(e.PropertyName, nameof(OverlaySharedState.Mode), StringComparison.Ordinal)
            || string.Equals(e.PropertyName, nameof(OverlaySharedState.StatusMessage), StringComparison.Ordinal))
        {
            if (Dispatcher.UIThread.CheckAccess() || Avalonia.Application.Current is null)
            {
                SyncOverlayPresentationFromSharedState();
                return;
            }

            Dispatcher.UIThread.Post(SyncOverlayPresentationFromSharedState);
        }
    }

    private void SyncOverlayPresentationFromSharedState()
    {
        if (_overlayVisible != _overlaySharedState.Visible)
        {
            _overlayVisible = _overlaySharedState.Visible;
            OnPropertyChanged(nameof(OverlayVisible));
        }

        if (_overlayMode != _overlaySharedState.Mode)
        {
            _overlayMode = _overlaySharedState.Mode;
            OnPropertyChanged(nameof(OverlayMode));
            OnPropertyChanged(nameof(IsOverlayHiddenMode));
            OnPropertyChanged(nameof(IsOverlayPreviewMode));
            OnPropertyChanged(nameof(IsOverlayNativeMode));
        }

        if (!string.IsNullOrWhiteSpace(_overlaySharedState.StatusMessage)
            && !string.Equals(_overlayStatusText, _overlaySharedState.StatusMessage, StringComparison.Ordinal))
        {
            _overlayStatusText = _overlaySharedState.StatusMessage;
            OnPropertyChanged(nameof(OverlayStatusText));
        }

        SyncSelectedOverlayTargetFromSharedState();
        OnPropertyChanged(nameof(OverlayTargetSummaryText));
        OnPropertyChanged(nameof(OverlayButtonToolTip));
    }

    private void SyncSelectedOverlayTargetFromSharedState()
    {
        if (OverlayTargets.Count == 0 || string.IsNullOrWhiteSpace(_overlaySharedState.SelectedTargetId))
        {
            OnPropertyChanged(nameof(OverlayTargetSummaryText));
            return;
        }

        var selected = OverlayTargets.FirstOrDefault(target =>
            string.Equals(target.Id, _overlaySharedState.SelectedTargetId, StringComparison.Ordinal));
        if (selected is null || Equals(_selectedOverlayTarget, selected))
        {
            OnPropertyChanged(nameof(OverlayTargetSummaryText));
            return;
        }

        _selectedOverlayTarget = selected;
        OnPropertyChanged(nameof(SelectedOverlayTarget));
        OnPropertyChanged(nameof(OverlayTargetSummaryText));
        OnPropertyChanged(nameof(OverlayButtonToolTip));
    }

    private static string NormalizeLogContent(string? content, string displayTime)
    {
        var text = string.IsNullOrWhiteSpace(content) ? "-" : content.Trim();
        if (string.IsNullOrWhiteSpace(text))
        {
            return "-";
        }

        if (text.StartsWith(displayTime, StringComparison.Ordinal))
        {
            text = text[displayTime.Length..].TrimStart();
        }

        text = LeadingLogTimestampPattern.Replace(text, string.Empty).TrimStart();
        return string.IsNullOrWhiteSpace(text) ? "-" : text;
    }

    private void TrimLogCards()
    {
        while (LogCards.Count > MaxLogCards)
        {
            var first = LogCards[0];
            first.Thumbnail = null;
            LogCards.RemoveAt(0);
        }
    }

    private async Task AttachThumbnailToCardAsync(TaskQueueLogCardViewModel card, bool forceScreenshot)
    {
        if (!await _logThumbnailSemaphore.WaitAsync(100))
        {
            return;
        }

        try
        {
            var imageResult = await Runtime.CoreBridge.GetImageAsync();
            if (!imageResult.Success || imageResult.Value is null || imageResult.Value.Length == 0)
            {
                return;
            }

            using var stream = new MemoryStream(imageResult.Value, writable: false);
            var bitmap = new Bitmap(stream);
            if (Dispatcher.UIThread.CheckAccess())
            {
                if (!LogCards.Contains(card))
                {
                    bitmap.Dispose();
                    return;
                }

                card.Thumbnail = bitmap;
                TrimLogThumbnails();
            }
            else
            {
                await Dispatcher.UIThread.InvokeAsync(() =>
                {
                    if (!LogCards.Contains(card))
                    {
                        bitmap.Dispose();
                        return;
                    }

                    card.Thumbnail = bitmap;
                    TrimLogThumbnails();
                });
            }
        }
        catch
        {
            if (forceScreenshot)
            {
                // Ignore screencap errors for log card UI.
            }
        }
        finally
        {
            _logThumbnailSemaphore.Release();
        }
    }

    private void TrimLogThumbnails()
    {
        var maxThumbnails = Math.Max(
            0,
            TryReadGlobalInt(
                Runtime.ConfigurationService.CurrentConfig.GlobalValues,
                MAAUnified.Compat.Constants.ConfigurationKeys.MaxNumberOfLogThumbnails,
                100));
        if (maxThumbnails == 0)
        {
            foreach (var card in LogCards)
            {
                card.Thumbnail = null;
            }

            return;
        }

        var cardsWithThumbnails = LogCards
            .Where(static card => card.Thumbnail is not null)
            .ToList();
        if (cardsWithThumbnails.Count <= maxThumbnails)
        {
            return;
        }

        for (var i = 0; i < cardsWithThumbnails.Count - maxThumbnails; i++)
        {
            cardsWithThumbnails[i].Thumbnail = null;
        }
    }

    private void AppendSemanticCallbackLog(
        string callbackName,
        string module,
        int? taskIndex,
        string? subTask)
    {
        var moduleDisplay = ResolveModuleDisplayName(module);
        var taskLabel = IsValidTaskIndex(taskIndex, Tasks.Count)
            ? Tasks[taskIndex!.Value].Name
            : moduleDisplay;
        var (message, level, splitMode, withThumbnail) = callbackName switch
        {
            "TaskChainStart" => (
                string.Format(RootTexts.GetOrDefault("TaskQueue.Log.TaskStart", "Task started: {0}"), taskLabel),
                "INFO",
                TaskQueueLogSplitMode.Before,
                false),
            "SubTaskStart" => (
                string.Format(
                    RootTexts.GetOrDefault("TaskQueue.Log.SubTaskRunning", "{0}: {1} running"),
                    moduleDisplay,
                    subTask ?? "SubTask"),
                "INFO",
                TaskQueueLogSplitMode.None,
                false),
            "SubTaskCompleted" => (
                string.Format(
                    RootTexts.GetOrDefault("TaskQueue.Log.SubTaskCompleted", "{0}: {1} completed"),
                    moduleDisplay,
                    subTask ?? "SubTask"),
                "SUCCESS",
                TaskQueueLogSplitMode.None,
                true),
            "TaskChainCompleted" => (
                string.Format(RootTexts.GetOrDefault("TaskQueue.Log.TaskCompleted", "Task completed: {0}"), taskLabel),
                "SUCCESS",
                TaskQueueLogSplitMode.After,
                true),
            "TaskChainError" => (
                string.Format(RootTexts.GetOrDefault("TaskQueue.Log.TaskError", "{0} failed"), taskLabel),
                "ERROR",
                TaskQueueLogSplitMode.Both,
                true),
            "SubTaskError" => (
                string.Format(
                    RootTexts.GetOrDefault("TaskQueue.Log.SubTaskError", "{0}: {1} failed"),
                    moduleDisplay,
                    subTask ?? "SubTask"),
                "ERROR",
                TaskQueueLogSplitMode.None,
                true),
            "TaskChainStopped" => (
                RootTexts.GetOrDefault("TaskQueue.Log.TaskStopped", "Task stopped"),
                "WARN",
                TaskQueueLogSplitMode.Both,
                false),
            "AllTasksCompleted" => (
                RootTexts.GetOrDefault("TaskQueue.Log.AllCompleted", "All tasks completed"),
                "SUCCESS",
                TaskQueueLogSplitMode.Both,
                true),
            _ => (
                string.Format(
                    RootTexts.GetOrDefault("TaskQueue.Log.Observed", "{0}: {1}"),
                    moduleDisplay,
                    callbackName),
                "INFO",
                TaskQueueLogSplitMode.None,
                false),
        };

        AppendLogEntry(
            timestamp: DateTimeOffset.Now,
            content: message,
            level: level,
            splitMode: splitMode,
            updateThumbnail: withThumbnail,
            forceScreenshot: withThumbnail);
    }

    private void AppendWpfCallbackLog(CoreCallbackEvent callback, CallbackPayload payload, int? taskIndex)
    {
        TaskQueueCallbackUserLog? log = callback.MsgName switch
        {
            "TaskChainStart" => BuildTaskChainStartLog(taskIndex, payload.TaskChain),
            "TaskChainCompleted" => BuildTaskChainCompletedLog(taskIndex, payload.TaskChain),
            "TaskChainError" => BuildTaskChainErrorLog(payload.TaskChain),
            "TaskChainExtraInfo" => BuildTaskChainExtraInfoLog(payload),
            "SubTaskError" => BuildSubTaskErrorLog(payload, taskIndex),
            "SubTaskStart" => BuildSubTaskStartLog(payload),
            "SubTaskCompleted" => BuildSubTaskCompletedLog(payload, taskIndex),
            "SubTaskExtraInfo" => BuildSubTaskExtraInfoLog(payload, taskIndex),
            "AllTasksCompleted" => BuildAllTasksCompletedLog(callback.Timestamp),
            _ => null,
        };

        if (!log.HasValue)
        {
            return;
        }

        var value = log.Value;
        if (string.IsNullOrWhiteSpace(value.Content) && !value.UpdateThumbnail)
        {
            return;
        }

        AppendLogEntry(
            timestamp: callback.Timestamp,
            content: value.Content,
            level: value.Level,
            splitMode: value.SplitMode,
            updateThumbnail: value.UpdateThumbnail,
            forceScreenshot: value.ForceScreenshot);
    }

    private TaskQueueCallbackUserLog BuildTaskChainStartLog(int? taskIndex, string? taskChain)
    {
        return new(
            Content: GetRootText("StartTask", "Start task: ") + ResolveTaskLogName(taskIndex, taskChain),
            SplitMode: TaskQueueLogSplitMode.Before);
    }

    private TaskQueueCallbackUserLog BuildTaskChainCompletedLog(int? taskIndex, string? taskChain)
    {
        var content = GetRootText("CompleteTask", "Complete task: ") + ResolveTaskLogName(taskIndex, taskChain);
        if (string.Equals(TaskModuleTypes.Normalize(taskChain), TaskModuleTypes.Fight, StringComparison.OrdinalIgnoreCase)
            && _fightSanityCurrent.HasValue
            && _fightSanityMax.HasValue)
        {
            content += Environment.NewLine + string.Format(
                GetRootText("CurrentSanity", "Sanity: {0}/{1}  "),
                _fightSanityCurrent.Value,
                _fightSanityMax.Value);
        }

        return new(content, "SUCCESS");
    }

    private TaskQueueCallbackUserLog BuildTaskChainErrorLog(string? taskChain)
    {
        return new(
            Content: GetRootText("TaskError", "Task error: ") + ResolveModuleDisplayName(taskChain ?? "TaskQueue"),
            Level: "ERROR",
            UpdateThumbnail: true,
            ForceScreenshot: true);
    }

    private TaskQueueCallbackUserLog? BuildTaskChainExtraInfoLog(CallbackPayload payload)
    {
        if (!string.Equals(payload.What, "RoutingRestart", StringComparison.OrdinalIgnoreCase))
        {
            return null;
        }

        if (!string.Equals(payload.Why, "TooManyBattlesAhead", StringComparison.OrdinalIgnoreCase))
        {
            return null;
        }

        var cost = GetIntValue(payload.Root, "node_cost")?.ToString() ?? "?";
        return new(
            Content: string.Format(
                GetRootText("RoutingRestartTooManyBattles", "Too many battles ahead: {0}, restarting route"),
                cost),
            Level: "WARN");
    }

    private TaskQueueCallbackUserLog BuildAllTasksCompletedLog(DateTimeOffset timestamp)
    {
        var startedAt = _runStartedAt ?? timestamp;
        var duration = timestamp.ToLocalTime() - startedAt.ToLocalTime();
        if (duration < TimeSpan.Zero)
        {
            duration = TimeSpan.Zero;
        }

        var content = string.Format(
            GetRootText("AllTasksComplete", "All task(s) completed!\n(in {0})"),
            duration.ToString(@"h\h\ m\m\ s\s"));
        var sanityReport = BuildSanityRecoveryReport(timestamp);
        if (!string.IsNullOrWhiteSpace(sanityReport))
        {
            content += Environment.NewLine + sanityReport;
        }

        return new(
            Content: content,
            Level: "SUCCESS",
            SplitMode: TaskQueueLogSplitMode.Both,
            UpdateThumbnail: true,
            ForceScreenshot: true);
    }

    private TaskQueueCallbackUserLog? BuildSubTaskErrorLog(CallbackPayload payload, int? taskIndex)
    {
        return payload.SubTask switch
        {
            "StartGameTask" => new(GetRootText("FailedToOpenClient", "Failed to open the client. Please check the configuration file"), "ERROR"),
            "StopGameTask" => new(GetRootText("CloseArknightsFailed", "Shutdown Arknights failed"), "ERROR"),
            "AutoRecruitTask" => new(
                $"{payload.Why ?? GetRootText("ErrorOccurred", "Error occurred")}, {GetRootText("HasReturned", "Has returned")}",
                "ERROR"),
            "RecognizeDrops" => new(GetRootText("DropRecognitionError", "Drops recognition error"), "ERROR"),
            "ReportToPenguinStats" => new(BuildPenguinUploadFailureLog(payload, taskIndex), "WARN"),
            "CheckStageValid" => new(GetRootText("TheEx", "No bonus stage, stopped"), "ERROR"),
            _ => null,
        };
    }

    private string BuildPenguinUploadFailureLog(CallbackPayload payload, int? taskIndex)
    {
        if (IsAnnihilationFightTask(taskIndex))
        {
            return $"AnnihilationStage, {GetRootText("GiveUpUploadingPenguins", "Abort upload to Penguin Statistics")}";
        }

        var why = string.IsNullOrWhiteSpace(payload.Why)
            ? GetRootText("GiveUpUploadingPenguins", "Abort upload to Penguin Statistics")
            : payload.Why!;
        return $"{why}, {GetRootText("GiveUpUploadingPenguins", "Abort upload to Penguin Statistics")}";
    }

    private TaskQueueCallbackUserLog? BuildSubTaskStartLog(CallbackPayload payload)
    {
        if (string.Equals(payload.SubTask, "CombatRecordRecognitionTask", StringComparison.OrdinalIgnoreCase))
        {
            return string.IsNullOrWhiteSpace(payload.What)
                ? null
                : new(payload.What!);
        }

        if (!string.Equals(payload.SubTask, "ProcessTask", StringComparison.OrdinalIgnoreCase))
        {
            return null;
        }

        var taskName = GetStringValue(payload.Details, "task");
        var execTimes = GetIntValue(payload.Details, "exec_times") ?? 0;
        return taskName switch
        {
            "StartButton2" or "AnnihilationConfirm" => new(
                BuildFightMissionStartLog(),
                SplitMode: TaskQueueLogSplitMode.Before),
            "StoneConfirm" => BuildStoneUsedLog(execTimes),
            "AbandonAction" => new(GetRootText("ActingCommandError", "PRTS error"), "ERROR"),
            "FightMissionFailedAndStop" => new(GetRootText("FightMissionFailedAndStop", "Proxy failed too many times, task stopped"), "ERROR"),
            "RecruitRefreshConfirm" => new(GetRootText("LabelsRefreshed", "Labels refreshed")),
            "RecruitConfirm" => new(GetRootText("RecruitConfirm", "Recruit confirm")),
            "InfrastDormDoubleConfirmButton" => new(GetRootText("InfrastDormDoubleConfirmed", "Operator conflict"), "ERROR"),
            "ExitThenAbandon" => new(GetRootText("ExplorationAbandoned", "Abandoned this Exploration")),
            "MissionCompletedFlag" => new(GetRootText("FightCompleted", "Combat completed"), "SUCCESS", UpdateThumbnail: true),
            "MissionFailedFlag" => new(GetRootText("FightFailed", "Combat failed"), "ERROR", UpdateThumbnail: true),
            "StageTrader" => new(GetRootText("Trader", "Node: Rogue Trader")),
            "StageSafeHouse" => new(GetRootText("SafeHouse", "Node: Safe House")),
            "StageFilterTruth" => new(GetRootText("FilterTruth", "Node: Idea Filter")),
            "StageCombatOps" => new(GetRootText("CombatOps", "Node: Combat Operation")),
            "StageEmergencyOps" => new(GetRootText("EmergencyOps", "Stage: Emergency Operation")),
            "StageDreadfulFoe" or "StageDreadfulFoe-5" => new(GetRootText("DreadfulFoe", "Stage: Dreadful Foe")),
            "StageTraderInvestSystemFull" => new(GetRootText("UpperLimit", "Investment limit reached")),
            "OfflineConfirm" => new(
                GetRootText(
                    IsAutoRestartOnDropEnabled() ? "GameDrop" : "GameDropNoRestart",
                    IsAutoRestartOnDropEnabled()
                        ? "Game disconnected, pending reconnect"
                        : "Game disconnected, not restarting, stopping"),
                "WARN"),
            "GamePass" => new(GetRootText("RoguelikeGamePass", "Exploration completed! Congratulations!")),
            "StageTraderSpecialShoppingAfterRefresh" => new(GetRootText("RoguelikeSpecialItemBought", "Special Item Purchased!")),
            "DeepExplorationNotUnlockedComplain" => new(GetRootText("DeepExplorationNotUnlockedComplain", "Deep Investigation not unlocked yet"), "WARN"),
            "PNS-Resume" => new(GetRootText("ReclamationPnsModeError", "Current task mode does not support the current save."), "ERROR"),
            "PIS-Commence" => new(GetRootText("ReclamationPisModeError", "Current task mode requires a compatible save."), "ERROR"),
            _ => null,
        };
    }

    private TaskQueueCallbackUserLog BuildStoneUsedLog(int execTimes)
    {
        var displayTimes = execTimes > 0 ? execTimes : _stoneUsedTimes + 1;
        _stoneUsedTimes = Math.Max(_stoneUsedTimes + 1, displayTimes);
        return new($"{GetRootText("StoneUsed", "Originite Prime used")} {displayTimes} {GetRootText("UnitTime", "times")}");
    }

    private string BuildFightMissionStartLog()
    {
        var times = "???";
        var sanityCost = "???";
        if (_fightTimesFinished.HasValue && _fightSeries.HasValue && _fightSeries.Value > 0)
        {
            sanityCost = (_fightSanityCost ?? -1) >= 0 ? _fightSanityCost!.Value.ToString() : "???";
            times = _fightSeries.Value == 1
                ? (_fightTimesFinished.Value + 1).ToString()
                : $"{_fightTimesFinished.Value + 1}~{_fightTimesFinished.Value + _fightSeries.Value}";
        }

        var builder = new StringBuilder();
        builder.AppendLine(string.Format(
            GetRootText("MissionStart.FightTask", "Mission started {0} times (-{1} Sanity)"),
            times,
            sanityCost));
        if (_fightSanityCurrent.HasValue && _fightSanityMax.HasValue)
        {
            builder.AppendFormat(
                GetRootText("CurrentSanity", "Sanity: {0}/{1}  "),
                _fightSanityCurrent.Value,
                _fightSanityMax.Value);
        }

        if (_expiringMedicineUsedTimes > 0)
        {
            builder.AppendFormat(
                GetRootText("MedicineUsedTimesWithExpiring", "Medicine: {0},{1}(Expiring)  "),
                _medicineUsedTimes,
                _expiringMedicineUsedTimes);
        }
        else if (_medicineUsedTimes > 0)
        {
            builder.AppendFormat(
                GetRootText("MedicineUsedTimes", "Medicine: {0}  "),
                _medicineUsedTimes);
        }

        if (_stoneUsedTimes > 0)
        {
            builder.AppendFormat(
                GetRootText("StoneUsedTimes", "Stone: {0}  "),
                _stoneUsedTimes);
        }

        return builder.ToString().TrimEnd();
    }

    private TaskQueueCallbackUserLog? BuildSubTaskCompletedLog(CallbackPayload payload, int? taskIndex)
    {
        if (!string.Equals(payload.SubTask, "ProcessTask", StringComparison.OrdinalIgnoreCase))
        {
            return null;
        }

        var taskName = GetStringValue(payload.Details, "task");
        var taskChain = TaskModuleTypes.Normalize(payload.TaskChain);
        var execTimes = GetIntValue(payload.Details, "exec_times") ?? 0;
        if (string.Equals(taskChain, TaskModuleTypes.Infrast, StringComparison.OrdinalIgnoreCase)
            && string.Equals(taskName, "UnlockClues", StringComparison.Ordinal))
        {
            return new(GetRootText("ClueExchangeUnlocked", "Clue Exchange Unlocked"));
        }

        if (string.Equals(taskChain, TaskModuleTypes.Roguelike, StringComparison.OrdinalIgnoreCase)
            && string.Equals(taskName, "StartExplore", StringComparison.Ordinal))
        {
            return new(
                $"{GetRootText("BegunToExplore", "Exploration started")} {execTimes} {GetRootText("UnitTime", "times")}",
                SplitMode: TaskQueueLogSplitMode.Before);
        }

        if (!string.Equals(taskChain, TaskModuleTypes.Mall, StringComparison.OrdinalIgnoreCase))
        {
            return null;
        }

        return taskName switch
        {
            "EndOfActionThenStop" => new(GetRootText("CompleteTask", "Complete task: ") + GetRootText("CreditFight", "Combat with Support to earn Credits")),
            "VisitLimited" or "VisitNextBlack" => new(GetRootText("CompleteTask", "Complete task: ") + GetRootText("Visiting", "Visit Friends")),
            _ => null,
        };
    }

    private TaskQueueCallbackUserLog? BuildSubTaskExtraInfoLog(CallbackPayload payload, int? taskIndex)
    {
        switch (payload.What)
        {
            case "SanityBeforeStage":
                UpdateFightSanityState(payload.Details);
                return null;
            case "FightTimes":
                return UpdateFightTimesState(payload.Details, taskIndex);
            case "UseMedicine":
                return UpdateMedicineUsageState(payload.Details);
            case "StageDrops":
                return BuildStageDropsLog(payload.Details);
            case "EnterFacility":
                return BuildEnterFacilityLog(payload.Details);
            case "ProductIncorrect":
                return new(GetRootText("ProductIncorrect", "Product does NOT match the configuration."), "ERROR");
            case "ProductUnknown":
                return new(GetRootText("ProductUnknown", "Unknown Product"), "ERROR");
            case "ProductChanged":
                return new(GetRootText("ProductChanged", "Product has changed"));
            case "InfrastConfirmButton":
                return new(
                    string.Empty,
                    UpdateThumbnail: true,
                    ForceScreenshot: true);
            case "RecruitTagsDetected":
                return BuildRecruitTagsDetectedLog(payload.Details);
            case "RecruitSupportOperator":
                return BuildRecruitSupportOperatorLog(payload.Details);
            case "RecruitTagsSelected":
                return BuildRecruitTagsSelectedLog(payload.Details);
            case "RecruitTagsRefreshed":
                return new($"{GetRootText("Refreshed", "Refreshed")}{GetIntValue(payload.Details, "count") ?? 0}{GetRootText("UnitTime", "times")}");
            case "RecruitNoPermit":
                return new(GetRootText(
                    GetBoolValue(payload.Details, "continue") == true ? "ContinueRefresh" : "NoRecruitmentPermit",
                    GetBoolValue(payload.Details, "continue") == true
                        ? "No recruitment permit, trying to refresh Tags"
                        : "No recruitment permit, returned"));
            case "NotEnoughStaff":
                return new(GetRootText("NotEnoughStaff", "Insufficient Operators"), "ERROR");
            case "CreditFullOnlyBuyDiscount":
                return new($"{GetRootText("CreditFullOnlyBuyDiscount", "Remaining credits: ")}{GetIntValue(payload.Details, "credit") ?? 0}");
            case "StageInfo":
                return new($"{GetRootText("StartCombat", "Start combat: ")}{GetStringValue(payload.Details, "name") ?? string.Empty}");
            case "StageInfoError":
                return new(
                    GetRootText("StageInfoError", "Stage recognition error"),
                    "ERROR",
                    TaskQueueLogSplitMode.Both,
                    UpdateThumbnail: true,
                    ForceScreenshot: true);
            case "CustomInfrastRoomGroupsMatch":
                return new($"{GetRootText("RoomGroupsMatch", "Match Group: ")}{GetStringValue(payload.Details, "group") ?? string.Empty}");
            case "CustomInfrastRoomGroupsMatchFailed":
                return BuildRoomGroupsMatchFailedLog(payload.Details);
            case "CustomInfrastRoomOperators":
                return BuildRoomOperatorsLog(payload.Details);
            case "InfrastTrainingIdle":
                return new(GetRootText("TrainingIdle", "Training room is vacant"));
            case "InfrastTrainingCompleted":
                return BuildTrainingCompletedLog(payload.Details);
            case "InfrastTrainingTimeLeft":
                return BuildTrainingTimeLeftLog(payload.Details);
            case "ReclamationReport":
                return BuildReclamationReportLog(payload.Details);
            case "ReclamationProcedureStart":
                return new($"{GetRootText("MissionStart", "Mission started")} {GetIntValue(payload.Details, "times") ?? 0} {GetRootText("UnitTime", "times")}");
            case "StageQueueUnableToAgent":
                return new($"{GetRootText("StageQueue", "Stage Queue: ")} {GetStringValue(payload.Details, "stage_code")} {GetRootText("UnableToAgent", "Unable to use PRTS")}");
            case "StageQueueMissionCompleted":
                return new($"{GetRootText("StageQueue", "Stage Queue: ")} {GetStringValue(payload.Details, "stage_code")} - {GetIntValue(payload.Details, "stars") ?? 0} ★");
            default:
                return null;
        }
    }

    private void UpdateFightSanityState(JsonObject? details)
    {
        _fightSanityCurrent = GetIntValue(details, "current_sanity");
        _fightSanityMax = GetIntValue(details, "max_sanity");
        var reportTime = GetStringValue(details, "report_time");
        if (DateTimeOffset.TryParse(reportTime, out var parsed))
        {
            _fightSanityReportTime = parsed;
        }
    }

    private TaskQueueCallbackUserLog? UpdateFightTimesState(JsonObject? details, int? taskIndex)
    {
        _fightSanityCost = GetIntValue(details, "sanity_cost");
        _fightSeries = GetIntValue(details, "series");
        _fightTimesFinished = GetIntValue(details, "times_finished");

        var finished = GetBoolValue(details, "finished") == true;
        if (!finished || !_fightTimesFinished.HasValue || !_fightSeries.HasValue)
        {
            return null;
        }

        var maxTimes = GetFightTaskTimesLimit(taskIndex);
        if (!maxTimes.HasValue || _fightTimesFinished.Value >= maxTimes.Value)
        {
            return null;
        }

        return new(
            string.Format(
                GetRootText(
                    "FightTimesUnused",
                    "Completed {0} battles, will execute {1} multiplier proxy next time, will complete {2} battles after entering, exceeds {3} limit, will not enter battle"),
                _fightTimesFinished.Value,
                _fightSeries.Value,
                _fightTimesFinished.Value + _fightSeries.Value,
                maxTimes.Value),
            "WARN");
    }

    private TaskQueueCallbackUserLog? UpdateMedicineUsageState(JsonObject? details)
    {
        var medicineCount = GetIntValue(details, "count");
        if (!medicineCount.HasValue || medicineCount.Value <= 0)
        {
            return null;
        }

        var isExpiring = GetBoolValue(details, "is_expiring") == true;
        if (isExpiring)
        {
            _expiringMedicineUsedTimes += medicineCount.Value;
        }

        _medicineUsedTimes += medicineCount.Value;
        return new(
            isExpiring
                ? $"{GetRootText("ExpiringMedicineUsed", "Expiring medicine used")} {_expiringMedicineUsedTimes}(+{medicineCount.Value})"
                : $"{GetRootText("MedicineUsed", "Medicine used")} {_medicineUsedTimes}(+{medicineCount.Value})");
    }

    private TaskQueueCallbackUserLog? BuildStageDropsLog(JsonObject? details)
    {
        if (details is null)
        {
            return null;
        }

        var builder = new StringBuilder();
        var stats = GetArrayValue(details, "stats");
        if (stats is not null)
        {
            foreach (var itemNode in stats)
            {
                if (itemNode is not JsonObject item)
                {
                    continue;
                }

                var itemName = GetStringValue(item, "itemName") ?? string.Empty;
                if (string.Equals(itemName, "furni", StringComparison.OrdinalIgnoreCase))
                {
                    itemName = GetRootText("FurnitureDrop", "Furniture");
                }

                var total = GetIntValue(item, "quantity") ?? 0;
                var add = GetIntValue(item, "addQuantity") ?? 0;
                builder.Append(itemName);
                builder.Append(" : ");
                builder.Append(total);
                if (add > 0)
                {
                    builder.Append(" (+");
                    builder.Append(add);
                    builder.Append(')');
                }

                builder.AppendLine();
            }
        }

        var stage = GetObjectValue(details, "stage");
        var stageCode = GetStringValue(stage, "stageCode") ?? string.Empty;
        var dropText = builder.Length > 0 ? builder.ToString().TrimEnd() : GetRootText("NoDrop", "Nothing");
        var curTimes = GetIntValue(details, "cur_times");
        var content = new StringBuilder()
            .Append(stageCode)
            .Append(' ')
            .Append(GetRootText("TotalDrop", "Total Drops: "))
            .AppendLine()
            .Append(dropText);
        if (curTimes.HasValue && curTimes.Value >= 0)
        {
            content.AppendLine()
                .Append(GetRootText("CurTimes", "Current times"))
                .Append(" : ")
                .Append(curTimes.Value);
        }

        return new(content.ToString(), UpdateThumbnail: true);
    }

    private TaskQueueCallbackUserLog? BuildEnterFacilityLog(JsonObject? details)
    {
        if (details is null)
        {
            return null;
        }

        var facility = GetStringValue(details, "facility") ?? string.Empty;
        var index = (GetIntValue(details, "index") ?? -1) + 1;
        return new(
            $"{GetRootText("ThisFacility", "Current Facility: ")}{facility} {index:D2}",
            SplitMode: TaskQueueLogSplitMode.Before);
    }

    private TaskQueueCallbackUserLog? BuildRecruitTagsDetectedLog(JsonObject? details)
    {
        var tags = GetArrayValue(details, "tags");
        if (tags is null)
        {
            return null;
        }

        var lines = tags
            .Select(tag => tag?.GetValue<string>() ?? string.Empty)
            .Where(static tag => !string.IsNullOrWhiteSpace(tag))
            .ToArray();
        var content = lines.Length == 0 ? GetRootText("NoDrop", "Nothing") : string.Join(Environment.NewLine, lines);
        return new(
            $"{GetRootText("RecruitingResults", "Recruitment Results: ")}{Environment.NewLine}{content}",
            SplitMode: TaskQueueLogSplitMode.Before,
            UpdateThumbnail: true);
    }

    private TaskQueueCallbackUserLog? BuildRecruitSupportOperatorLog(JsonObject? details)
    {
        var name = GetStringValue(details, "name");
        return string.IsNullOrWhiteSpace(name) ? null : new($"Support Operator: {name}");
    }

    private TaskQueueCallbackUserLog? BuildRecruitTagsSelectedLog(JsonObject? details)
    {
        var tags = GetArrayValue(details, "tags");
        if (tags is null)
        {
            return null;
        }

        var selected = tags
            .Select(tag => tag?.GetValue<string>() ?? string.Empty)
            .Where(static tag => !string.IsNullOrWhiteSpace(tag))
            .ToArray();
        var content = selected.Length == 0 ? GetRootText("NoDrop", "Nothing") : string.Join(Environment.NewLine, selected);
        return new($"{GetRootText("Choose", "Choose")} Tags：{Environment.NewLine}{content}");
    }

    private TaskQueueCallbackUserLog? BuildRoomGroupsMatchFailedLog(JsonObject? details)
    {
        var groups = GetArrayValue(details, "groups");
        if (groups is null)
        {
            return null;
        }

        var content = string.Join(", ", groups.Select(group => group?.ToString()).Where(static group => !string.IsNullOrWhiteSpace(group)));
        return string.IsNullOrWhiteSpace(content)
            ? null
            : new($"{GetRootText("RoomGroupsMatchFailed", "Failed to match operator groups, group list: ")}{content}");
    }

    private TaskQueueCallbackUserLog? BuildRoomOperatorsLog(JsonObject? details)
    {
        var names = GetArrayValue(details, "names");
        if (names is null)
        {
            return null;
        }

        var content = string.Join(", ", names.Select(name => name?.ToString()).Where(static name => !string.IsNullOrWhiteSpace(name)));
        return string.IsNullOrWhiteSpace(content)
            ? null
            : new($"{GetRootText("RoomOperators", "Preferred Operators: ")}{content}");
    }

    private TaskQueueCallbackUserLog? BuildTrainingCompletedLog(JsonObject? details)
    {
        if (details is null)
        {
            return null;
        }

        var oper = GetStringValue(details, "operator") ?? "Unknown";
        var skill = GetStringValue(details, "skill") ?? "Unknown";
        var level = GetIntValue(details, "level") ?? -1;
        return new(
            $"[{oper}] {skill}{Environment.NewLine}{GetRootText("TrainingLevel", "Skill Rank")}: {level} {GetRootText("TrainingCompleted", "Training completed")}");
    }

    private TaskQueueCallbackUserLog? BuildTrainingTimeLeftLog(JsonObject? details)
    {
        if (details is null)
        {
            return null;
        }

        var oper = GetStringValue(details, "operator") ?? "Unknown";
        var skill = GetStringValue(details, "skill") ?? "Unknown";
        var level = GetIntValue(details, "level") ?? -1;
        var time = GetStringValue(details, "time") ?? string.Empty;
        return new(
            $"[{oper}] {skill}{Environment.NewLine}{GetRootText("TrainingLevel", "Skill Rank")}: {level}{Environment.NewLine}{GetRootText("TrainingTimeLeft", "Remaining Time")}: {time}");
    }

    private TaskQueueCallbackUserLog? BuildReclamationReportLog(JsonObject? details)
    {
        if (details is null)
        {
            return null;
        }

        return new(
            $"{GetRootText("AlgorithmFinish", "Algorithm Finish")}{Environment.NewLine}" +
            $"{GetRootText("AlgorithmBadge", "Algorithm Badge")}: {GetIntValue(details, "total_badges") ?? -1}(+{GetIntValue(details, "badges") ?? -1}){Environment.NewLine}" +
            $"{GetRootText("AlgorithmConstructionPoint", "Algorithm Construction Point")}: {GetIntValue(details, "total_construction_points") ?? -1}(+{GetIntValue(details, "construction_points") ?? -1})");
    }

    private string? BuildSanityRecoveryReport(DateTimeOffset timestamp)
    {
        if (!_fightSanityCurrent.HasValue || !_fightSanityMax.HasValue || !_fightSanityReportTime.HasValue)
        {
            return null;
        }

        var recoveryTime = _fightSanityReportTime.Value.AddMinutes(
            _fightSanityCurrent.Value < _fightSanityMax.Value
                ? (_fightSanityMax.Value - _fightSanityCurrent.Value) * 6
                : 0);
        var diff = recoveryTime - timestamp;
        if (diff < TimeSpan.Zero)
        {
            diff = TimeSpan.Zero;
        }

        return GetRootText("SanityReport", "Sanity will be full at {DateTime} (in {TimeDiff})")
            .Replace("{DateTime}", recoveryTime.ToLocalTime().ToString("yyyy-MM-dd HH:mm"), StringComparison.Ordinal)
            .Replace("{TimeDiff}", diff.ToString(@"h\h\ m\m"), StringComparison.Ordinal);
    }

    private string ResolveTaskLogName(int? taskIndex, string? taskChain)
    {
        if (IsValidTaskIndex(taskIndex, Tasks.Count))
        {
            return ResolveTaskDisplayName(Tasks[taskIndex!.Value]);
        }

        return string.IsNullOrWhiteSpace(taskChain)
            ? "Task"
            : $"({ResolveModuleDisplayName(taskChain)})";
    }

    private void ResetRuntimeLogState()
    {
        _runStartedAt = DateTimeOffset.Now;
        _fightSanityReportTime = null;
        _fightSanityCurrent = null;
        _fightSanityMax = null;
        _fightSanityCost = null;
        _fightSeries = null;
        _fightTimesFinished = null;
        _medicineUsedTimes = 0;
        _expiringMedicineUsedTimes = 0;
        _stoneUsedTimes = 0;
    }

    private int? GetFightTaskTimesLimit(int? taskIndex)
    {
        var taskParams = GetCurrentTaskParams(taskIndex);
        return GetIntValue(taskParams, "times");
    }

    private bool IsAnnihilationFightTask(int? taskIndex)
    {
        var taskParams = GetCurrentTaskParams(taskIndex);
        var stage = GetStringValue(taskParams, "stage");
        return string.Equals(stage, "Annihilation", StringComparison.OrdinalIgnoreCase);
    }

    private JsonObject? GetCurrentTaskParams(int? taskIndex)
    {
        if (!IsValidTaskIndex(taskIndex, Tasks.Count))
        {
            return null;
        }

        if (!Runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
        {
            return null;
        }

        var index = taskIndex!.Value;
        if (index < 0 || index >= profile.TaskQueue.Count)
        {
            return null;
        }

        return profile.TaskQueue[index].Params;
    }

    private bool IsAutoRestartOnDropEnabled()
    {
        return TryReadProfileBool(Runtime.ConfigurationService.CurrentConfig, ConfigurationKeys.AutoRestartOnDrop, true);
    }

    private string GetRootText(string key, string fallback)
    {
        return RootTexts.GetOrDefault(key, fallback);
    }

    private static string? GetStringValue(JsonObject? obj, string key)
    {
        if (obj is null || !obj.TryGetPropertyValue(key, out var node) || node is null)
        {
            return null;
        }

        if (node is JsonValue value && value.TryGetValue(out string? text))
        {
            return text;
        }

        return node.ToString();
    }

    private static int? GetIntValue(JsonObject? obj, string key)
    {
        if (obj is null || !obj.TryGetPropertyValue(key, out var node) || node is null)
        {
            return null;
        }

        if (node is JsonValue value)
        {
            if (value.TryGetValue(out int number))
            {
                return number;
            }

            if (value.TryGetValue(out string? text) && int.TryParse(text, out number))
            {
                return number;
            }
        }

        return null;
    }

    private static bool? GetBoolValue(JsonObject? obj, string key)
    {
        if (obj is null || !obj.TryGetPropertyValue(key, out var node) || node is null)
        {
            return null;
        }

        if (node is JsonValue value)
        {
            if (value.TryGetValue(out bool flag))
            {
                return flag;
            }

            if (value.TryGetValue(out string? text) && bool.TryParse(text, out flag))
            {
                return flag;
            }
        }

        return null;
    }

    private static JsonObject? GetObjectValue(JsonObject? obj, string key)
    {
        if (obj is null || !obj.TryGetPropertyValue(key, out var node))
        {
            return null;
        }

        return node as JsonObject;
    }

    private static JsonArray? GetArrayValue(JsonObject? obj, string key)
    {
        if (obj is null || !obj.TryGetPropertyValue(key, out var node))
        {
            return null;
        }

        return node as JsonArray;
    }

    private async Task HandleCallbackAsync(CoreCallbackEvent callback)
    {
        await Dispatcher.UIThread.InvokeAsync(() => HandleCallbackCoreAsync(callback));
    }

    private async Task HandleCallbackCoreAsync(CoreCallbackEvent callback)
    {
        var currentOwner = Runtime.SessionService.CurrentRunOwner;
        if (!string.IsNullOrWhiteSpace(currentOwner) && !Runtime.SessionService.IsRunOwner(TaskQueueRunOwner))
        {
            return;
        }

        var metadata = ParseCallbackPayload(callback.PayloadJson);
        if (metadata.HasParseError)
        {
            var warning = $"msgId={callback.MsgId}; msgName={callback.MsgName}; {metadata.ParseError}";
            Runtime.LogService.Warn($"TaskQueue callback payload parse failed: {warning}");
            await RecordEventAsync("TaskQueue.Callback.Parse", warning);
        }

        UpdateRoguelikeCombatState(callback.MsgName, metadata);

        var taskChain = metadata.TaskChain;
        var runId = ResolveRunId(metadata.RunId);
        var taskResolution = ResolveCallbackTaskIndex(
            metadata.TaskIndex,
            metadata.TaskId,
            taskChain,
            callback.MsgName);
        var taskIndex = taskResolution.TaskIndex;
        var resolveSource = taskResolution.ResolveSource;
        var module = ResolveCallbackModule(taskChain, taskIndex);
        await RecordTaskResolutionWarningIfNeededAsync(
            taskResolution,
            callback.MsgName,
            runId,
            taskChain,
            metadata.TaskIndex,
            metadata.TaskId);

        switch (callback.MsgName)
        {
            case "TaskChainStart":
                _currentRunId = runId;
                UpdateTaskStatus(taskIndex, taskChain, TaskQueueItemStatus.Running);
                AppendWpfCallbackLog(callback, metadata, taskIndex);
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    TaskQueueItemStatus.Running,
                    callback.PayloadJson,
                    resolveSource: resolveSource);
                break;
            case "SubTaskStart":
                UpdateTaskStatus(taskIndex, taskChain, TaskQueueItemStatus.Running);
                StatusMessage = $"{taskChain ?? "Task"}::{metadata.SubTask ?? "SubTask"} running.";
                AppendWpfCallbackLog(callback, metadata, taskIndex);
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    TaskQueueItemStatus.Running,
                    callback.PayloadJson,
                    resolveSource: resolveSource);
                break;
            case "SubTaskCompleted":
                UpdateTaskStatus(taskIndex, taskChain, TaskQueueItemStatus.Running);
                StatusMessage = $"{taskChain ?? "Task"}::{metadata.SubTask ?? "SubTask"} completed.";
                await UpdateMallDailyExecutionMarkerAsync(metadata, taskIndex);
                AppendWpfCallbackLog(callback, metadata, taskIndex);
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    TaskQueueItemStatus.Running,
                    callback.PayloadJson,
                    resolveSource: resolveSource);
                break;
            case "TaskChainCompleted":
                UpdateTaskStatus(taskIndex, taskChain, TaskQueueItemStatus.Success);
                ClearRoguelikeCombatStateIfTaskCompleted(taskChain);
                AppendWpfCallbackLog(callback, metadata, taskIndex);
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    TaskQueueItemStatus.Success,
                    callback.PayloadJson,
                    resolveSource: resolveSource);
                CompleteTaskQueueRunOwnership();
                break;
            case "TaskChainError":
            case "SubTaskError":
                UpdateTaskStatus(taskIndex, taskChain, TaskQueueItemStatus.Error);
                if (string.Equals(callback.MsgName, "TaskChainError", StringComparison.OrdinalIgnoreCase))
                {
                    ClearRoguelikeCombatStateIfTaskCompleted(taskChain);
                }
                LastErrorMessage = $"{callback.MsgName}: {callback.PayloadJson}";
                AppendWpfCallbackLog(callback, metadata, taskIndex);
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    TaskQueueItemStatus.Error,
                    callback.PayloadJson,
                    UiErrorCode.TaskRuntimeCallbackError,
                    resolveSource);
                CompleteTaskQueueRunOwnership();
                break;
            case "TaskChainStopped":
                if (_clearTaskStatusesWhenStopped)
                {
                    ResetAllTaskStatuses();
                }
                else if (taskIndex.HasValue || !string.IsNullOrWhiteSpace(taskChain))
                {
                    UpdateTaskStatus(taskIndex, taskChain, TaskQueueItemStatus.Skipped);
                }
                else
                {
                    MarkRunningTasks(TaskQueueItemStatus.Skipped);
                }

                ClearRoguelikeCombatStateIfTaskCompleted(taskChain);

                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    TaskQueueItemStatus.Skipped,
                    callback.PayloadJson,
                    resolveSource: resolveSource);
                CompleteTaskQueueRunOwnership();
                break;
            case "AllTasksCompleted":
                MarkRunningTasks(TaskQueueItemStatus.Success);
                AppendWpfCallbackLog(callback, metadata, taskIndex);
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    TaskQueueItemStatus.Success,
                    callback.PayloadJson,
                    resolveSource: resolveSource);
                if (!string.Equals(_lastPostActionRunId, runId, StringComparison.Ordinal))
                {
                    _lastPostActionRunId = runId;
                    await ExecutePostActionAfterCompletionAsync(callback, runId, taskIndex);
                }

                CompleteTaskQueueRunOwnership();

                break;
            case "TaskChainExtraInfo":
            case "SubTaskExtraInfo":
                AppendWpfCallbackLog(callback, metadata, taskIndex);
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    "Observed",
                    callback.PayloadJson,
                    resolveSource: resolveSource);
                break;
            default:
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    "Observed",
                    callback.PayloadJson,
                    resolveSource: resolveSource);
                break;
        }
    }

    private async Task UpdateMallDailyExecutionMarkerAsync(CallbackPayload payload, int? taskIndex)
    {
        if (!string.Equals(TaskModuleTypes.Normalize(payload.TaskChain), TaskModuleTypes.Mall, StringComparison.OrdinalIgnoreCase))
        {
            return;
        }

        if (!IsValidTaskIndex(taskIndex, Tasks.Count) || string.IsNullOrWhiteSpace(payload.SubTask))
        {
            return;
        }

        var updateCreditFight = string.Equals(payload.SubTask, "EndOfActionThenStop", StringComparison.Ordinal);
        var updateVisitFriends = string.Equals(payload.SubTask, "VisitLimited", StringComparison.Ordinal)
                                 || string.Equals(payload.SubTask, "VisitNextBlack", StringComparison.Ordinal);
        if (!updateCreditFight && !updateVisitFriends)
        {
            return;
        }

        if (!Runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
        {
            return;
        }

        var index = taskIndex!.Value;
        if (index < 0 || index >= profile.TaskQueue.Count)
        {
            return;
        }

        var task = profile.TaskQueue[index];
        if (!string.Equals(TaskModuleTypes.Normalize(task.Type), TaskModuleTypes.Mall, StringComparison.OrdinalIgnoreCase))
        {
            return;
        }

        task.Params ??= new JsonObject();
        var clientType = ResolveProfileClientType(profile);
        var mark = MallDailyResetHelper.GetCurrentYjDateString(DateTime.UtcNow, clientType);
        var updated = false;
        if (updateCreditFight)
        {
            task.Params[UiMallCreditFightLastTime] = mark;
            updated = true;
        }

        if (updateVisitFriends)
        {
            task.Params[UiMallVisitFriendsLastTime] = mark;
            updated = true;
        }

        if (!updated)
        {
            return;
        }

        try
        {
            await Runtime.ConfigurationService.SaveAsync();
        }
        catch (Exception ex)
        {
            Runtime.LogService.Warn($"Failed to persist mall daily marker: {ex.Message}");
        }
    }

    private static string ResolveProfileClientType(UnifiedProfile profile)
    {
        if (!profile.Values.TryGetValue("ClientType", out var node) || node is not JsonValue value)
        {
            return "Official";
        }

        if (!value.TryGetValue(out string? clientType) || string.IsNullOrWhiteSpace(clientType))
        {
            return "Official";
        }

        return clientType.Trim();
    }

    private async Task<bool> EnsureEditableAsync(string scope, CancellationToken cancellationToken = default)
    {
        if (CanEdit)
        {
            return true;
        }

        return await ApplyResultAsync(
            UiOperationResult.Fail(
                UiErrorCode.TaskQueueEditBlocked,
                Texts.GetOrDefault(
                    "TaskQueue.Error.EditBlockedWhileRunning",
                    "Task editing is blocked while running.")),
            scope,
            cancellationToken);
    }

    private static bool IsValidTaskIndex(int? taskIndex, int count)
    {
        return taskIndex.HasValue && taskIndex.Value >= 0 && taskIndex.Value < count;
    }

    private CallbackTaskResolution ResolveCallbackTaskIndex(
        int? callbackTaskIndex,
        int? callbackTaskId,
        string? taskChain,
        string action)
    {
        if (IsValidTaskIndex(callbackTaskIndex, Tasks.Count))
        {
            return new CallbackTaskResolution(callbackTaskIndex, "task_index");
        }

        if (callbackTaskId.HasValue
            && Runtime.SessionService.TryResolveTaskIndexByCoreTaskId(callbackTaskId.Value, out var mappedIndex)
            && IsValidTaskIndex(mappedIndex, Tasks.Count))
        {
            return new CallbackTaskResolution(mappedIndex, "task_id_map");
        }

        return ResolveCallbackTaskByChain(taskChain, action);
    }

    private CallbackTaskResolution ResolveCallbackTaskByChain(string? taskChain, string action)
    {
        var matchedIndices = FindTaskIndicesByChain(taskChain);
        if (matchedIndices.Count == 0)
        {
            return new CallbackTaskResolution(
                null,
                "unresolved",
                WarningDetail: $"taskChain={taskChain ?? "-"} action={action} reason=no-matching-task-chain");
        }

        if (matchedIndices.Count == 1)
        {
            return new CallbackTaskResolution(matchedIndices[0], "chain_unique");
        }

        int selectedIndex;
        var strategy = "fallback-min-index";
        if (ShouldPreferRunningTask(action))
        {
            if (TryFindIndexByStatus(matchedIndices, TaskQueueItemStatus.Running, out selectedIndex))
            {
                strategy = "prefer-running";
            }
            else
            {
                selectedIndex = matchedIndices[0];
            }
        }
        else if (ShouldPreferIdleTask(action))
        {
            if (TryFindIndexByStatus(matchedIndices, TaskQueueItemStatus.Idle, out selectedIndex))
            {
                strategy = "prefer-idle";
            }
            else
            {
                selectedIndex = matchedIndices[0];
            }
        }
        else
        {
            selectedIndex = matchedIndices[0];
        }

        return new CallbackTaskResolution(
            selectedIndex,
            "chain_heuristic",
            WarningDetail: $"taskChain={taskChain ?? "-"} action={action} candidates={string.Join(",", matchedIndices)} selected={selectedIndex} strategy={strategy}");
    }

    private static bool ShouldPreferRunningTask(string action)
    {
        return action is "TaskChainCompleted" or "TaskChainError" or "SubTaskError" or "TaskChainStopped";
    }

    private static bool ShouldPreferIdleTask(string action)
    {
        return action is "TaskChainStart" or "SubTaskStart" or "SubTaskCompleted";
    }

    private bool TryFindIndexByStatus(IReadOnlyList<int> candidateIndices, string expectedStatus, out int selectedIndex)
    {
        foreach (var index in candidateIndices)
        {
            var task = Tasks[index];
            if (string.Equals(expectedStatus, TaskQueueItemStatus.Idle, StringComparison.OrdinalIgnoreCase))
            {
                if (!task.IsStatusIdle)
                {
                    continue;
                }
            }
            else if (!string.Equals(task.Status, expectedStatus, StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            selectedIndex = index;
            return true;
        }

        selectedIndex = -1;
        return false;
    }

    private List<int> FindTaskIndicesByChain(string? taskChain)
    {
        var matches = new List<int>();
        if (string.IsNullOrWhiteSpace(taskChain))
        {
            return matches;
        }

        for (var i = 0; i < Tasks.Count; i++)
        {
            if (string.Equals(
                    TaskModuleTypes.Normalize(Tasks[i].Type),
                    taskChain,
                    StringComparison.OrdinalIgnoreCase))
            {
                matches.Add(i);
            }
        }

        return matches;
    }

    private async Task RecordTaskResolutionWarningIfNeededAsync(
        CallbackTaskResolution resolution,
        string action,
        string runId,
        string? taskChain,
        int? callbackTaskIndex,
        int? callbackTaskId)
    {
        if (resolution.ResolveSource is not ("chain_heuristic" or "unresolved"))
        {
            return;
        }

        var payload =
            $"runId={runId} action={action} taskChain={taskChain ?? "-"} " +
            $"taskIndex={callbackTaskIndex?.ToString() ?? "-"} taskId={callbackTaskId?.ToString() ?? "-"} " +
            $"resolveSource={resolution.ResolveSource} detail={resolution.WarningDetail ?? "-"}";
        await RecordEventAsync("TaskQueue.Callback.ResolveTask", payload);
    }

    private string ResolveCallbackModule(string? taskChain, int? taskIndex)
    {
        if (!string.IsNullOrWhiteSpace(taskChain))
        {
            return taskChain!;
        }

        if (IsValidTaskIndex(taskIndex, Tasks.Count))
        {
            return TaskModuleTypes.Normalize(Tasks[taskIndex!.Value].Type);
        }

        return "TaskQueue";
    }

    private void UpdateTaskStatus(int? taskIndex, string? taskChain, string status)
    {
        if (IsValidTaskIndex(taskIndex, Tasks.Count))
        {
            var task = Tasks[taskIndex!.Value];
            task.Status = status;
            RefreshTaskItemLocalization(task);
        }
    }

    private async Task ExecutePostActionAfterCompletionAsync(CoreCallbackEvent callback, string runId, int? taskIndex)
    {
        var result = await Runtime.PostActionFeatureService.ExecuteAfterCompletionAsync(
            new PostActionExecutionContext(
                callback.MsgName,
                WasSuccessfulTaskChain: true,
                RunId: runId,
                TaskIndex: taskIndex),
            PostActionModule.BuildRuntimeConfig());

        if (!result.Success)
        {
            LastErrorMessage = result.Message;
            await RecordFailedResultAsync("PostAction.Execute", result);
            return;
        }

        StatusMessage = result.Message;
        if (PostActionModule.Once)
        {
            await PostActionModule.ReloadPersistentConfigAsync();
        }
    }

    private void MarkRunningTasks(string status)
    {
        foreach (var task in Tasks.Where(t => string.Equals(t.Status, TaskQueueItemStatus.Running, StringComparison.OrdinalIgnoreCase)))
        {
            task.Status = status;
            RefreshTaskItemLocalization(task);
        }
    }

    private void ResetAllTaskStatuses()
    {
        foreach (var task in Tasks)
        {
            if (string.Equals(task.Status, TaskQueueItemStatus.Idle, StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            task.Status = TaskQueueItemStatus.Idle;
            RefreshTaskItemLocalization(task);
        }
    }

    private void SyncStoppedUiStateIfSessionNotActive()
    {
        if (CurrentSessionState is SessionState.Running or SessionState.Stopping)
        {
            return;
        }

        StartPrecheckWarningMessage = string.Empty;
        if (_clearTaskStatusesWhenStopped)
        {
            ResetAllTaskStatuses();
            _clearTaskStatusesWhenStopped = false;
        }
        else
        {
            MarkRunningTasks(TaskQueueItemStatus.Skipped);
        }

        CompleteTaskQueueRunOwnership();
    }

    private void CompleteTaskQueueRunOwnership()
    {
        Runtime.SessionService.EndRun(TaskQueueRunOwner);
    }

    private async Task RecordRuntimeStatusAsync(
        string runId,
        int? taskIndex,
        string module,
        string action,
        string status,
        string message,
        string? errorCode = null,
        string resolveSource = "unresolved")
    {
        LastRuntimeStatus = new TaskRuntimeStatusSnapshot(
            RunId: runId,
            TaskIndex: taskIndex,
            Module: module,
            Action: action,
            Status: status,
            Message: message,
            Timestamp: DateTimeOffset.UtcNow);

        var code = string.IsNullOrWhiteSpace(errorCode) ? "-" : errorCode;
        var payload =
            $"runId={runId} taskIndex={taskIndex?.ToString() ?? "-"} module={module} action={action} " +
            $"resolveSource={resolveSource} errorCode={code} message={message}";
        if (code == "-")
        {
            await RecordEventAsync("TaskQueue.Callback", payload);
        }
        else
        {
            await RecordFailedResultAsync(
                "TaskQueue.Callback",
                UiOperationResult.Fail(code, payload));
        }
    }

    private readonly record struct CallbackTaskResolution(int? TaskIndex, string ResolveSource, string? WarningDetail = null);

    private string ResolveRunId(string? callbackRunId)
    {
        if (!string.IsNullOrWhiteSpace(callbackRunId))
        {
            return callbackRunId!;
        }

        if (string.IsNullOrWhiteSpace(_currentRunId) || _currentRunId == "-")
        {
            _currentRunId = Guid.NewGuid().ToString("N");
        }

        return _currentRunId;
    }

    private static CallbackPayload ParseCallbackPayload(string payloadJson)
    {
        if (string.IsNullOrWhiteSpace(payloadJson))
        {
            return CallbackPayload.Empty;
        }

        try
        {
            if (JsonNode.Parse(payloadJson) is not JsonObject root)
            {
                return new CallbackPayload(null, null, null, null, null, null, null, null, null, "payload is not a JSON object");
            }

            var taskChain = GetStringValue(root, "task_chain") ?? GetStringValue(root, "taskchain");
            var subTask = GetStringValue(root, "sub_task") ?? GetStringValue(root, "subtask");
            var runId = GetStringValue(root, "run_id")
                        ?? GetStringValue(root, "runid")
                        ?? GetStringValue(root, "uuid")
                        ?? GetStringValue(root, "id");
            var taskIndex = GetIntValue(root, "task_index") ?? GetIntValue(root, "taskindex");
            var taskId = GetIntValue(root, "task_id") ?? GetIntValue(root, "taskid");
            var what = GetStringValue(root, "what");
            var why = GetStringValue(root, "why");
            var details = GetObjectValue(root, "details");

            return new CallbackPayload(taskChain, subTask, runId, taskIndex, taskId, what, why, details, root, null);
        }
        catch (JsonException ex)
        {
            return new CallbackPayload(null, null, null, null, null, null, null, null, null, $"payload parse failed: {ex.Message}");
        }
    }

    private readonly record struct CallbackPayload(
        string? TaskChain,
        string? SubTask,
        string? RunId,
        int? TaskIndex,
        int? TaskId,
        string? What,
        string? Why,
        JsonObject? Details,
        JsonObject? Root,
        string? ParseError = null)
    {
        public static CallbackPayload Empty { get; } = new(null, null, null, null, null, null, null, null, null, null);

        public bool HasParseError => !string.IsNullOrWhiteSpace(ParseError);
    }

    private readonly record struct TaskQueueCallbackUserLog(
        string Content,
        string Level = "INFO",
        TaskQueueLogSplitMode SplitMode = TaskQueueLogSplitMode.None,
        bool UpdateThumbnail = false,
        bool ForceScreenshot = false);

    private bool ShouldDelayStopUntilRoguelikeCombatComplete()
    {
        return _roguelikeInCombat && TryReadProfileBool(
            Runtime.ConfigurationService.CurrentConfig,
            ConfigurationKeys.RoguelikeDelayAbortUntilCombatComplete,
            false);
    }

    private async Task WaitUntilRoguelikeCombatCompleteAsync(CancellationToken cancellationToken)
    {
        const int MaxWaitSeconds = 600;
        var elapsed = 0;
        while (elapsed < MaxWaitSeconds && _roguelikeInCombat)
        {
            cancellationToken.ThrowIfCancellationRequested();

            var delayEnabled = TryReadProfileBool(
                Runtime.ConfigurationService.CurrentConfig,
                ConfigurationKeys.RoguelikeDelayAbortUntilCombatComplete,
                false);
            if (!delayEnabled)
            {
                break;
            }

            CurrentSessionState = Runtime.SessionService.CurrentState;
            if (CurrentSessionState != SessionState.Running)
            {
                break;
            }

            await Task.Delay(TimeSpan.FromSeconds(1), cancellationToken);
            elapsed++;
        }
    }

    private void UpdateRoguelikeCombatState(string callbackName, CallbackPayload payload)
    {
        if (!string.Equals(callbackName, "SubTaskExtraInfo", StringComparison.OrdinalIgnoreCase))
        {
            return;
        }

        if (!string.Equals(
                TaskModuleTypes.Normalize(payload.TaskChain),
                TaskModuleTypes.Roguelike,
                StringComparison.OrdinalIgnoreCase))
        {
            return;
        }

        if (string.Equals(payload.What, "RoguelikeCombatEnd", StringComparison.OrdinalIgnoreCase))
        {
            _roguelikeInCombat = false;
            return;
        }

        var delayEnabled = TryReadProfileBool(
            Runtime.ConfigurationService.CurrentConfig,
            ConfigurationKeys.RoguelikeDelayAbortUntilCombatComplete,
            false);
        if (!delayEnabled)
        {
            return;
        }

        if (string.Equals(payload.What, "StageInfo", StringComparison.OrdinalIgnoreCase))
        {
            _roguelikeInCombat = true;
        }
    }

    private void ClearRoguelikeCombatStateIfTaskCompleted(string? taskChain)
    {
        if (string.Equals(TaskModuleTypes.Normalize(taskChain), TaskModuleTypes.Roguelike, StringComparison.OrdinalIgnoreCase))
        {
            _roguelikeInCombat = false;
        }
    }

    private string ResolveLanguage()
    {
        if (Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue("GUI.Localization", out var value)
            && value is JsonValue jsonValue
            && jsonValue.TryGetValue(out string? language)
            && !string.IsNullOrWhiteSpace(language))
        {
            return UiLanguageCatalog.Normalize(language);
        }

        return UiLanguageCatalog.DefaultLanguage;
    }

    private void RefreshSelectionBatchModeFromConfig()
    {
        var config = Runtime.ConfigurationService.CurrentConfig;
        var showToggle = TryReadProfileString(config, ConfigurationKeys.InverseClearMode, "Clear")
            .Equals("ClearInverse", StringComparison.OrdinalIgnoreCase);
        var inverseMode = TryReadProfileBool(config, ConfigurationKeys.MainFunctionInverseMode, false);

        ShowBatchModeToggle = showToggle;
        SelectionBatchMode = showToggle && inverseMode
            ? SelectionBatchMode.Inverse
            : SelectionBatchMode.Clear;
    }

    private async Task PersistSelectionBatchModeAsync(CancellationToken cancellationToken = default)
    {
        var config = Runtime.ConfigurationService.CurrentConfig;
        if (config.Profiles.TryGetValue(config.CurrentProfile, out var profile))
        {
            profile.Values[ConfigurationKeys.MainFunctionInverseMode] = JsonValue.Create(SelectionBatchMode == SelectionBatchMode.Inverse);
            if (ShowBatchModeToggle)
            {
                profile.Values[ConfigurationKeys.InverseClearMode] = JsonValue.Create("ClearInverse");
            }
        }
        else
        {
            config.GlobalValues[ConfigurationKeys.MainFunctionInverseMode] = JsonValue.Create(SelectionBatchMode == SelectionBatchMode.Inverse);
            if (ShowBatchModeToggle)
            {
                config.GlobalValues[ConfigurationKeys.InverseClearMode] = JsonValue.Create("ClearInverse");
            }
        }

        try
        {
            await Runtime.ConfigurationService.SaveAsync(cancellationToken);
        }
        catch (Exception ex)
        {
            await RecordUnhandledExceptionAsync(
                "TaskQueue.SelectionBatchMode.Save",
                ex,
                UiErrorCode.UiOperationFailed,
                "Failed to persist batch selection mode.");
        }
    }

    private static string TryReadGlobalString(
        IReadOnlyDictionary<string, JsonNode?> values,
        string key,
        string fallback)
    {
        if (!values.TryGetValue(key, out var node) || node is null)
        {
            return fallback;
        }

        if (node is JsonValue jsonValue && jsonValue.TryGetValue(out string? text) && !string.IsNullOrWhiteSpace(text))
        {
            return text;
        }

        return fallback;
    }

    private static bool TryReadGlobalBool(
        IReadOnlyDictionary<string, JsonNode?> values,
        string key,
        bool fallback)
    {
        if (!values.TryGetValue(key, out var node) || node is null)
        {
            return fallback;
        }

        if (node is JsonValue jsonValue)
        {
            if (jsonValue.TryGetValue(out bool parsedBool))
            {
                return parsedBool;
            }

            if (jsonValue.TryGetValue(out int parsedInt))
            {
                return parsedInt != 0;
            }

            if (jsonValue.TryGetValue(out string? text) && bool.TryParse(text, out parsedBool))
            {
                return parsedBool;
            }
        }

        return fallback;
    }

    private static int TryReadGlobalInt(
        IReadOnlyDictionary<string, JsonNode?> values,
        string key,
        int fallback)
    {
        if (!values.TryGetValue(key, out var node) || node is null)
        {
            return fallback;
        }

        if (node is JsonValue jsonValue)
        {
            if (jsonValue.TryGetValue(out int parsedInt))
            {
                return parsedInt;
            }

            if (jsonValue.TryGetValue(out string? text) && int.TryParse(text, out parsedInt))
            {
                return parsedInt;
            }
        }

        return fallback;
    }

    private static string TryReadProfileString(UnifiedConfig config, string key, string fallback)
    {
        if (TryGetProfileNode(config, key, out var node)
            && TryReadStringNode(node, out var value))
        {
            return value;
        }

        return TryReadGlobalString(config.GlobalValues, key, fallback);
    }

    private static bool TryReadProfileBool(UnifiedConfig config, string key, bool fallback)
    {
        if (TryGetProfileNode(config, key, out var node)
            && TryReadBoolNode(node, out var value))
        {
            return value;
        }

        return TryReadGlobalBool(config.GlobalValues, key, fallback);
    }

    private static int TryReadProfileInt(UnifiedConfig config, string key, int fallback)
    {
        if (TryGetProfileNode(config, key, out var node)
            && TryReadIntNode(node, out var value))
        {
            return value;
        }

        return TryReadGlobalInt(config.GlobalValues, key, fallback);
    }

    private static bool TryGetProfileNode(UnifiedConfig config, string key, out JsonNode? node)
    {
        if (!string.IsNullOrWhiteSpace(config.CurrentProfile)
            && config.Profiles.TryGetValue(config.CurrentProfile, out var profile)
            && profile.Values.TryGetValue(key, out node)
            && node is not null)
        {
            return true;
        }

        node = null;
        return false;
    }

    private static bool TryReadStringNode(JsonNode? node, out string value)
    {
        if (node is JsonValue jsonValue
            && jsonValue.TryGetValue(out string? text)
            && !string.IsNullOrWhiteSpace(text))
        {
            value = text.Trim();
            return true;
        }

        var raw = node?.ToString();
        if (!string.IsNullOrWhiteSpace(raw))
        {
            value = raw.Trim();
            return true;
        }

        value = string.Empty;
        return false;
    }

    private static bool TryReadBoolNode(JsonNode? node, out bool value)
    {
        if (node is JsonValue jsonValue)
        {
            if (jsonValue.TryGetValue(out bool parsedBool))
            {
                value = parsedBool;
                return true;
            }

            if (jsonValue.TryGetValue(out int parsedInt))
            {
                value = parsedInt != 0;
                return true;
            }

            if (jsonValue.TryGetValue(out string? text))
            {
                if (bool.TryParse(text, out parsedBool))
                {
                    value = parsedBool;
                    return true;
                }

                if (int.TryParse(text, out parsedInt))
                {
                    value = parsedInt != 0;
                    return true;
                }
            }
        }

        value = false;
        return false;
    }

    private static bool TryReadIntNode(JsonNode? node, out int value)
    {
        if (node is JsonValue jsonValue)
        {
            if (jsonValue.TryGetValue(out int parsedInt))
            {
                value = parsedInt;
                return true;
            }

            if (jsonValue.TryGetValue(out bool parsedBool))
            {
                value = parsedBool ? 1 : 0;
                return true;
            }

            if (jsonValue.TryGetValue(out string? text) && int.TryParse(text, out parsedInt))
            {
                value = parsedInt;
                return true;
            }
        }

        value = 0;
        return false;
    }

    private void PrepareForConfigurationContextSwitch()
    {
        CancelTypedModuleAutoSave();
        CancelPendingBinding();
        ClearTaskModuleBindings();
    }

    private void CancelTypedModuleAutoSave()
    {
        lock (_moduleAutoSaveGate)
        {
            _moduleAutoSaveCts?.Cancel();
            _moduleAutoSaveCts?.Dispose();
            _moduleAutoSaveCts = null;
        }
    }

    private void CancelPendingBinding()
    {
        lock (_pendingBindingGate)
        {
            _pendingBindingVersion++;
            _pendingBindingCts?.Cancel();
            _pendingBindingCts?.Dispose();
            _pendingBindingCts = null;
            _pendingBindingTask = Task.CompletedTask;
        }
    }

    private void RememberSelectedTaskIndex()
    {
        if (!Runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
        {
            return;
        }

        var selectedIndex = _selectedTask is null ? -1 : Tasks.IndexOf(_selectedTask);
        profile.Values[TaskSelectedIndexConfigKey] = JsonValue.Create(selectedIndex);
    }

    private int? ResolveReloadSelectionIndex(bool preferProfileSelectedIndex, int previousSelectedIndex, int taskCount)
    {
        if (taskCount <= 0)
        {
            return null;
        }

        if (preferProfileSelectedIndex && TryReadPersistedSelectedTaskIndex(out var persistedIndex))
        {
            return persistedIndex switch
            {
                < 0 when previousSelectedIndex >= 0 && previousSelectedIndex < taskCount => previousSelectedIndex,
                < 0 => null,
                _ when persistedIndex < taskCount => persistedIndex,
                _ => 0,
            };
        }

        if (previousSelectedIndex >= 0 && previousSelectedIndex < taskCount)
        {
            return previousSelectedIndex;
        }

        return 0;
    }

    private bool TryReadPersistedSelectedTaskIndex(out int index)
    {
        index = -1;
        if (!TryGetProfileNode(Runtime.ConfigurationService.CurrentConfig, TaskSelectedIndexConfigKey, out var node)
            || node is null)
        {
            return false;
        }

        return TryReadIntNode(node, out index);
    }

    private async Task RefreshOverlayStatusTextAsync(CancellationToken cancellationToken = default)
    {
        if (!string.IsNullOrWhiteSpace(_overlaySharedState.StatusMessage))
        {
            OverlayStatusText = _overlaySharedState.StatusMessage;
            return;
        }

        var snapshotResult = await Runtime.PlatformCapabilityService.GetSnapshotAsync(cancellationToken);
        if (snapshotResult.Success && snapshotResult.Value is not null)
        {
            OverlayStatusText = BuildCapabilityLine(PlatformCapabilityId.Overlay, snapshotResult.Value.Overlay);
            return;
        }

        OverlayStatusText = PlatformCapabilityTextMap.FormatSnapshotUnavailable(
            Texts.Language,
            snapshotResult.Message,
            _localizationFallbackReporter);
    }

    private void OnSessionStateChanged(SessionState state)
    {
        void Apply(SessionState changedState)
        {
            CurrentSessionState = changedState;
            if (changedState is SessionState.Running or SessionState.Stopping)
            {
                return;
            }

            _roguelikeInCombat = false;
            IsWaitingForStop = false;
            SyncStoppedUiStateIfSessionNotActive();
        }

        if (Dispatcher.UIThread.CheckAccess())
        {
            Apply(state);
            return;
        }

        Dispatcher.UIThread.Post(() => Apply(state));
    }
}
