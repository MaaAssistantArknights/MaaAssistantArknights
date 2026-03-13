using System.Collections.ObjectModel;
using System.Globalization;
using System.Text.Json;
using System.Text.Json.Nodes;
using Avalonia.Media;
using Avalonia.Media.Imaging;
using Avalonia.Threading;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Localization;
using MAAUnified.CoreBridge;
using LegacyConfigurationKeys = MAAUnified.Compat.Constants.ConfigurationKeys;

namespace MAAUnified.App.ViewModels.Toolbox;

public sealed class ToolboxPageViewModel : PageViewModelBase
{
    private const string ToolboxRunOwner = "Toolbox";
    private const int MaxHistoryCount = 30;
    private const string ToolboxExecutionHistoryKey = "Toolbox.ExecutionHistory";
    private const string ToolboxHistoryLoadScope = "Toolbox.History.Load";
    private const string ToolboxHistorySaveScope = "Toolbox.History.Save";
    private const string ToolboxLegacyResultScope = "Toolbox.LegacyResult";
    private const string MiniGameSecretFrontTaskName = "MiniGame@SecretFront";
    private const string RecruitTaskChain = "Recruit";
    private const string DepotTaskChain = "Depot";
    private const string OperBoxTaskChain = "OperBox";
    private static readonly Random GachaTipRandom = new();
    private const string GachaWarningMessageTextEn = "This feature is risky and may cause unintended pulls.\nPressing Confirm means you understand and accept the possible risks.";
    private static readonly JsonSerializerOptions PersistedPayloadJsonOptions = new()
    {
        PropertyNameCaseInsensitive = true,
    };

    private static readonly IReadOnlyDictionary<int, ToolboxToolKind> ToolByTabIndex = new Dictionary<int, ToolboxToolKind>
    {
        [0] = ToolboxToolKind.Recruit,
        [1] = ToolboxToolKind.OperBox,
        [2] = ToolboxToolKind.Depot,
        [3] = ToolboxToolKind.Gacha,
        [4] = ToolboxToolKind.VideoRecognition,
        [5] = ToolboxToolKind.MiniGame,
    };

    private static readonly HashSet<string> MiniGameSecretFrontEndings = new(StringComparer.Ordinal)
    {
        "A",
        "B",
        "C",
        "D",
        "E",
    };

    private static readonly HashSet<string> ExcludedDepotItemIds =
    [
        "3401",
        "3112",
        "3113",
        "3114",
        "4001",
        "4003",
        "4006",
        "5001",
    ];

    private static readonly string[] GachaTips =
    [
        "抽卡运行中，已自动开启窥屏。",
        "抽卡过程中可在当前页直接停止。",
        "窥屏帧率过高时可适当调低 FPS。",
        "若模拟器卡顿，优先检查截图方式与连接质量。",
        "抽卡使用自定义任务链，结束时会自动收尾。",
    ];

    private const string RecruitRecognitionTipText = "小提示: 和主界面的自动公招是两个独立的功能，请手动打开游戏公招 Tags 界面后使用~";
    private const string RecruitPotentialTipText = "请先使用「干员识别」获取干员信息。";
    private const string OperBoxRecognitionTipText = "特别关注会影响干员识别准确率，如有特别关注干员识别错误请自行判断。";
    private const string DepotRecognitionTipText = "该功能尚处于测试阶段，请检查结果是否准确再行使用。若有误，欢迎打包 debug/depot 文件夹后向我们提交 issue ~";
    private const string GachaInitTipText = "在罗德岛竟然有这么多志同道合的志士。是的，诗歌！战争！自由！能在历史的洪流中汇集众人的力量，为这片大地的改变而奋斗。真是令人振奋！这些悲壮又非凡的故事，是应当被传颂下去的。";
    private const string GachaWarningMessageText = "该功能属于危险功能，可能会造成误抽\n按下「确认」键即视为你了解并愿意承担可能出现的风险";
    private const string CopiedToClipboardText = "已复制到剪切板";
    private const string PeepTipText = "看看牛牛眼中的世界？";
    private const string MiniGameNameEmptyTipText = "在上方选择小游戏以开始运行。";
    private const double OperBoxPanelItemWidth = 148d;
    private const double DepotPanelItemWidth = 166d;

    private readonly ConnectionGameSharedStateViewModel? _connectionState;
    private readonly DispatcherTimer _gachaTipTimer;
    private readonly DispatcherTimer _peepTimer;
    private readonly Dictionary<string, int> _operBoxPotential = new(StringComparer.Ordinal);
    private readonly Dictionary<string, ToolboxOwnedOperatorState> _operBoxOwnedById = new(StringComparer.Ordinal);
    private ToolboxToolKind? _activeTool;
    private string _lastDispatchedParameterSummary = string.Empty;
    private bool _peepRefreshInFlight;
    private bool _peepWasAutoStarted;
    private DateTimeOffset _lastPeepFpsWindowStartedAt = DateTimeOffset.MinValue;
    private int _peepFramesInWindow;
    private int _selectedTabIndex;
    private string _resultText = "等待执行工具。";
    private bool _disclaimerAccepted;
    private string _currentToolParameters = string.Empty;
    private ToolboxExecutionState _executionState;
    private string _lastExecutionErrorCode = string.Empty;
    private DateTimeOffset? _lastExecutionAt;

    private string _recruitInfo = RecruitRecognitionTipText;
    private bool _chooseLevel3 = true;
    private bool _chooseLevel4 = true;
    private bool _chooseLevel5 = true;
    private bool _chooseLevel6 = true;
    private int _recruitLevel3Time = 540;
    private int _recruitLevel4Time = 540;
    private int _recruitLevel5Time = 540;
    private bool _recruitAutoSetTime = true;
    private bool _recruitmentShowPotential = true;

    private string _operBoxInfo = OperBoxRecognitionTipText;
    private int _operBoxSelectedIndex;
    private string _operBoxMode = "owned";

    private string _depotInfo = DepotRecognitionTipText;
    private DateTimeOffset? _lastDepotSyncTime;
    private string _depotFormat = "summary";
    private string _depotTopNInput = "50";

    private string _gachaInfo = GachaInitTipText;
    private bool _gachaShowDisclaimer = true;
    private bool _gachaShowDisclaimerNoMore;
    private bool _isGachaInProgress;
    private string _gachaDrawCountInput = "10";

    private bool _peeping;
    private bool _isPeepTransitioning;
    private Bitmap? _peepImage;
    private double _peepScreenFps;
    private int _peepTargetFps = 20;

    private string _miniGameTaskName = "SS@Store@Begin";
    private string _miniGameSecretFrontEnding = "A";
    private string _miniGameSecretFrontEvent = string.Empty;
    private string _miniGameTip = MiniGameNameEmptyTipText;

    public ToolboxPageViewModel(MAAUnifiedRuntime runtime, ConnectionGameSharedStateViewModel? connectionState = null)
        : base(runtime)
    {
        _connectionState = connectionState;
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
        RecruitResultLines = new ObservableCollection<RecruitResultLineViewModel>();
        OperBoxHaveList = new ObservableCollection<OperBoxOperatorItemViewModel>();
        OperBoxNotHaveList = new ObservableCollection<OperBoxOperatorItemViewModel>();
        DepotResult = new ObservableCollection<DepotItemViewModel>();
        MiniGameTaskList = new ObservableCollection<ToolboxMiniGameEntry>();
        MiniGameSecretFrontEventOptions =
        [
            new ToolboxNamedOption("不选择", string.Empty),
            new ToolboxNamedOption("支援作战平台", "支援作战平台"),
            new ToolboxNamedOption("游侠", "游侠"),
            new ToolboxNamedOption("诡影迷踪", "诡影迷踪"),
        ];
        MiniGameSecretFrontEndingOptions =
        [
            "A",
            "B",
            "C",
            "D",
            "E",
        ];

        _gachaTipTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(5) };
        _gachaTipTimer.Tick += (_, _) => RefreshGachaTip();

        _peepTimer = new DispatcherTimer();
        _peepTimer.Tick += async (_, _) => await RefreshPeepImageAsync();

        RecruitResultLines.CollectionChanged += (_, _) => OnPropertyChanged(nameof(HasRecruitResults));
        OperBoxHaveList.CollectionChanged += (_, _) =>
        {
            OnPropertyChanged(nameof(OperBoxHaveHeader));
            OnPropertyChanged(nameof(OperBoxHavePanelWidth));
        };
        OperBoxNotHaveList.CollectionChanged += (_, _) =>
        {
            OnPropertyChanged(nameof(OperBoxNotHaveHeader));
            OnPropertyChanged(nameof(OperBoxNotHavePanelWidth));
        };
        DepotResult.CollectionChanged += (_, _) =>
        {
            OnPropertyChanged(nameof(HasDepotResult));
            OnPropertyChanged(nameof(DepotPanelWidth));
        };

        runtime.SessionService.CallbackReceived += callback => _ = HandleCallbackAsync(callback);
        RefreshCurrentToolParametersPreview();
    }

    public IReadOnlyList<string> Tabs { get; }

    public ObservableCollection<ToolExecutionRecord> ExecutionHistory { get; }

    public ObservableCollection<RecruitResultLineViewModel> RecruitResultLines { get; }

    public ObservableCollection<OperBoxOperatorItemViewModel> OperBoxHaveList { get; }

    public ObservableCollection<OperBoxOperatorItemViewModel> OperBoxNotHaveList { get; }

    public ObservableCollection<DepotItemViewModel> DepotResult { get; }

    public ObservableCollection<ToolboxMiniGameEntry> MiniGameTaskList { get; }

    public IReadOnlyList<ToolboxNamedOption> MiniGameSecretFrontEventOptions { get; }

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
                OnPropertyChanged(nameof(IsGachaTabSelected));
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
        private set => SetProperty(ref _currentToolParameters, value ?? string.Empty);
    }

    public bool IsExecuting => ExecutionState == ToolboxExecutionState.Executing;

    public bool IsToolboxBusy => _activeTool is not null || Peeping || IsPeepTransitioning || IsGachaInProgress;

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

    public bool HasRecruitResults => RecruitResultLines.Count > 0;

    public bool ShowRecruitAutoSetTimeControls => RecruitAutoSetTime;

    public string RecruitPotentialTip => RecruitPotentialTipText;

    public string RecruitInfo
    {
        get => _recruitInfo;
        set => SetProperty(ref _recruitInfo, value);
    }

    public bool ChooseLevel3
    {
        get => _chooseLevel3;
        set => SetTrackedProperty(ref _chooseLevel3, value);
    }

    public bool ChooseLevel4
    {
        get => _chooseLevel4;
        set => SetTrackedProperty(ref _chooseLevel4, value);
    }

    public bool ChooseLevel5
    {
        get => _chooseLevel5;
        set => SetTrackedProperty(ref _chooseLevel5, value);
    }

    public bool ChooseLevel6
    {
        get => _chooseLevel6;
        set => SetTrackedProperty(ref _chooseLevel6, value);
    }

    public int RecruitLevel3Time
    {
        get => _recruitLevel3Time;
        set
        {
            var normalized = NormalizeRecruitMinutes(value);
            if (SetTrackedProperty(ref _recruitLevel3Time, normalized))
            {
                OnPropertyChanged(nameof(RecruitLevel3Hour));
                OnPropertyChanged(nameof(RecruitLevel3Minute));
                OnPropertyChanged(nameof(RecruitLevel3TimeInput));
            }
        }
    }

    public int RecruitLevel4Time
    {
        get => _recruitLevel4Time;
        set
        {
            var normalized = NormalizeRecruitMinutes(value);
            if (SetTrackedProperty(ref _recruitLevel4Time, normalized))
            {
                OnPropertyChanged(nameof(RecruitLevel4Hour));
                OnPropertyChanged(nameof(RecruitLevel4Minute));
                OnPropertyChanged(nameof(RecruitLevel4TimeInput));
            }
        }
    }

    public int RecruitLevel5Time
    {
        get => _recruitLevel5Time;
        set
        {
            var normalized = NormalizeRecruitMinutes(value);
            if (SetTrackedProperty(ref _recruitLevel5Time, normalized))
            {
                OnPropertyChanged(nameof(RecruitLevel5Hour));
                OnPropertyChanged(nameof(RecruitLevel5Minute));
                OnPropertyChanged(nameof(RecruitLevel5TimeInput));
            }
        }
    }

    public int RecruitLevel3Hour
    {
        get => RecruitLevel3Time / 60;
        set => RecruitLevel3Time = (Math.Clamp(value, 1, 9) * 60) + RecruitLevel3Minute;
    }

    public int RecruitLevel3Minute
    {
        get => RecruitLevel3Time % 60;
        set => RecruitLevel3Time = (RecruitLevel3Hour * 60) + NormalizeRecruitMinutePart(value);
    }

    public int RecruitLevel4Hour
    {
        get => RecruitLevel4Time / 60;
        set => RecruitLevel4Time = (Math.Clamp(value, 1, 9) * 60) + RecruitLevel4Minute;
    }

    public int RecruitLevel4Minute
    {
        get => RecruitLevel4Time % 60;
        set => RecruitLevel4Time = (RecruitLevel4Hour * 60) + NormalizeRecruitMinutePart(value);
    }

    public int RecruitLevel5Hour
    {
        get => RecruitLevel5Time / 60;
        set => RecruitLevel5Time = (Math.Clamp(value, 1, 9) * 60) + RecruitLevel5Minute;
    }

    public int RecruitLevel5Minute
    {
        get => RecruitLevel5Time % 60;
        set => RecruitLevel5Time = (RecruitLevel5Hour * 60) + NormalizeRecruitMinutePart(value);
    }

    public string RecruitLevel3TimeInput
    {
        get => RecruitLevel3Time.ToString(CultureInfo.InvariantCulture);
        set
        {
            if (TryParseRecruitMinutes(value, out var minutes))
            {
                RecruitLevel3Time = minutes;
            }
        }
    }

    public string RecruitLevel4TimeInput
    {
        get => RecruitLevel4Time.ToString(CultureInfo.InvariantCulture);
        set
        {
            if (TryParseRecruitMinutes(value, out var minutes))
            {
                RecruitLevel4Time = minutes;
            }
        }
    }

    public string RecruitLevel5TimeInput
    {
        get => RecruitLevel5Time.ToString(CultureInfo.InvariantCulture);
        set
        {
            if (TryParseRecruitMinutes(value, out var minutes))
            {
                RecruitLevel5Time = minutes;
            }
        }
    }

    public bool RecruitAutoSetTime
    {
        get => _recruitAutoSetTime;
        set
        {
            if (SetTrackedProperty(ref _recruitAutoSetTime, value))
            {
                OnPropertyChanged(nameof(ShowRecruitAutoSetTimeControls));
            }
        }
    }

    public bool RecruitmentShowPotential
    {
        get => _recruitmentShowPotential;
        set => SetTrackedProperty(ref _recruitmentShowPotential, value);
    }

    public string OperBoxInfo
    {
        get => _operBoxInfo;
        set => SetProperty(ref _operBoxInfo, value);
    }

    public int OperBoxSelectedIndex
    {
        get => _operBoxSelectedIndex;
        set => SetProperty(ref _operBoxSelectedIndex, Math.Clamp(value, 0, 1));
    }

    public string OperBoxNotHaveHeader => $"未持有 ({OperBoxNotHaveList.Count})";

    public string OperBoxHaveHeader => $"已持有 ({OperBoxHaveList.Count})";

    public double OperBoxNotHavePanelWidth => ResolvePanelWidth(OperBoxNotHaveList.Count, OperBoxPanelItemWidth);

    public double OperBoxHavePanelWidth => ResolvePanelWidth(OperBoxHaveList.Count, OperBoxPanelItemWidth);

    public string OperBoxMode
    {
        get => _operBoxMode;
        set => SetTrackedProperty(ref _operBoxMode, string.IsNullOrWhiteSpace(value) ? "owned" : value.Trim());
    }

    public string DepotInfo
    {
        get => _depotInfo;
        set => SetProperty(ref _depotInfo, value);
    }

    public DateTimeOffset? LastDepotSyncTime
    {
        get => _lastDepotSyncTime;
        private set
        {
            if (SetProperty(ref _lastDepotSyncTime, value))
            {
                OnPropertyChanged(nameof(LastDepotSyncTimeText));
                OnPropertyChanged(nameof(HasLastDepotSyncTime));
                OnPropertyChanged(nameof(LastDepotSyncDisplayText));
            }
        }
    }

    public string LastDepotSyncTimeText
    {
        get
        {
            if (LastDepotSyncTime is null)
            {
                return "尚未同步";
            }

            return LastDepotSyncTime.Value.ToLocalTime().ToString("yyyy-MM-dd HH:mm:ss", CultureInfo.InvariantCulture);
        }
    }

    public bool HasLastDepotSyncTime => LastDepotSyncTime is not null;

    public bool HasDepotResult => DepotResult.Count > 0;

    public string LastDepotSyncDisplayText => HasLastDepotSyncTime
        ? $"上次同步时间: {LastDepotSyncTimeText}"
        : string.Empty;

    public double DepotPanelWidth => ResolvePanelWidth(DepotResult.Count, DepotPanelItemWidth);

    public string DepotFormat
    {
        get => _depotFormat;
        set => SetTrackedProperty(ref _depotFormat, string.IsNullOrWhiteSpace(value) ? "summary" : value.Trim());
    }

    public string DepotTopNInput
    {
        get => _depotTopNInput;
        set => SetTrackedProperty(ref _depotTopNInput, string.IsNullOrWhiteSpace(value) ? "50" : value.Trim());
    }

    public string GachaInfo
    {
        get => _gachaInfo;
        set => SetProperty(ref _gachaInfo, value);
    }

    public bool GachaShowDisclaimer
    {
        get => _gachaShowDisclaimer;
        set
        {
            if (SetProperty(ref _gachaShowDisclaimer, value))
            {
                OnPropertyChanged(nameof(ShowGachaControls));
                OnPropertyChanged(nameof(ShowGachaPreview));
            }
        }
    }

    public bool GachaShowDisclaimerNoMore
    {
        get => _gachaShowDisclaimerNoMore;
        set => SetTrackedProperty(ref _gachaShowDisclaimerNoMore, value);
    }

    public bool IsGachaInProgress
    {
        get => _isGachaInProgress;
        private set
        {
            if (!SetProperty(ref _isGachaInProgress, value))
            {
                return;
            }

            OnPropertyChanged(nameof(PeepCommandText));
            OnPropertyChanged(nameof(ShowGachaPreview));
            if (!value)
            {
                GachaInfo = GachaInitTipText;
            }
        }
    }

    public bool ShowGachaControls => !GachaShowDisclaimer;

    public string GachaWarningText => DialogTextCatalog.Select(ResolveCurrentLanguage(), GachaWarningMessageText, GachaWarningMessageTextEn);

    public string DialogLanguage => ResolveCurrentLanguage();

    public bool ShowGachaPreview => ShowGachaControls && Peeping && PeepImage is not null;

    public string GachaDrawCountInput
    {
        get => _gachaDrawCountInput;
        set => SetTrackedProperty(ref _gachaDrawCountInput, string.IsNullOrWhiteSpace(value) ? "10" : value.Trim());
    }

    public bool IsGachaTabSelected => SelectedTabIndex == 3;

    public bool Peeping
    {
        get => _peeping;
        private set
        {
            if (!SetProperty(ref _peeping, value))
            {
                return;
            }

            OnPropertyChanged(nameof(PeepCommandText));
            OnPropertyChanged(nameof(ShowPeepTip));
            OnPropertyChanged(nameof(ShowPeepPreview));
            OnPropertyChanged(nameof(ShowGachaPreview));
        }
    }

    public bool IsPeepTransitioning
    {
        get => _isPeepTransitioning;
        private set
        {
            if (SetProperty(ref _isPeepTransitioning, value))
            {
                OnPropertyChanged(nameof(CanTogglePeep));
            }
        }
    }

    public Bitmap? PeepImage
    {
        get => _peepImage;
        private set
        {
            if (ReferenceEquals(_peepImage, value))
            {
                return;
            }

            var previous = _peepImage;
            if (SetProperty(ref _peepImage, value))
            {
                OnPropertyChanged(nameof(ShowPeepPreview));
                OnPropertyChanged(nameof(ShowGachaPreview));
                previous?.Dispose();
            }
        }
    }

    public double PeepScreenFps
    {
        get => _peepScreenFps;
        private set => SetProperty(ref _peepScreenFps, value);
    }

    public int PeepTargetFps
    {
        get => _peepTargetFps;
        set
        {
            var normalized = Math.Clamp(value, 1, 60);
            if (!SetTrackedProperty(ref _peepTargetFps, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(VideoRecognitionTargetFpsInput));
            UpdatePeepTimerInterval();
        }
    }

    public string VideoRecognitionTargetFpsInput
    {
        get => PeepTargetFps.ToString(CultureInfo.InvariantCulture);
        set
        {
            if (int.TryParse(NormalizeToken(value), out var parsed))
            {
                PeepTargetFps = parsed;
            }
        }
    }

    public string PeepTip => PeepTipText;

    public bool ShowPeepTip => !Peeping;

    public bool ShowPeepPreview => Peeping && PeepImage is not null;

    public bool CanTogglePeep => !IsPeepTransitioning;

    public string PeepCommandText => !Peeping ? "Peep!" : (IsGachaInProgress ? "Stop!!!!!" : "Stop!");

    public string MiniGameTaskName
    {
        get => _miniGameTaskName;
        set
        {
            var normalized = string.IsNullOrWhiteSpace(value) ? "SS@Store@Begin" : value.Trim();
            if (!SetTrackedProperty(ref _miniGameTaskName, normalized))
            {
                return;
            }

            UpdateMiniGameSelectionFromTaskName();
            MiniGameTip = ResolveMiniGameTip(normalized);
            OnPropertyChanged(nameof(IsMiniGameSecretFront));
        }
    }

    public string MiniGameSecretFrontEnding
    {
        get => _miniGameSecretFrontEnding;
        set
        {
            var normalized = string.IsNullOrWhiteSpace(value) ? "A" : value.Trim();
            if (SetTrackedProperty(ref _miniGameSecretFrontEnding, normalized))
            {
                RefreshCurrentToolParametersPreview();
            }
        }
    }

    public string MiniGameSecretFrontEvent
    {
        get => _miniGameSecretFrontEvent;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (!SetTrackedProperty(ref _miniGameSecretFrontEvent, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(SelectedMiniGameSecretFrontEventOption));
        }
    }

    public ToolboxMiniGameEntry? SelectedMiniGameEntry
    {
        get => MiniGameTaskList.FirstOrDefault(entry => string.Equals(entry.Value, MiniGameTaskName, StringComparison.Ordinal));
        set
        {
            if (value is null)
            {
                return;
            }

            MiniGameTaskName = value.Value;
        }
    }

    public ToolboxNamedOption? SelectedMiniGameSecretFrontEventOption
    {
        get => MiniGameSecretFrontEventOptions.FirstOrDefault(option => string.Equals(option.Value, MiniGameSecretFrontEvent, StringComparison.Ordinal))
            ?? MiniGameSecretFrontEventOptions[0];
        set
        {
            if (value is null)
            {
                return;
            }

            MiniGameSecretFrontEvent = value.Value;
        }
    }

    public string MiniGameTip
    {
        get => _miniGameTip;
        private set => SetProperty(ref _miniGameTip, value);
    }

    public bool IsMiniGameSecretFront => string.Equals(MiniGameTaskName, MiniGameSecretFrontTaskName, StringComparison.Ordinal);

    public bool IsMiniGameRunning => _activeTool == ToolboxToolKind.MiniGame;

    public string MiniGameCommandText => IsMiniGameRunning ? "Stop!" : "Link Start!";

    public string OperBoxExportText => BuildOperBoxExportText();

    public string ArkPlannerResult => BuildArkPlannerExportText();

    public string LoliconResult => BuildLoliconExportText();

    public async Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        LoadBridgeSettings();
        LoadMiniGameEntries();
        LoadOperBoxDetails();
        LoadDepotDetails();
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
                ChooseLevel3 = true;
                ChooseLevel4 = true;
                ChooseLevel5 = true;
                ChooseLevel6 = true;
                RecruitLevel3Time = 540;
                RecruitLevel4Time = 540;
                RecruitLevel5Time = 540;
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
                PeepTargetFps = 20;
                break;
            case ToolboxToolKind.MiniGame:
                MiniGameTaskName = "SS@Store@Begin";
                MiniGameSecretFrontEnding = "A";
                MiniGameSecretFrontEvent = string.Empty;
                break;
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
                ChooseLevel3 = false;
                ChooseLevel4 = false;
                ChooseLevel5 = false;
                ChooseLevel6 = false;
                RecruitLevel3Time = 55;
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
                PeepTargetFps = 0;
                break;
            case ToolboxToolKind.MiniGame:
                MiniGameTaskName = MiniGameSecretFrontTaskName;
                MiniGameSecretFrontEnding = string.Empty;
                break;
        }

        RefreshCurrentToolParametersPreview();
    }

    public async Task ExecuteCurrentToolAsync(CancellationToken cancellationToken = default)
    {
        if (!TryResolveTool(SelectedTabIndex, out var tool))
        {
            await ApplyFailureAsync(
                null,
                UiOperationResult.Fail(UiErrorCode.ToolNotSupported, $"Tool tab index `{SelectedTabIndex}` is not supported."),
                "resolve",
                cancellationToken);
            return;
        }

        switch (tool)
        {
            case ToolboxToolKind.Recruit:
                await StartRecruitAsync(cancellationToken);
                break;
            case ToolboxToolKind.OperBox:
                await StartOperBoxAsync(cancellationToken);
                break;
            case ToolboxToolKind.Depot:
                await StartDepotAsync(cancellationToken);
                break;
            case ToolboxToolKind.Gacha:
            {
                var once = string.Equals(NormalizeToken(GachaDrawCountInput), "1", StringComparison.Ordinal);
                await StartGachaAsync(once, cancellationToken);
                break;
            }
            case ToolboxToolKind.VideoRecognition:
                await TogglePeepAsync(cancellationToken);
                break;
            case ToolboxToolKind.MiniGame:
                await StartMiniGameAsync(cancellationToken);
                break;
        }
    }

    public async Task StartRecruitAsync(CancellationToken cancellationToken = default)
    {
        var request = BuildRecruitRequest();
        await DispatchToolAsync(ToolboxToolKind.Recruit, request, PrepareRecruitForStart, cancellationToken);
    }

    public async Task StartOperBoxAsync(CancellationToken cancellationToken = default)
    {
        var request = new ToolboxDispatchRequest(
            ToolboxToolKind.OperBox,
            ParameterSummary: BuildCurrentParameterText(ToolboxToolKind.OperBox));
        await DispatchToolAsync(ToolboxToolKind.OperBox, request, PrepareOperBoxForStart, cancellationToken);
    }

    public async Task StartDepotAsync(CancellationToken cancellationToken = default)
    {
        var request = new ToolboxDispatchRequest(
            ToolboxToolKind.Depot,
            ParameterSummary: BuildCurrentParameterText(ToolboxToolKind.Depot));
        await DispatchToolAsync(ToolboxToolKind.Depot, request, PrepareDepotForStart, cancellationToken);
    }

    public async Task StartGachaAsync(bool once = true, CancellationToken cancellationToken = default)
    {
        if (GachaShowDisclaimer)
        {
            await ApplyFailureAsync(
                ToolboxToolKind.Gacha,
                UiOperationResult.Fail(UiErrorCode.ToolboxDisclaimerNotAccepted, "请先确认抽卡风险提示。"),
                "gacha-disclaimer",
                cancellationToken);
            return;
        }

        GachaDrawCountInput = once ? "1" : "10";
        var request = new ToolboxDispatchRequest(
            ToolboxToolKind.Gacha,
            Gacha: new ToolboxGachaRequest(once),
            ParameterSummary: BuildCurrentParameterText(ToolboxToolKind.Gacha));
        await DispatchToolAsync(ToolboxToolKind.Gacha, request, PrepareGachaForStart, cancellationToken, startPeepAfterDispatch: true);
    }

    public async Task StartMiniGameAsync(CancellationToken cancellationToken = default)
    {
        var request = new ToolboxDispatchRequest(
            ToolboxToolKind.MiniGame,
            MiniGame: new ToolboxMiniGameRequest(GetMiniGameTask()),
            ParameterSummary: BuildCurrentParameterText(ToolboxToolKind.MiniGame));
        await DispatchToolAsync(ToolboxToolKind.MiniGame, request, PrepareMiniGameForStart, cancellationToken);
    }

    public async Task TogglePeepAsync(CancellationToken cancellationToken = default)
    {
        if (IsPeepTransitioning)
        {
            return;
        }

        IsPeepTransitioning = true;
        try
        {
            if (Peeping && _activeTool is null)
            {
                StopPeepPolling(clearImage: false, releaseRunOwner: true);
                ResultText = "已停止窥屏。";
                ExecutionState = ToolboxExecutionState.Succeeded;
                LastExecutionErrorCode = string.Empty;
                LastExecutionAt = DateTimeOffset.Now;
                ExecutionHistory.Insert(0, ToolExecutionRecord.Succeeded("Peep", BuildCurrentParameterText(ToolboxToolKind.VideoRecognition), "已停止窥屏。"));
                TrimExecutionHistory();
                await PersistExecutionHistoryAsync(cancellationToken);
                return;
            }

            if (_activeTool is not null && _activeTool != ToolboxToolKind.Gacha)
            {
                await ApplyFailureAsync(
                    ToolboxToolKind.VideoRecognition,
                    UiOperationResult.Fail(UiErrorCode.ToolboxExecutionFailed, $"当前正在执行 `{_activeTool}`，请先停止后再开启窥屏。"),
                    "peep-busy",
                    cancellationToken);
                return;
            }

            if (!Runtime.SessionService.TryBeginRun(ToolboxRunOwner, out var currentOwner))
            {
                await ApplyFailureAsync(
                    ToolboxToolKind.VideoRecognition,
                    UiOperationResult.Fail(UiErrorCode.ToolboxExecutionFailed, $"当前已有 `{currentOwner}` 正在运行。"),
                    "peep-owner",
                    cancellationToken);
                return;
            }

            var readyResult = await EnsureConnectedAsync(cancellationToken);
            if (!readyResult.Success)
            {
                Runtime.SessionService.EndRun(ToolboxRunOwner);
                await ApplyFailureAsync(ToolboxToolKind.VideoRecognition, readyResult, "peep-connect", cancellationToken);
                return;
            }

            await PersistBridgeSettingsForToolAsync(ToolboxToolKind.VideoRecognition, cancellationToken);
            _peepWasAutoStarted = false;
            StartPeepPolling();
            ResultText = "已开启窥屏。";
            ExecutionState = ToolboxExecutionState.Succeeded;
            LastExecutionErrorCode = string.Empty;
            LastExecutionAt = DateTimeOffset.Now;
        }
        finally
        {
            IsPeepTransitioning = false;
        }
    }

    public async Task StopActiveToolAsync(CancellationToken cancellationToken = default)
    {
        if (_activeTool is null)
        {
            if (Peeping)
            {
                StopPeepPolling(clearImage: false, releaseRunOwner: true);
                ResultText = "已停止窥屏。";
                ExecutionState = ToolboxExecutionState.Succeeded;
                LastExecutionErrorCode = string.Empty;
                LastExecutionAt = DateTimeOffset.Now;
            }

            return;
        }

        var activeTool = _activeTool.Value;
        var stopResult = await Runtime.ToolboxFeatureService.StopAsync(cancellationToken);
        if (!stopResult.Success)
        {
            await ApplyFailureAsync(activeTool, stopResult, "stop", cancellationToken);
            return;
        }

        CompleteActiveToolRun(activeTool, success: false, "已停止当前工具。", UiErrorCode.ToolboxExecutionCancelled);
    }

    public void AgreeGachaDisclaimer()
    {
        GachaShowDisclaimer = false;
        if (GachaShowDisclaimerNoMore)
        {
            _ = PersistBridgeSettingsForToolAsync(ToolboxToolKind.Gacha, CancellationToken.None);
        }

        GachaInfo = GachaInitTipText;
    }

    public void NotifyOperBoxExportCopied()
    {
        OperBoxInfo = CopiedToClipboardText;
    }

    public void NotifyDepotExportCopied(string target)
    {
        DepotInfo = CopiedToClipboardText;
    }

    public string GetMiniGameTask()
    {
        return MiniGameTaskName switch
        {
            MiniGameSecretFrontTaskName => $"{MiniGameTaskName}@Begin@Ending{MiniGameSecretFrontEnding}{(string.IsNullOrEmpty(MiniGameSecretFrontEvent) ? string.Empty : $"@{MiniGameSecretFrontEvent}")}",
            _ => MiniGameTaskName,
        };
    }

    internal void ApplyRuntimeCallback(CoreCallbackEvent callback)
    {
        JsonObject? payload = null;
        if (!string.IsNullOrWhiteSpace(callback.PayloadJson))
        {
            try
            {
                payload = JsonNode.Parse(callback.PayloadJson) as JsonObject;
            }
            catch
            {
                Runtime.LogService.Warn($"Toolbox callback payload parse failed: msgName={callback.MsgName}, payload={callback.PayloadJson}");
            }
        }

        if (string.Equals(callback.MsgName, "SubTaskExtraInfo", StringComparison.OrdinalIgnoreCase))
        {
            HandleSubTaskExtraInfo(payload);
            return;
        }

        if (_activeTool is null)
        {
            return;
        }

        switch (callback.MsgName)
        {
            case "TaskChainCompleted":
            case "AllTasksCompleted":
                CompleteActiveToolRun(_activeTool.Value, success: true, "工具执行完成。");
                break;
            case "TaskChainStopped":
                CompleteActiveToolRun(_activeTool.Value, success: false, "工具已停止。", UiErrorCode.ToolboxExecutionCancelled);
                break;
            case "TaskChainError":
                if (_activeTool == ToolboxToolKind.Recruit)
                {
                    RecruitInfo = "招募识别失败，请检查当前画面与连接状态。";
                }

                CompleteActiveToolRun(_activeTool.Value, success: false, "工具执行失败。", UiErrorCode.ToolboxExecutionFailed);
                break;
        }
    }

    internal async Task HandleCallbackAsync(CoreCallbackEvent callback)
    {
        await Dispatcher.UIThread.InvokeAsync(() => ApplyRuntimeCallback(callback));
    }

    private async Task DispatchToolAsync(
        ToolboxToolKind tool,
        ToolboxDispatchRequest request,
        Action prepareUiAction,
        CancellationToken cancellationToken,
        bool startPeepAfterDispatch = false)
    {
        CurrentToolParameters = BuildCurrentParameterText(tool);
        var validation = ValidateCurrentToolParameters(tool);
        if (!validation.Success)
        {
            await ApplyFailureAsync(tool, validation, "validation", cancellationToken);
            return;
        }

        if (IsToolboxBusy)
        {
            await ApplyFailureAsync(
                tool,
                UiOperationResult.Fail(UiErrorCode.ToolboxExecutionFailed, "Toolbox 当前已有任务在运行，请先停止。"),
                "busy",
                cancellationToken);
            return;
        }

        if (!Runtime.SessionService.TryBeginRun(ToolboxRunOwner, out var currentOwner))
        {
            await ApplyFailureAsync(
                tool,
                UiOperationResult.Fail(UiErrorCode.ToolboxExecutionFailed, $"当前已有 `{currentOwner}` 正在运行。"),
                "owner",
                cancellationToken);
            return;
        }

        var connectResult = await EnsureConnectedAsync(cancellationToken);
        if (!connectResult.Success)
        {
            Runtime.SessionService.EndRun(ToolboxRunOwner);
            await ApplyFailureAsync(tool, connectResult, "connect", cancellationToken);
            return;
        }

        await PersistBridgeSettingsForToolAsync(tool, cancellationToken);

        prepareUiAction();
        TransitionToExecuting(tool, request.ParameterSummary ?? CurrentToolParameters);

        UiOperationResult<ToolboxDispatchResult> dispatchResult;
        try
        {
            dispatchResult = await Runtime.ToolboxFeatureService.DispatchToolAsync(request, cancellationToken);
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            Runtime.SessionService.EndRun(ToolboxRunOwner);
            await ApplyFailureAsync(
                tool,
                UiOperationResult.Fail(UiErrorCode.ToolboxExecutionCancelled, "工具执行已取消。"),
                "dispatch-cancelled",
                CancellationToken.None);
            return;
        }
        catch (Exception ex)
        {
            Runtime.SessionService.EndRun(ToolboxRunOwner);
            await ApplyFailureAsync(
                tool,
                UiOperationResult.Fail(UiErrorCode.ToolboxExecutionFailed, ex.Message, ex.ToString()),
                "dispatch-exception",
                CancellationToken.None);
            return;
        }

        if (!dispatchResult.Success || dispatchResult.Value is null)
        {
            Runtime.SessionService.EndRun(ToolboxRunOwner);
            await ApplyFailureAsync(tool, dispatchResult.ToUntyped(), "dispatch", cancellationToken);
            return;
        }

        LastExecutionAt = dispatchResult.Value.StartedAt;
        ResultText = $"{GetToolDisplayName(tool)}已启动。";
        StatusMessage = dispatchResult.Message;

        if (startPeepAfterDispatch)
        {
            _peepWasAutoStarted = true;
            StartPeepPolling();
            IsGachaInProgress = true;
            RefreshGachaTip();
            _gachaTipTimer.Start();
        }
    }

    private void PrepareRecruitForStart()
    {
        RecruitInfo = "识别中...";
        RecruitResultLines.Clear();
    }

    private void PrepareOperBoxForStart()
    {
        OperBoxInfo = "识别中...";
        _operBoxOwnedById.Clear();
        _operBoxPotential.Clear();
        OperBoxHaveList.Clear();
        OperBoxNotHaveList.Clear();
        OperBoxSelectedIndex = 1;
    }

    private void PrepareDepotForStart()
    {
        DepotInfo = "识别中...";
        DepotResult.Clear();
    }

    private void PrepareGachaForStart()
    {
        GachaInfo = "正在连接模拟器...";
    }

    private void PrepareMiniGameForStart()
    {
        ResultText = "小游戏任务启动中...";
    }

    private ToolboxDispatchRequest BuildRecruitRequest()
    {
        var selectedLevels = new List<int>();
        if (ChooseLevel3)
        {
            selectedLevels.Add(3);
        }

        if (ChooseLevel4)
        {
            selectedLevels.Add(4);
        }

        if (ChooseLevel5)
        {
            selectedLevels.Add(5);
        }

        if (ChooseLevel6)
        {
            selectedLevels.Add(6);
        }

        return new ToolboxDispatchRequest(
            ToolboxToolKind.Recruit,
            Recruit: new ToolboxRecruitRequest(
                selectedLevels,
                RecruitAutoSetTime,
                RecruitLevel3Time,
                RecruitLevel4Time,
                RecruitLevel5Time,
                ResolveServerType()),
            ParameterSummary: BuildCurrentParameterText(ToolboxToolKind.Recruit));
    }

    private void TransitionToExecuting(ToolboxToolKind tool, string parameterSummary)
    {
        _activeTool = tool;
        _lastDispatchedParameterSummary = parameterSummary;
        ExecutionState = ToolboxExecutionState.Executing;
        LastExecutionErrorCode = string.Empty;
        LastExecutionAt = null;
        OnPropertyChanged(nameof(IsMiniGameRunning));
        OnPropertyChanged(nameof(MiniGameCommandText));
    }

    private async Task<UiOperationResult> EnsureConnectedAsync(CancellationToken cancellationToken)
    {
        if (Runtime.SessionService.CurrentState is SessionState.Connected or SessionState.Running or SessionState.Stopping)
        {
            return UiOperationResult.Ok("Session already connected.");
        }

        return await TryConnectWithCurrentSettingsAsync(cancellationToken);
    }

    private async Task<UiOperationResult> TryConnectWithCurrentSettingsAsync(CancellationToken cancellationToken)
    {
        var connectConfig = ResolveConnectConfig();
        var adbPath = ResolveEffectiveAdbPath();
        var candidates = BuildConnectAddressCandidates();
        UiOperationResult? lastFailure = null;

        foreach (var candidate in candidates)
        {
            var result = await Runtime.ConnectFeatureService.ConnectAsync(candidate, connectConfig, adbPath, cancellationToken);
            if (result.Success)
            {
                return result;
            }

            lastFailure = result;
        }

        return lastFailure ?? UiOperationResult.Fail(UiErrorCode.UiOperationFailed, "连接失败。");
    }

    private IReadOnlyList<string> BuildConnectAddressCandidates()
    {
        if (_connectionState is not null)
        {
            return _connectionState.BuildConnectAddressCandidates(includeConfiguredAddress: true);
        }

        var configured = ResolveProfileString("ConnectAddress", LegacyConfigurationKeys.ConnectAddress) ?? "127.0.0.1:5555";
        var connectConfig = ResolveConnectConfig();
        var autoDetect = ResolveProfileBool("AutoDetect", fallback: true);
        var alwaysAutoDetect = ResolveProfileBool("AlwaysAutoDetect", fallback: false, LegacyConfigurationKeys.AlwaysAutoDetect);
        var candidates = new List<string>();
        AddAddressCandidate(candidates, configured);

        if (autoDetect || alwaysAutoDetect || candidates.Count == 0)
        {
            foreach (var candidate in GetDefaultAddresses(connectConfig))
            {
                AddAddressCandidate(candidates, candidate);
            }
        }

        return candidates.Count == 0 ? ["127.0.0.1:5555"] : candidates;
    }

    private string ResolveConnectConfig()
    {
        if (_connectionState is not null)
        {
            return string.IsNullOrWhiteSpace(_connectionState.ConnectConfig) ? "General" : _connectionState.ConnectConfig.Trim();
        }

        return ResolveProfileString("ConnectConfig", LegacyConfigurationKeys.ConnectConfig) ?? "General";
    }

    private string? ResolveEffectiveAdbPath()
    {
        if (_connectionState is not null)
        {
            var resolved = _connectionState.ResolveEffectiveAdbPath(updateStateWhenResolved: true);
            return string.IsNullOrWhiteSpace(resolved) ? null : resolved;
        }

        var adb = ResolveProfileString("AdbPath", LegacyConfigurationKeys.AdbPath);
        return string.IsNullOrWhiteSpace(adb) ? null : adb;
    }

    private string ResolveServerType()
    {
        return ResolveProfileString("ServerType", fallback: "CN") ?? "CN";
    }

    private string ResolveClientType()
    {
        if (_connectionState is not null && !string.IsNullOrWhiteSpace(_connectionState.ClientType))
        {
            return _connectionState.ClientType.Trim();
        }

        return ResolveProfileString("ClientType", fallback: "Official") ?? "Official";
    }

    private string ResolveCurrentLanguage()
    {
        if (Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue(LegacyConfigurationKeys.Localization, out var node)
            && node is JsonValue value
            && value.TryGetValue(out string? language)
            && !string.IsNullOrWhiteSpace(language))
        {
            return UiLanguageCatalog.Normalize(language);
        }

        return UiLanguageCatalog.DefaultLanguage;
    }

    private static IReadOnlyList<string> GetDefaultAddresses(string connectConfig)
    {
        return connectConfig.Trim() switch
        {
            "BlueStacks" => ["127.0.0.1:5555", "127.0.0.1:5556", "127.0.0.1:5565", "127.0.0.1:5575", "127.0.0.1:5585", "127.0.0.1:5595", "127.0.0.1:5554"],
            "MuMuEmulator12" => ["127.0.0.1:16384", "127.0.0.1:16416", "127.0.0.1:16448", "127.0.0.1:16480", "127.0.0.1:16512", "127.0.0.1:16544", "127.0.0.1:16576"],
            "LDPlayer" => ["emulator-5554", "emulator-5556", "emulator-5558", "emulator-5560", "127.0.0.1:5555", "127.0.0.1:5557", "127.0.0.1:5559", "127.0.0.1:5561"],
            "Nox" => ["127.0.0.1:62001", "127.0.0.1:59865"],
            "XYAZ" => ["127.0.0.1:21503"],
            "WSA" => ["127.0.0.1:58526"],
            _ => ["127.0.0.1:5555"],
        };
    }

    private static void AddAddressCandidate(ICollection<string> candidates, string? raw)
    {
        var normalized = NormalizeToken(raw)
            .Replace("：", ":", StringComparison.Ordinal)
            .Replace("；", ":", StringComparison.Ordinal)
            .Replace(';', ':');
        if (string.IsNullOrWhiteSpace(normalized))
        {
            return;
        }

        if (candidates.Contains(normalized, StringComparer.OrdinalIgnoreCase))
        {
            return;
        }

        candidates.Add(normalized);
    }

    private void HandleSubTaskExtraInfo(JsonObject? payload)
    {
        if (payload is null)
        {
            return;
        }

        var taskChain = ReadString(payload, "taskchain");
        var what = ReadString(payload, "what");
        var details = payload["details"] as JsonObject;

        if (string.Equals(what, "StageDrops", StringComparison.OrdinalIgnoreCase) && details is not null)
        {
            UpdateDepotFromDrops(details);
        }

        if (!Runtime.SessionService.IsRunOwner(ToolboxRunOwner) && _activeTool is null)
        {
            return;
        }

        if (string.Equals(taskChain, RecruitTaskChain, StringComparison.OrdinalIgnoreCase))
        {
            HandleRecruitCallback(what, details);
            return;
        }

        if (string.Equals(taskChain, DepotTaskChain, StringComparison.OrdinalIgnoreCase) && details is not null)
        {
            ApplyDepotRecognition(details, updateSyncTime: true);
            return;
        }

        if (string.Equals(taskChain, OperBoxTaskChain, StringComparison.OrdinalIgnoreCase) && details is not null)
        {
            ApplyOperBoxRecognition(details);
        }
    }

    private void HandleRecruitCallback(string what, JsonObject? details)
    {
        if (details is null)
        {
            return;
        }

        switch (what)
        {
            case "RecruitTagsDetected":
            {
                var tags = ReadStringArray(details["tags"]);
                RecruitInfo = tags.Count == 0 ? "已识别招募标签。" : $"已识别标签：{string.Join(" / ", tags)}";
                break;
            }
            case "RecruitResult":
                ApplyRecruitResult(details["result"] as JsonArray);
                break;
        }
    }

    private void ApplyRecruitResult(JsonArray? resultArray)
    {
        RecruitResultLines.Clear();
        var language = ResolveCurrentLanguage();

        foreach (var comboNode in resultArray ?? [])
        {
            if (comboNode is not JsonObject combo)
            {
                continue;
            }

            var tagLevel = ReadInt(combo, "level");
            var tags = ReadStringArray(combo["tags"]);
            RecruitResultLines.Add(new RecruitResultLineViewModel(
                [
                    new RecruitResultSegmentViewModel(
                        $"{tagLevel}★ Tags: {string.Join("    ", tags)}",
                        null),
                ]));

            var operatorsWithPotential = (combo["opers"] as JsonArray ?? [])
                .OfType<JsonObject>()
                .Select(oper =>
                {
                    var operLevel = ReadInt(oper, "level");
                    var operId = ReadString(oper, "id");
                    var potential = -1;

                    if (RecruitmentShowPotential
                        && !string.IsNullOrWhiteSpace(operId)
                        && (tagLevel >= 4 || operLevel == 1)
                        && _operBoxPotential.TryGetValue(operId, out var potentialValue))
                    {
                        potential = potentialValue;
                    }

                    return new RecruitOperatorProjection(oper, operLevel, potential);
                })
                .OrderByDescending(item => item.Level)
                .ThenBy(item => item.Potential)
                .ToList();

            var operatorSegments = new List<RecruitResultSegmentViewModel>();
            foreach (var candidate in operatorsWithPotential)
            {
                var oper = candidate.Operator;
                var operLevel = ReadInt(oper, "level");
                var operId = ReadString(oper, "id");
                var operName = ReadString(oper, "name");

                if (!string.IsNullOrWhiteSpace(operId)
                    && ToolboxAssetCatalog.GetOperators().TryGetValue(operId, out var asset))
                {
                    operName = ToolboxAssetCatalog.GetLocalizedOperatorName(asset, language);
                }

                var suffix = string.Empty;
                if (RecruitmentShowPotential
                    && !string.IsNullOrWhiteSpace(operId)
                    && (tagLevel >= 4 || operLevel == 1))
                {
                    if (_operBoxPotential.TryGetValue(operId, out var potential))
                    {
                        suffix = potential >= 6 ? " (MAX)" : $" ({potential})";
                    }
                    else
                    {
                        suffix = " (NEW)";
                    }
                }

                operatorSegments.Add(new RecruitResultSegmentViewModel(
                    $"{operName}{suffix}",
                    ResolveStarBrush(operLevel)));
            }

            if (operatorSegments.Count > 0)
            {
                RecruitResultLines.Add(new RecruitResultLineViewModel(operatorSegments));
            }

            RecruitResultLines.Add(RecruitResultLineViewModel.CreateSpacer());
        }
    }

    private void ApplyOperBoxRecognition(JsonObject details)
    {
        if (details["own_opers"] is not JsonArray ownOpers)
        {
            return;
        }

        var operators = ToolboxAssetCatalog.GetOperators();
        var language = ResolveCurrentLanguage();

        foreach (var node in ownOpers)
        {
            if (node is not JsonObject oper)
            {
                continue;
            }

            var id = ReadString(oper, "id");
            if (string.IsNullOrWhiteSpace(id))
            {
                continue;
            }

            var rarity = ReadInt(oper, "rarity");
            var elite = ReadInt(oper, "elite");
            var level = ReadInt(oper, "level");
            var potential = ReadInt(oper, "potential");
            var name = ReadString(oper, "name");

            if (operators.TryGetValue(id, out var asset))
            {
                name = ToolboxAssetCatalog.GetLocalizedOperatorName(asset, language);
                rarity = asset.Rarity;
            }

            _operBoxOwnedById[id] = new ToolboxOwnedOperatorState(id, name, rarity, elite, level, potential);
            _operBoxPotential[id] = potential;
        }

        var done = ReadBool(details, "done");
        if (!done)
        {
            return;
        }

        RebuildOperBoxLists();
        OperBoxInfo = $"识别完成。\n{OperBoxRecognitionTipText}";
        _ = PersistOperBoxAsync(CancellationToken.None);
    }

    private void RebuildOperBoxLists()
    {
        OperBoxHaveList.Clear();
        OperBoxNotHaveList.Clear();
        var operators = ToolboxAssetCatalog.GetOperators();
        var clientType = ResolveClientType();
        var language = ResolveCurrentLanguage();

        foreach (var owned in _operBoxOwnedById.Values
                     .OrderByDescending(item => item.Rarity)
                     .ThenByDescending(item => item.Elite)
                     .ThenByDescending(item => item.Level)
                     .ThenByDescending(item => item.Potential)
                     .ThenBy(item => item.Id, StringComparer.Ordinal))
        {
            OperBoxHaveList.Add(new OperBoxOperatorItemViewModel(
                owned.Id,
                owned.Name,
                owned.Rarity,
                owned.Elite,
                owned.Level,
                owned.Potential,
                own: true));
        }

        foreach (var asset in operators.Values
                     .Where(asset => ToolboxAssetCatalog.IsOperatorAvailableInClient(asset, clientType) && !_operBoxOwnedById.ContainsKey(asset.Id))
                     .OrderByDescending(asset => asset.Rarity)
                     .ThenBy(asset => asset.Id, StringComparer.Ordinal))
        {
            OperBoxNotHaveList.Add(new OperBoxOperatorItemViewModel(
                asset.Id,
                ToolboxAssetCatalog.GetLocalizedOperatorName(asset, language),
                asset.Rarity,
                elite: 0,
                level: 0,
                potential: 0,
                own: false));
        }

        OperBoxSelectedIndex = OperBoxNotHaveList.Count > 0 ? 0 : 1;
    }

    private void ApplyDepotRecognition(JsonObject details, bool updateSyncTime)
    {
        var counts = ParseDepotCounts(details);
        DepotResult.Clear();
        var itemNames = ToolboxAssetCatalog.GetItemNames(ResolveCurrentLanguage());

        foreach (var pair in counts.OrderBy(pair => pair.Key, StringComparer.Ordinal))
        {
            DepotResult.Add(new DepotItemViewModel(
                pair.Key,
                itemNames.TryGetValue(pair.Key, out var name) ? name : pair.Key,
                pair.Value,
                ToolboxAssetCatalog.ResolveItemImagePath(pair.Key)));
        }

        var done = ReadBool(details, "done");
        if (!done)
        {
            return;
        }

        if (updateSyncTime)
        {
            LastDepotSyncTime = DateTimeOffset.UtcNow;
        }
        else
        {
            var syncTime = ReadString(details, "syncTime");
            if (DateTimeOffset.TryParse(syncTime, CultureInfo.InvariantCulture, DateTimeStyles.AssumeUniversal, out var parsed))
            {
                LastDepotSyncTime = parsed.ToUniversalTime();
            }
        }

        DepotInfo = "识别完成。";
        _ = PersistDepotAsync(CancellationToken.None);
    }

    private Dictionary<string, int> ParseDepotCounts(JsonObject details)
    {
        var result = new Dictionary<string, int>(StringComparer.Ordinal);
        if (details["data"] is JsonValue dataValue && dataValue.TryGetValue(out string? rawData) && !string.IsNullOrWhiteSpace(rawData))
        {
            TryReadDepotCountsFromJson(rawData, result);
        }
        else if (details["data"] is JsonObject directData)
        {
            TryReadDepotCountsFromJson(directData.ToJsonString(), result);
        }

        if (result.Count > 0)
        {
            return result;
        }

        if (details["arkplanner"] is JsonObject arkPlanner
            && arkPlanner["object"] is JsonObject objectNode
            && objectNode["items"] is JsonArray items)
        {
            foreach (var node in items)
            {
                if (node is not JsonObject item)
                {
                    continue;
                }

                var id = ReadString(item, "id");
                if (string.IsNullOrWhiteSpace(id))
                {
                    continue;
                }

                result[id] = ReadInt(item, "have");
            }
        }

        return result;
    }

    private void UpdateDepotFromDrops(JsonObject details)
    {
        if (details["stats"] is not JsonArray stats)
        {
            return;
        }

        var itemNames = ToolboxAssetCatalog.GetItemNames(ResolveCurrentLanguage());
        var byId = DepotResult.ToDictionary(item => item.Id, StringComparer.Ordinal);
        var changed = false;

        foreach (var node in stats)
        {
            if (node is not JsonObject item)
            {
                continue;
            }

            var itemId = ReadString(item, "itemId");
            if (string.IsNullOrWhiteSpace(itemId))
            {
                continue;
            }

            if (string.Equals(ReadString(item, "itemName"), "furni", StringComparison.OrdinalIgnoreCase))
            {
                itemId = "3401";
            }

            if (ExcludedDepotItemIds.Contains(itemId) || !int.TryParse(itemId, out _))
            {
                continue;
            }

            var addQuantity = ReadInt(item, "addQuantity");
            if (addQuantity <= 0)
            {
                continue;
            }

            if (byId.TryGetValue(itemId, out var existing))
            {
                existing.Count += addQuantity;
            }
            else
            {
                var name = itemNames.TryGetValue(itemId, out var itemName) ? itemName : itemId;
                var created = new DepotItemViewModel(itemId, name, addQuantity, ToolboxAssetCatalog.ResolveItemImagePath(itemId));
                byId[itemId] = created;
                DepotResult.Add(created);
            }

            changed = true;
        }

        if (!changed)
        {
            return;
        }

        var ordered = DepotResult.OrderBy(item => item.Id, StringComparer.Ordinal).ToArray();
        DepotResult.Clear();
        foreach (var item in ordered)
        {
            DepotResult.Add(item);
        }

        _ = PersistDepotAsync(CancellationToken.None);
    }

    private void CompleteActiveToolRun(ToolboxToolKind tool, bool success, string message, string errorCode = "")
    {
        if (_activeTool is null)
        {
            return;
        }

        if (tool == ToolboxToolKind.Gacha)
        {
            _gachaTipTimer.Stop();
            IsGachaInProgress = false;
            if (!success)
            {
                GachaInfo = message;
            }
        }

        if (_peepWasAutoStarted)
        {
            StopPeepPolling(clearImage: false, releaseRunOwner: false);
            _peepWasAutoStarted = false;
        }

        ExecutionState = success ? ToolboxExecutionState.Succeeded : ToolboxExecutionState.Failed;
        ResultText = message;
        LastExecutionErrorCode = errorCode;
        LastExecutionAt = DateTimeOffset.Now;

        ExecutionHistory.Insert(0, success
            ? ToolExecutionRecord.Succeeded(GetToolDisplayName(tool), _lastDispatchedParameterSummary, BuildResultSummary(message))
            : ToolExecutionRecord.Failed(GetToolDisplayName(tool), _lastDispatchedParameterSummary, BuildResultSummary(message), string.IsNullOrWhiteSpace(errorCode) ? UiErrorCode.ToolboxExecutionFailed : errorCode));
        TrimExecutionHistory();
        _ = PersistExecutionHistoryAsync(CancellationToken.None);

        Runtime.SessionService.EndRun(ToolboxRunOwner);
        _activeTool = null;
        _lastDispatchedParameterSummary = string.Empty;
        OnPropertyChanged(nameof(IsMiniGameRunning));
        OnPropertyChanged(nameof(MiniGameCommandText));
    }

    private async Task ApplyFailureAsync(
        ToolboxToolKind? tool,
        UiOperationResult result,
        string stage,
        CancellationToken cancellationToken)
    {
        var errorCode = string.IsNullOrWhiteSpace(result.Error?.Code)
            ? UiErrorCode.ToolboxExecutionFailed
            : result.Error.Code;
        var formatted = FormatFailureMessage(errorCode, result.Message);
        var details = MergeDetails(
            BuildFailureContextDetails(tool, errorCode, stage),
            result.Error?.Details);
        var normalized = UiOperationResult.Fail(errorCode, formatted, details);
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

    private void StartPeepPolling()
    {
        UpdatePeepTimerInterval();
        Peeping = true;
        PeepScreenFps = 0;
        _lastPeepFpsWindowStartedAt = DateTimeOffset.UtcNow;
        _peepFramesInWindow = 0;
        _peepTimer.Start();
    }

    private void StopPeepPolling(bool clearImage, bool releaseRunOwner)
    {
        _peepTimer.Stop();
        Peeping = false;
        PeepScreenFps = 0;
        if (clearImage)
        {
            PeepImage = null;
        }

        if (releaseRunOwner && _activeTool is null)
        {
            Runtime.SessionService.EndRun(ToolboxRunOwner);
        }
    }

    private void UpdatePeepTimerInterval()
    {
        _peepTimer.Interval = TimeSpan.FromMilliseconds(1000d / Math.Max(1, PeepTargetFps));
    }

    private async Task RefreshPeepImageAsync()
    {
        if (_peepRefreshInFlight || !Peeping)
        {
            return;
        }

        _peepRefreshInFlight = true;
        try
        {
            var imageResult = await Runtime.CoreBridge.GetImageAsync();
            if (!imageResult.Success || imageResult.Value is null || imageResult.Value.Length == 0)
            {
                return;
            }

            using var stream = new MemoryStream(imageResult.Value, writable: false);
            var bitmap = new Bitmap(stream);
            PeepImage = bitmap;

            _peepFramesInWindow += 1;
            var now = DateTimeOffset.UtcNow;
            var elapsed = now - _lastPeepFpsWindowStartedAt;
            if (elapsed >= TimeSpan.FromSeconds(1))
            {
                PeepScreenFps = elapsed.TotalSeconds <= 0 ? 0 : _peepFramesInWindow / elapsed.TotalSeconds;
                _lastPeepFpsWindowStartedAt = now;
                _peepFramesInWindow = 0;
            }
        }
        catch
        {
            // Ignore peep frame refresh errors to keep the UI responsive.
        }
        finally
        {
            _peepRefreshInFlight = false;
        }
    }

    private void RefreshGachaTip()
    {
        GachaInfo = GachaTips[GachaTipRandom.Next(GachaTips.Length)];
    }

    private UiOperationResult ValidateCurrentToolParameters(ToolboxToolKind tool)
    {
        switch (tool)
        {
            case ToolboxToolKind.Recruit:
                if (!ChooseLevel3 && !ChooseLevel4 && !ChooseLevel5 && !ChooseLevel6)
                {
                    return UiOperationResult.Fail(UiErrorCode.ToolboxInvalidParameters, "至少需要选择一个星级用于公招计算。");
                }

                if (!TryParseRecruitMinutes(RecruitLevel3TimeInput, out _)
                    || !TryParseRecruitMinutes(RecruitLevel4TimeInput, out _)
                    || !TryParseRecruitMinutes(RecruitLevel5TimeInput, out _))
                {
                    return UiOperationResult.Fail(UiErrorCode.ToolboxInvalidParameters, "招募时间必须是 60 到 540 之间的 10 分钟整数倍。");
                }

                return UiOperationResult.Ok("Recruit parameters validated.");
            case ToolboxToolKind.OperBox:
                if (!string.Equals(OperBoxMode, "owned", StringComparison.OrdinalIgnoreCase)
                    && !string.Equals(OperBoxMode, "all", StringComparison.OrdinalIgnoreCase))
                {
                    return UiOperationResult.Fail(UiErrorCode.ToolboxInvalidParameters, "干员识别模式仅支持 owned 或 all。");
                }

                return UiOperationResult.Ok("OperBox parameters validated.");
            case ToolboxToolKind.Depot:
                if (!TryParseInt(DepotTopNInput, 1, 500, out _))
                {
                    return UiOperationResult.Fail(UiErrorCode.ToolboxInvalidParameters, "仓库 TopN 必须在 1 到 500 之间。");
                }

                return UiOperationResult.Ok("Depot parameters validated.");
            case ToolboxToolKind.Gacha:
                if (!TryParseInt(GachaDrawCountInput, 1, 10, out var drawCount) || (drawCount != 1 && drawCount != 10))
                {
                    return UiOperationResult.Fail(UiErrorCode.ToolboxInvalidParameters, "抽卡次数仅支持 1 或 10。");
                }

                return UiOperationResult.Ok("Gacha parameters validated.");
            case ToolboxToolKind.VideoRecognition:
                if (!TryParseInt(VideoRecognitionTargetFpsInput, 1, 60, out _))
                {
                    return UiOperationResult.Fail(UiErrorCode.ToolboxInvalidParameters, "窥屏目标帧率必须在 1 到 60 之间。");
                }

                return UiOperationResult.Ok("Peep parameters validated.");
            case ToolboxToolKind.MiniGame:
                if (string.IsNullOrWhiteSpace(MiniGameTaskName))
                {
                    return UiOperationResult.Fail(UiErrorCode.ToolboxInvalidParameters, "小游戏任务名不能为空。");
                }

                if (string.Equals(MiniGameTaskName, MiniGameSecretFrontTaskName, StringComparison.Ordinal)
                    && !MiniGameSecretFrontEndings.Contains(NormalizeToken(MiniGameSecretFrontEnding)))
                {
                    return UiOperationResult.Fail(UiErrorCode.ToolboxInvalidParameters, "当小游戏任务为 MiniGame@SecretFront 时，结局必须为 A~E 之一。");
                }

                return UiOperationResult.Ok("MiniGame parameters validated.");
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
            var failed = UiOperationResult.Fail(
                result.Error?.Code ?? UiErrorCode.SettingsSaveFailed,
                result.Message,
                result.Error?.Details);
            await RecordFailedResultAsync("Toolbox.ConfigBridge", failed, cancellationToken);
        }
    }

    private IReadOnlyDictionary<string, string> BuildBridgeUpdates(ToolboxToolKind tool)
    {
        var updates = new Dictionary<string, string>(StringComparer.Ordinal);
        switch (tool)
        {
            case ToolboxToolKind.Recruit:
                updates[LegacyConfigurationKeys.ChooseLevel3] = ChooseLevel3.ToString();
                updates[LegacyConfigurationKeys.ChooseLevel4] = ChooseLevel4.ToString();
                updates[LegacyConfigurationKeys.ChooseLevel5] = ChooseLevel5.ToString();
                updates[LegacyConfigurationKeys.ChooseLevel6] = ChooseLevel6.ToString();
                updates[LegacyConfigurationKeys.ToolBoxChooseLevel3Time] = RecruitLevel3Time.ToString(CultureInfo.InvariantCulture);
                updates[LegacyConfigurationKeys.ToolBoxChooseLevel4Time] = RecruitLevel4Time.ToString(CultureInfo.InvariantCulture);
                updates[LegacyConfigurationKeys.ToolBoxChooseLevel5Time] = RecruitLevel5Time.ToString(CultureInfo.InvariantCulture);
                updates[LegacyConfigurationKeys.AutoSetTime] = RecruitAutoSetTime.ToString();
                updates[LegacyConfigurationKeys.RecruitmentShowPotential] = RecruitmentShowPotential.ToString();
                break;
            case ToolboxToolKind.Gacha:
                updates[LegacyConfigurationKeys.GachaShowDisclaimerNoMore] = GachaShowDisclaimerNoMore.ToString();
                break;
            case ToolboxToolKind.VideoRecognition:
                updates[LegacyConfigurationKeys.PeepTargetFps] = PeepTargetFps.ToString(CultureInfo.InvariantCulture);
                break;
            case ToolboxToolKind.MiniGame:
                updates[LegacyConfigurationKeys.MiniGameTaskName] = MiniGameTaskName;
                updates[LegacyConfigurationKeys.MiniGameSecretFrontEnding] = MiniGameSecretFrontEnding;
                updates[LegacyConfigurationKeys.MiniGameSecretFrontEvent] = MiniGameSecretFrontEvent;
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
            var failed = UiOperationResult.Fail(UiErrorCode.ToolboxExecutionFailed, warning);
            await RecordFailedResultAsync(ToolboxHistoryLoadScope, failed, cancellationToken);
            return;
        }

        foreach (var record in history)
        {
            ExecutionHistory.Add(record);
        }
    }

    private bool TryReadPersistedHistoryPayload(out string payload)
    {
        payload = string.Empty;
        if (!Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue(ToolboxExecutionHistoryKey, out var node) || node is null)
        {
            return false;
        }

        if (node is JsonValue jsonValue && jsonValue.TryGetValue(out string? raw))
        {
            payload = raw ?? string.Empty;
            return !string.IsNullOrWhiteSpace(payload);
        }

        payload = node.ToJsonString();
        return !string.IsNullOrWhiteSpace(payload);
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
            var entries = JsonSerializer.Deserialize<List<PersistedToolExecutionRecord>>(payload, PersistedPayloadJsonOptions);
            if (entries is null)
            {
                warning = "读取执行历史失败：配置为空。";
                return false;
            }

            foreach (var entry in entries.Where(entry => entry is not null))
            {
                history.Add(new ToolExecutionRecord(
                    entry!.ExecutedAt,
                    entry.ToolName,
                    BuildParameterSummary(entry.ParameterSummary),
                    entry.Success,
                    BuildResultSummary(entry.ResultSummary),
                    NormalizeToken(entry.ErrorCode)));
            }
        }
        catch (Exception ex)
        {
            warning = $"读取执行历史失败：{ex.Message}";
            return false;
        }

        history = history
            .OrderByDescending(record => record.ExecutedAt)
            .Take(MaxHistoryCount)
            .ToList();
        return history.Count > 0 || string.IsNullOrWhiteSpace(warning);
    }

    private async Task PersistExecutionHistoryAsync(CancellationToken cancellationToken)
    {
        var payload = JsonSerializer.Serialize(
            ExecutionHistory.Take(MaxHistoryCount).Select(record => new PersistedToolExecutionRecord(
                record.ExecutedAt,
                record.ToolName,
                record.ParameterSummary,
                record.Success,
                record.ResultSummary,
                record.ErrorCode)));
        var result = await Runtime.SettingsFeatureService.SaveGlobalSettingAsync(ToolboxExecutionHistoryKey, payload, cancellationToken);
        if (!result.Success)
        {
            var failed = UiOperationResult.Fail(result.Error?.Code ?? UiErrorCode.SettingsSaveFailed, result.Message, result.Error?.Details);
            await RecordFailedResultAsync(ToolboxHistorySaveScope, failed, cancellationToken);
        }
    }

    private async Task PersistOperBoxAsync(CancellationToken cancellationToken)
    {
        var payload = JsonSerializer.Serialize(_operBoxOwnedById.Values
            .OrderByDescending(item => item.Rarity)
            .ThenBy(item => item.Id, StringComparer.Ordinal)
            .Select(item => new PersistedOperBoxOperator(
                item.Id,
                item.Name,
                item.Rarity,
                item.Elite,
                item.Level,
                true,
                item.Potential)));
        var result = await Runtime.SettingsFeatureService.SaveGlobalSettingAsync(LegacyConfigurationKeys.OperBoxData, payload, cancellationToken);
        if (!result.Success)
        {
            var failed = UiOperationResult.Fail(result.Error?.Code ?? UiErrorCode.SettingsSaveFailed, result.Message, result.Error?.Details);
            await RecordFailedResultAsync(ToolboxLegacyResultScope, failed, cancellationToken);
        }
    }

    private async Task PersistDepotAsync(CancellationToken cancellationToken)
    {
        var data = new JsonObject();
        foreach (var item in DepotResult.Where(item => item.Count >= 0))
        {
            data[item.Id] = item.Count;
        }

        var payload = new JsonObject
        {
            ["done"] = true,
            ["data"] = data.ToJsonString(),
        };
        if (LastDepotSyncTime is not null)
        {
            payload["syncTime"] = LastDepotSyncTime.Value.ToString("O", CultureInfo.InvariantCulture);
        }

        var result = await Runtime.SettingsFeatureService.SaveGlobalSettingAsync(LegacyConfigurationKeys.DepotResult, payload.ToJsonString(), cancellationToken);
        if (!result.Success)
        {
            var failed = UiOperationResult.Fail(result.Error?.Code ?? UiErrorCode.SettingsSaveFailed, result.Message, result.Error?.Details);
            await RecordFailedResultAsync(ToolboxLegacyResultScope, failed, cancellationToken);
        }
    }

    private void LoadOperBoxDetails()
    {
        _operBoxOwnedById.Clear();
        _operBoxPotential.Clear();
        OperBoxHaveList.Clear();
        OperBoxNotHaveList.Clear();

        if (!Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue(LegacyConfigurationKeys.OperBoxData, out var node) || node is null)
        {
            RebuildOperBoxLists();
            return;
        }

        try
        {
            var payload = node is JsonValue value && value.TryGetValue(out string? raw)
                ? raw
                : node.ToJsonString();
            if (string.IsNullOrWhiteSpace(payload))
            {
                RebuildOperBoxLists();
                return;
            }

            var items = JsonSerializer.Deserialize<List<PersistedOperBoxOperator>>(payload, PersistedPayloadJsonOptions);
            if (items is not null)
            {
                foreach (var item in items.Where(item => item is not null && item.Own))
                {
                    _operBoxOwnedById[item!.Id] = new ToolboxOwnedOperatorState(item.Id, item.Name, item.Rarity, item.Elite, item.Level, item.Potential);
                    _operBoxPotential[item.Id] = item.Potential;
                }
            }
        }
        catch
        {
            // Ignore incompatible persisted payloads.
        }

        RebuildOperBoxLists();
    }

    private void LoadDepotDetails()
    {
        DepotResult.Clear();
        if (!Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue(LegacyConfigurationKeys.DepotResult, out var node) || node is null)
        {
            return;
        }

        try
        {
            var payload = node is JsonValue value && value.TryGetValue(out string? raw)
                ? JsonNode.Parse(raw ?? string.Empty) as JsonObject
                : node as JsonObject ?? JsonNode.Parse(node.ToJsonString()) as JsonObject;
            if (payload is not null)
            {
                ApplyDepotRecognition(payload, updateSyncTime: false);
            }
        }
        catch
        {
            // Ignore incompatible persisted payloads.
        }
    }

    private void LoadMiniGameEntries()
    {
        MiniGameTaskList.Clear();
        foreach (var entry in ToolboxAssetCatalog.GetMiniGameEntries())
        {
            MiniGameTaskList.Add(entry);
        }

        if (!MiniGameTaskList.Any(entry => string.Equals(entry.Value, MiniGameTaskName, StringComparison.Ordinal)))
        {
            MiniGameTaskList.Insert(0, new ToolboxMiniGameEntry(MiniGameTaskName, MiniGameTaskName, string.Empty));
        }

        MiniGameTip = ResolveMiniGameTip(MiniGameTaskName);
    }

    private void UpdateMiniGameSelectionFromTaskName()
    {
        if (MiniGameTaskList.Any(entry => string.Equals(entry.Value, MiniGameTaskName, StringComparison.Ordinal)))
        {
            OnPropertyChanged(nameof(SelectedMiniGameEntry));
            return;
        }

        MiniGameTaskList.Insert(0, new ToolboxMiniGameEntry(MiniGameTaskName, MiniGameTaskName, string.Empty));
        OnPropertyChanged(nameof(SelectedMiniGameEntry));
    }

    private string ResolveMiniGameTip(string taskName)
    {
        var entry = MiniGameTaskList.FirstOrDefault(item => string.Equals(item.Value, taskName, StringComparison.Ordinal));
        if (entry is null)
        {
            return MiniGameNameEmptyTipText;
        }

        return string.IsNullOrWhiteSpace(entry.Tip)
            ? $"当前任务：{entry.Display}"
            : entry.Tip;
    }

    private void LoadBridgeSettings()
    {
        ChooseLevel3 = ReadBoolSetting(LegacyConfigurationKeys.ChooseLevel3, true);
        ChooseLevel4 = ReadBoolSetting(LegacyConfigurationKeys.ChooseLevel4, true);
        ChooseLevel5 = ReadBoolSetting(LegacyConfigurationKeys.ChooseLevel5, true);
        ChooseLevel6 = ReadBoolSetting(LegacyConfigurationKeys.ChooseLevel6, true);
        RecruitLevel3Time = ReadIntSetting(LegacyConfigurationKeys.ToolBoxChooseLevel3Time, 540);
        RecruitLevel4Time = ReadIntSetting(LegacyConfigurationKeys.ToolBoxChooseLevel4Time, 540);
        RecruitLevel5Time = ReadIntSetting(LegacyConfigurationKeys.ToolBoxChooseLevel5Time, 540);
        RecruitAutoSetTime = ReadBoolSetting(LegacyConfigurationKeys.AutoSetTime, true);
        RecruitmentShowPotential = ReadBoolSetting(LegacyConfigurationKeys.RecruitmentShowPotential, true);

        GachaShowDisclaimerNoMore = ReadBoolSetting(LegacyConfigurationKeys.GachaShowDisclaimerNoMore, false);
        GachaShowDisclaimer = !GachaShowDisclaimerNoMore;
        PeepTargetFps = ReadIntSetting(LegacyConfigurationKeys.PeepTargetFps, 20);

        MiniGameTaskName = ReadStringSetting(LegacyConfigurationKeys.MiniGameTaskName, "SS@Store@Begin");
        MiniGameSecretFrontEnding = ReadStringSetting(LegacyConfigurationKeys.MiniGameSecretFrontEnding, "A");
        MiniGameSecretFrontEvent = ReadStringSetting(LegacyConfigurationKeys.MiniGameSecretFrontEvent, string.Empty);
    }

    private int ReadIntSetting(string key, int fallback)
    {
        if (!Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue(key, out var node) || node is null)
        {
            return fallback;
        }

        return int.TryParse(NormalizeToken(node.ToString()), out var parsed) ? parsed : fallback;
    }

    private bool ReadBoolSetting(string key, bool fallback)
    {
        if (!Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue(key, out var node) || node is null)
        {
            return fallback;
        }

        var normalized = NormalizeToken(node.ToString());
        if (bool.TryParse(normalized, out var parsed))
        {
            return parsed;
        }

        return int.TryParse(normalized, out var number) ? number != 0 : fallback;
    }

    private string ReadStringSetting(string key, string fallback)
    {
        if (!Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue(key, out var node) || node is null)
        {
            return fallback;
        }

        var normalized = NormalizeToken(node.ToString());
        return string.IsNullOrWhiteSpace(normalized) ? fallback : normalized;
    }

    private static bool TryResolveTool(int tabIndex, out ToolboxToolKind tool)
    {
        return ToolByTabIndex.TryGetValue(tabIndex, out tool);
    }

    private bool SetTrackedProperty<T>(ref T storage, T value)
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
        CurrentToolParameters = TryResolveTool(SelectedTabIndex, out var tool)
            ? BuildCurrentParameterText(tool)
            : string.Empty;
    }

    private string BuildCurrentParameterText(ToolboxToolKind tool)
    {
        return tool switch
        {
            ToolboxToolKind.Recruit => string.Join(
                ';',
                new[]
                {
                    $"select={BuildRecruitSelectedLevelsSummary()}",
                    $"autoSetTime={RecruitAutoSetTime.ToString().ToLowerInvariant()}",
                    $"level3Time={RecruitLevel3Time}",
                    $"level4Time={RecruitLevel4Time}",
                    $"level5Time={RecruitLevel5Time}",
                    $"showPotential={RecruitmentShowPotential.ToString().ToLowerInvariant()}",
                }),
            ToolboxToolKind.OperBox => $"mode={NormalizeToken(OperBoxMode)}",
            ToolboxToolKind.Depot => $"format={NormalizeToken(DepotFormat)};topN={NormalizeToken(DepotTopNInput)}",
            ToolboxToolKind.Gacha => $"drawCount={NormalizeToken(GachaDrawCountInput)};showDisclaimerNoMore={GachaShowDisclaimerNoMore.ToString().ToLowerInvariant()}",
            ToolboxToolKind.VideoRecognition => $"targetFps={PeepTargetFps}",
            ToolboxToolKind.MiniGame => $"taskName={NormalizeToken(MiniGameTaskName)};secretFrontEnding={NormalizeToken(MiniGameSecretFrontEnding)};secretFrontEvent={NormalizeToken(MiniGameSecretFrontEvent)}",
            _ => string.Empty,
        };
    }

    private string BuildRecruitSelectedLevelsSummary()
    {
        var selected = new List<string>();
        if (ChooseLevel3)
        {
            selected.Add("3");
        }

        if (ChooseLevel4)
        {
            selected.Add("4");
        }

        if (ChooseLevel5)
        {
            selected.Add("5");
        }

        if (ChooseLevel6)
        {
            selected.Add("6");
        }

        return selected.Count == 0 ? "none" : string.Join(',', selected);
    }

    private static int NormalizeRecruitMinutes(int value)
    {
        return value switch
        {
            < 60 => 60,
            > 540 => 540,
            _ => value / 10 * 10,
        };
    }

    private static int NormalizeRecruitMinutePart(int value)
    {
        return Math.Clamp(value / 10 * 10, 0, 50);
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

    private string? ResolveProfileString(string key, string? legacyKey = null, string? fallback = null)
    {
        if (Runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
        {
            if (TryReadProfileString(profile, key, out var value))
            {
                return value;
            }

            if (!string.IsNullOrWhiteSpace(legacyKey) && TryReadProfileString(profile, legacyKey!, out value))
            {
                return value;
            }
        }

        if (!string.IsNullOrWhiteSpace(legacyKey)
            && Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue(legacyKey!, out var node)
            && node is JsonValue globalValue
            && globalValue.TryGetValue(out string? globalText)
            && !string.IsNullOrWhiteSpace(globalText))
        {
            return globalText.Trim();
        }

        return fallback;
    }

    private bool ResolveProfileBool(string key, bool fallback, string? legacyKey = null)
    {
        if (Runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
        {
            if (TryReadProfileBool(profile, key, out var profileValue))
            {
                return profileValue;
            }

            if (!string.IsNullOrWhiteSpace(legacyKey) && TryReadProfileBool(profile, legacyKey!, out profileValue))
            {
                return profileValue;
            }
        }

        if (!string.IsNullOrWhiteSpace(legacyKey)
            && Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue(legacyKey!, out var node)
            && TryReadBoolNode(node, out var globalValue))
        {
            return globalValue;
        }

        return fallback;
    }

    private static bool TryReadProfileString(UnifiedProfile profile, string key, out string value)
    {
        value = string.Empty;
        if (!profile.Values.TryGetValue(key, out var node) || node is not JsonValue jsonValue || !jsonValue.TryGetValue(out string? raw) || string.IsNullOrWhiteSpace(raw))
        {
            return false;
        }

        value = raw.Trim();
        return true;
    }

    private static bool TryReadProfileBool(UnifiedProfile profile, string key, out bool value)
    {
        value = false;
        return profile.Values.TryGetValue(key, out var node) && TryReadBoolNode(node, out value);
    }

    private static bool TryReadBoolNode(JsonNode? node, out bool value)
    {
        value = false;
        if (node is not JsonValue jsonValue)
        {
            return false;
        }

        if (jsonValue.TryGetValue(out bool parsedBool))
        {
            value = parsedBool;
            return true;
        }

        if (jsonValue.TryGetValue(out string? parsedText))
        {
            if (bool.TryParse(parsedText, out parsedBool))
            {
                value = parsedBool;
                return true;
            }

            if (int.TryParse(parsedText, out var number))
            {
                value = number != 0;
                return true;
            }
        }

        return false;
    }

    private string BuildFailureContextDetails(ToolboxToolKind? tool, string errorCode, string stage)
    {
        return JsonSerializer.Serialize(new
        {
            tool = ToolNameOf(tool),
            selectedTabIndex = SelectedTabIndex,
            executionState = ExecutionState.ToString(),
            parameterSummary = BuildParameterSummary(CurrentToolParameters),
            errorCode,
            stage,
            occurredAt = DateTimeOffset.Now,
        });
    }

    private string BuildOperBoxExportText()
    {
        var operators = ToolboxAssetCatalog.GetOperators();
        var clientType = ResolveClientType();
        var language = ResolveCurrentLanguage();
        var exportItems = new List<PersistedOperBoxOperator>();

        foreach (var asset in operators.Values
                     .Where(asset => ToolboxAssetCatalog.IsOperatorAvailableInClient(asset, clientType))
                     .OrderByDescending(asset => asset.Rarity)
                     .ThenBy(asset => asset.Id, StringComparer.Ordinal))
        {
            if (_operBoxOwnedById.TryGetValue(asset.Id, out var owned))
            {
                exportItems.Add(new PersistedOperBoxOperator(
                    owned.Id,
                    owned.Name,
                    owned.Rarity,
                    owned.Elite,
                    owned.Level,
                    true,
                    owned.Potential));
            }
            else
            {
                exportItems.Add(new PersistedOperBoxOperator(
                    asset.Id,
                    ToolboxAssetCatalog.GetLocalizedOperatorName(asset, language),
                    asset.Rarity,
                    0,
                    0,
                    false,
                    0));
            }
        }

        return JsonSerializer.Serialize(exportItems, new JsonSerializerOptions { WriteIndented = true });
    }

    private string BuildArkPlannerExportText()
    {
        var items = DepotResult
            .Where(item => item.Count >= 0)
            .Select(item => new JsonObject
            {
                ["id"] = item.Id,
                ["have"] = item.Count,
                ["name"] = item.Name,
            })
            .ToArray();

        return new JsonObject
        {
            ["@type"] = "@penguin-statistics/depot",
            ["items"] = new JsonArray(items),
        }.ToJsonString();
    }

    private string BuildLoliconExportText()
    {
        var data = new JsonObject();
        foreach (var item in DepotResult.Where(item => item.Count >= 0))
        {
            data[item.Id] = item.Count;
        }

        return data.ToJsonString();
    }

    private static void TryReadDepotCountsFromJson(string payload, IDictionary<string, int> counts)
    {
        if (JsonNode.Parse(payload) is not JsonObject data)
        {
            return;
        }

        foreach (var pair in data)
        {
            if (pair.Value is null)
            {
                continue;
            }

            if (pair.Value is JsonValue value && value.TryGetValue(out int intValue))
            {
                counts[pair.Key] = intValue;
                continue;
            }

            if (int.TryParse(pair.Value.ToString(), out intValue))
            {
                counts[pair.Key] = intValue;
            }
        }
    }

    private static IReadOnlyList<string> ReadStringArray(JsonNode? node)
    {
        if (node is not JsonArray array)
        {
            return Array.Empty<string>();
        }

        return array
            .OfType<JsonValue>()
            .Select(value => value.TryGetValue(out string? text) ? NormalizeToken(text) : string.Empty)
            .Where(text => !string.IsNullOrWhiteSpace(text))
            .ToArray();
    }

    private static string ReadString(JsonObject node, string property)
    {
        return node[property] is JsonValue value && value.TryGetValue(out string? text)
            ? NormalizeToken(text)
            : string.Empty;
    }

    private static int ReadInt(JsonObject node, string property)
    {
        if (node[property] is not JsonValue value)
        {
            return 0;
        }

        if (value.TryGetValue(out int intValue))
        {
            return intValue;
        }

        return value.TryGetValue(out string? text) && int.TryParse(text, out intValue)
            ? intValue
            : 0;
    }

    private static bool ReadBool(JsonObject node, string property)
    {
        return TryReadBoolNode(node[property], out var value) && value;
    }

    private static string GetToolDisplayName(ToolboxToolKind tool)
    {
        return tool switch
        {
            ToolboxToolKind.Recruit => "招募识别",
            ToolboxToolKind.OperBox => "干员识别",
            ToolboxToolKind.Depot => "仓库识别",
            ToolboxToolKind.Gacha => "抽卡",
            ToolboxToolKind.VideoRecognition => "窥屏",
            ToolboxToolKind.MiniGame => "小游戏",
            _ => tool.ToString(),
        };
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

    private static string NormalizeToken(string? text)
    {
        return string.IsNullOrWhiteSpace(text) ? string.Empty : text.Trim();
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
        return normalized.Length <= maxLength ? normalized : normalized[..(maxLength - 3)] + "...";
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
            UiErrorCode.ToolboxInvalidParameters => $"工具参数错误：{fallback} ({UiErrorCode.ToolboxInvalidParameters})",
            UiErrorCode.ToolboxExecutionCancelled => $"工具执行已取消：{fallback} ({UiErrorCode.ToolboxExecutionCancelled})",
            UiErrorCode.ToolNotSupported => $"当前工具不受支持：{fallback} ({UiErrorCode.ToolNotSupported})",
            _ when string.IsNullOrWhiteSpace(code) => $"工具执行失败：{fallback}",
            _ => $"工具执行失败：{fallback} ({code})",
        };
    }

    private static IBrush ResolveStarBrush(int star)
    {
        return star switch
        {
            >= 6 => Brushes.Gold,
            5 => Brushes.Orange,
            4 => Brushes.SkyBlue,
            3 => Brushes.LightGreen,
            _ => Brushes.LightGray,
        };
    }

    private static double ResolvePanelWidth(int count, double itemWidth)
    {
        var columns = Math.Clamp(count, 1, 5);
        return columns * itemWidth;
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

public sealed class RecruitResultLineViewModel : ObservableObject
{
    public RecruitResultLineViewModel(IEnumerable<RecruitResultSegmentViewModel> segments, double spacerHeight = 0)
    {
        Segments = new ObservableCollection<RecruitResultSegmentViewModel>(segments);
        SpacerHeight = spacerHeight;
    }

    public ObservableCollection<RecruitResultSegmentViewModel> Segments { get; }

    public bool HasSegments => Segments.Count > 0;

    public double SpacerHeight { get; }

    public bool HasSpacer => SpacerHeight > 0;

    public string Text => string.Join("    ", Segments.Select(segment => segment.Text));

    public IBrush? Foreground => Segments.LastOrDefault()?.Foreground;

    public static RecruitResultLineViewModel CreateSpacer(double spacerHeight = 10)
        => new([], spacerHeight);
}

public sealed class RecruitResultSegmentViewModel : ObservableObject
{
    private string _text;
    private IBrush? _foreground;

    public RecruitResultSegmentViewModel(string text, IBrush? foreground)
    {
        _text = text;
        _foreground = foreground;
    }

    public string Text
    {
        get => _text;
        set => SetProperty(ref _text, value);
    }

    public IBrush? Foreground
    {
        get => _foreground;
        set => SetProperty(ref _foreground, value);
    }
}

public sealed class OperBoxOperatorItemViewModel : ObservableObject
{
    private int _elite;
    private int _level;
    private int _potential;

    public OperBoxOperatorItemViewModel(string id, string name, int rarity, int elite, int level, int potential, bool own)
    {
        Id = id;
        Name = name;
        Rarity = rarity;
        RarityBrush = ResolveRarityBrush(rarity);
        _elite = elite;
        _level = level;
        _potential = potential;
        Own = own;
    }

    public string Id { get; }

    public string Name { get; }

    public int Rarity { get; }

    public IBrush RarityBrush { get; }

    public bool Own { get; }

    public int Elite
    {
        get => _elite;
        set
        {
            if (SetProperty(ref _elite, value))
            {
                OnPropertyChanged(nameof(EliteIconImage));
            }
        }
    }

    public int Level
    {
        get => _level;
        set
        {
            if (SetProperty(ref _level, value))
            {
                OnPropertyChanged(nameof(LevelDisplay));
            }
        }
    }

    public int Potential
    {
        get => _potential;
        set
        {
            if (SetProperty(ref _potential, value))
            {
                OnPropertyChanged(nameof(PotentialIconImage));
            }
        }
    }

    public string RarityStars => Rarity <= 0 ? string.Empty : new string('★', Rarity);

    public string LevelDisplay => $"Lv.{Level}";

    public Bitmap? EliteIconImage => ToolboxAssetCatalog.ResolveOperatorEliteBitmap(Elite);

    public Bitmap? PotentialIconImage => ToolboxAssetCatalog.ResolveOperatorPotentialBitmap(Potential);

    public string Subtitle => Own
        ? $"{Rarity}★ / 精英 {Elite} / 等级 {Level} / 潜能 {Potential}"
        : $"{Rarity}★ / 未持有";

    private static int NormalizePotential(int potential)
    {
        return potential is >= 1 and <= 6 ? potential : 1;
    }

    private static IBrush ResolveRarityBrush(int rarity)
    {
        return rarity switch
        {
            >= 6 => Brushes.Gold,
            5 => Brushes.Orange,
            4 => Brushes.SkyBlue,
            3 => Brushes.LightGreen,
            _ => Brushes.LightGray,
        };
    }
}

public sealed class DepotItemViewModel : ObservableObject
{
    private int _count;
    private readonly Bitmap? _itemImage;

    public DepotItemViewModel(string id, string name, int count, string? imagePath)
    {
        Id = id;
        Name = name;
        _count = count;
        ImagePath = imagePath;
        _itemImage = ToolboxAssetCatalog.ResolveItemBitmap(id);
    }

    public string Id { get; }

    public string Name { get; }

    public string? ImagePath { get; }

    public Bitmap? ItemImage => _itemImage;

    public int Count
    {
        get => _count;
        set
        {
            if (SetProperty(ref _count, value))
            {
                OnPropertyChanged(nameof(DisplayCount));
            }
        }
    }

    public string DisplayCount => Count.ToString(CultureInfo.InvariantCulture);
}

file sealed record RecruitOperatorProjection(
    JsonObject Operator,
    int Level,
    int Potential);

public sealed record ToolboxNamedOption(string Label, string Value)
{
    public override string ToString() => Label;
}

internal sealed record ToolboxOwnedOperatorState(
    string Id,
    string Name,
    int Rarity,
    int Elite,
    int Level,
    int Potential);

internal sealed record PersistedOperBoxOperator(
    string Id,
    string Name,
    int Rarity,
    int Elite,
    int Level,
    bool Own,
    int Potential);

internal static class ToolboxUiOperationResultExtensions
{
    public static UiOperationResult ToUntyped<T>(this UiOperationResult<T> result)
    {
        return result.Success
            ? UiOperationResult.Ok(result.Message)
            : UiOperationResult.Fail(
                result.Error?.Code ?? UiErrorCode.UiOperationFailed,
                result.Message,
                result.Error?.Details);
    }
}
