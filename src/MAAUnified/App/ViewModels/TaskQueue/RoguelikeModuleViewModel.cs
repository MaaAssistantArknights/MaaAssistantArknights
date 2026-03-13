using System.ComponentModel;
using System.Text.Json.Nodes;
using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Localization;
using MAAUnified.Application.Services.TaskParams;
using MAAUnified.Compat.Constants;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class RoguelikeModuleViewModel : TypedTaskModuleViewModelBase<RoguelikeTaskParamsDto>
{
    private const int ModeExp = 0;
    private const int ModeInvestment = 1;
    private const int ModeCollectible = 4;
    private const int ModeCollapse = 5;
    private const int ModeSquad = 6;
    private const int ModeExploration = 7;
    private const int ModeFindPlaytime = 20001;
    private const string DelayAbortUntilCombatCompleteConfigKey = ConfigurationKeys.RoguelikeDelayAbortUntilCombatComplete;
    private const string OperNameLanguageClientMode = "OperNameLanguageClient";
    private const string OperNameLanguageForcePrefix = "OperNameLanguageForce.";

    private static readonly IReadOnlyDictionary<string, string> ClientTypeLanguageMap =
        new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["Official"] = "zh-cn",
            ["Bilibili"] = "zh-cn",
            ["YoStarEN"] = "en-us",
            ["YoStarJP"] = "ja-jp",
            ["YoStarKR"] = "ko-kr",
            ["txwy"] = "zh-tw",
        };

    private static readonly object BattleDataCacheLock = new();
    private static BattleDataCache? _battleDataCache;

    private static readonly (string Value, string TextKey, string Fallback)[] ThemeOptionSpecs =
    [
        ("Phantom", "RoguelikeThemePhantom", "傀影"),
        ("Mizuki", "RoguelikeThemeMizuki", "水月"),
        ("Sami", "RoguelikeThemeSami", "萨米"),
        ("Sarkaz", "RoguelikeThemeSarkaz", "萨卡兹"),
        ("JieGarden", "RoguelikeThemeJieGarden", "界园"),
    ];

    private static readonly Dictionary<string, IReadOnlyList<(string TextKey, string Value, string Fallback)>> SquadDictionary =
        new(StringComparer.Ordinal)
        {
            ["Phantom_Default"] =
            [
                ("GatheringSquad", "集群分队", "集群分队"),
                ("SpearheadSquad", "矛头分队", "矛头分队"),
                ("ResearchSquad", "研究分队", "研究分队"),
            ],
            ["Mizuki_Default"] =
            [
                ("GatheringSquad", "集群分队", "集群分队"),
                ("SpearheadSquad", "矛头分队", "矛头分队"),
                ("IS2NewSquad1", "心胜于物分队", "心胜于物分队"),
                ("IS2NewSquad2", "物尽其用分队", "物尽其用分队"),
                ("IS2NewSquad3", "以人为本分队", "以人为本分队"),
                ("ResearchSquad", "研究分队", "研究分队"),
            ],
            ["Sami_Default"] =
            [
                ("GatheringSquad", "集群分队", "集群分队"),
                ("SpearheadSquad", "矛头分队", "矛头分队"),
                ("IS3NewSquad1", "永恒狩猎分队", "永恒狩猎分队"),
                ("IS3NewSquad2", "生活至上分队", "生活至上分队"),
                ("IS3NewSquad3", "科学主义分队", "科学主义分队"),
                ("IS3NewSquad4", "特训分队", "特训分队"),
            ],
            ["Sarkaz_1"] =
            [
                ("GatheringSquad", "集群分队", "集群分队"),
                ("SpearheadSquad", "矛头分队", "矛头分队"),
                ("IS4NewSquad2", "博闻广记分队", "博闻广记分队"),
                ("IS4NewSquad3", "蓝图测绘分队", "蓝图测绘分队"),
                ("IS4NewSquad6", "点刺成锭分队", "点刺成锭分队"),
                ("IS4NewSquad7", "拟态学者分队", "拟态学者分队"),
            ],
            ["Sarkaz_Default"] =
            [
                ("GatheringSquad", "集群分队", "集群分队"),
                ("SpearheadSquad", "矛头分队", "矛头分队"),
                ("IS4NewSquad1", "魂灵护送分队", "魂灵护送分队"),
                ("IS4NewSquad2", "博闻广记分队", "博闻广记分队"),
                ("IS4NewSquad3", "蓝图测绘分队", "蓝图测绘分队"),
                ("IS4NewSquad4", "因地制宜分队", "因地制宜分队"),
                ("IS4NewSquad5", "异想天开分队", "异想天开分队"),
                ("IS4NewSquad6", "点刺成锭分队", "点刺成锭分队"),
                ("IS4NewSquad7", "拟态学者分队", "拟态学者分队"),
                ("IS4NewSquad8", "专业人士分队", "专业人士分队"),
            ],
            ["JieGarden_Default"] =
            [
                ("SpecialForceSquad", "特勤分队", "特勤分队"),
                ("IS5NewSquad1", "高台突破分队", "高台突破分队"),
                ("IS5NewSquad2", "地面突破分队", "地面突破分队"),
                ("IS5NewSquad3", "游客分队", "游客分队"),
                ("IS5NewSquad4", "司岁台分队", "司岁台分队"),
                ("IS5NewSquad5", "天师府分队", "天师府分队"),
                ("IS5NewSquad6", "花团锦簇分队", "花团锦簇分队"),
                ("IS5NewSquad7", "棋行险着分队", "棋行险着分队"),
                ("IS5NewSquad8", "岁影回音分队", "岁影回音分队"),
                ("IS5NewSquad9", "代理人分队", "代理人分队"),
                ("IS5NewSquad10", "知学分队", "知学分队"),
                ("IS5NewSquad11", "商贾分队", "商贾分队"),
            ],
        };

    private static readonly IReadOnlyList<(string TextKey, string Value, string Fallback)> CommonSquads =
    [
        ("LeaderSquad", "指挥分队", "指挥分队"),
        ("SupportSquad", "后勤分队", "后勤分队"),
        ("TacticalAssaultOperative", "突击战术分队", "突击战术分队"),
        ("TacticalFortificationOperative", "堡垒战术分队", "堡垒战术分队"),
        ("TacticalRangedOperative", "远程战术分队", "远程战术分队"),
        ("TacticalDestructionOperative", "破坏战术分队", "破坏战术分队"),
        ("First-ClassSquad", "高规格分队", "高规格分队"),
    ];

    private static readonly HashSet<string> ProfessionalSquads =
    [
        "突击战术分队",
        "堡垒战术分队",
        "远程战术分队",
        "破坏战术分队",
    ];

    private string _theme = "JieGarden";
    private int _mode;
    private int _difficulty = int.MaxValue;
    private int _startsCount = 999999;
    private bool _investmentEnabled = true;
    private bool _investmentWithMoreScore;
    private int _investmentsCount = 999;
    private bool _stopWhenInvestmentFull;
    private string _squad = string.Empty;
    private string _roles = string.Empty;
    private string _coreChar = string.Empty;
    private bool _useSupport;
    private bool _useNonfriendSupport;
    private bool _refreshTraderWithDice;
    private bool _stopAtFinalBoss;
    private bool _stopAtMaxLevel;
    private bool _collectibleModeShopping;
    private string _collectibleModeSquad = string.Empty;
    private bool _startWithEliteTwo;
    private bool _onlyStartWithEliteTwo;
    private bool _collectibleHotWater;
    private bool _collectibleShield;
    private bool _collectibleIngot;
    private bool _collectibleHope;
    private bool _collectibleRandom;
    private bool _collectibleKey;
    private bool _collectibleDice;
    private bool _collectibleIdeas;
    private bool _collectibleTicket;
    private bool _monthlySquadAutoIterate = true;
    private bool _monthlySquadCheckComms = true;
    private bool _deepExplorationAutoIterate = true;
    private int _findPlayTimeTarget = 1;
    private bool _firstFloorFoldartalEnabled;
    private string _firstFloorFoldartal = string.Empty;
    private bool _startFoldartalListEnabled;
    private string _startFoldartalListText = string.Empty;
    private string _expectedCollapsalParadigmsText = string.Empty;
    private bool _startWithSeedEnabled;
    private string _startWithSeed = string.Empty;
    private bool _delayAbortUntilCombatComplete;
    private bool _suppressDelayAbortPersist;

    private bool _suppressOptionRefresh;
    private IReadOnlyList<TaskModuleOption> _themeOptions = [];
    private IReadOnlyList<IntOption> _modeOptions = [];
    private IReadOnlyList<IntOption> _difficultyOptions = [];
    private IReadOnlyList<TaskModuleOption> _squadOptions = [];
    private IReadOnlyList<TaskModuleOption> _rolesOptions = [];
    private IReadOnlyList<IntOption> _findPlayTimeTargetOptions = [];
    private IReadOnlyList<TaskModuleOption> _coreCharOptions = [];
    private IReadOnlyList<string> _coreCharNameOptions = [];

    public RoguelikeModuleViewModel(MAAUnifiedRuntime runtime, LocalizedTextMap texts)
        : base(runtime, texts, "TaskQueue.Roguelike")
    {
        ApplyPersistentDelayAbortSetting(ResolveDelayAbortUntilCombatComplete());
        Texts.PropertyChanged += OnTextsPropertyChanged;
        RebuildOptionState();
    }

    public sealed record IntOption(int Value, string DisplayName);

    public IReadOnlyList<TaskModuleOption> ThemeOptions => _themeOptions;

    public IReadOnlyList<IntOption> ModeOptions => _modeOptions;

    public IReadOnlyList<IntOption> DifficultyOptions => _difficultyOptions;

    public IReadOnlyList<TaskModuleOption> SquadOptions => _squadOptions;

    public IReadOnlyList<TaskModuleOption> RolesOptions => _rolesOptions;

    public IReadOnlyList<IntOption> FindPlayTimeTargetOptions => _findPlayTimeTargetOptions;

    public IReadOnlyList<TaskModuleOption> CoreCharOptions => _coreCharOptions;

    public IReadOnlyList<string> CoreCharNameOptions => _coreCharNameOptions;

    public TaskModuleOption? SelectedThemeOption
    {
        get => ResolveSelectedOption(ThemeOptions, Theme);
        set => Theme = value?.Type ?? "JieGarden";
    }

    public IntOption? SelectedModeOption
    {
        get => ResolveSelectedOption(ModeOptions, Mode);
        set => Mode = value?.Value ?? ModeExp;
    }

    public IntOption? SelectedDifficultyOption
    {
        get => ResolveSelectedOption(DifficultyOptions, Difficulty);
        set => Difficulty = value?.Value ?? int.MaxValue;
    }

    public TaskModuleOption? SelectedSquadOption
    {
        get => ResolveSelectedOption(SquadOptions, Squad);
        set => Squad = value?.Type ?? string.Empty;
    }

    public TaskModuleOption? SelectedCollectibleModeSquadOption
    {
        get => ResolveSelectedOption(SquadOptions, CollectibleModeSquad);
        set => CollectibleModeSquad = value?.Type ?? string.Empty;
    }

    public TaskModuleOption? SelectedRolesOption
    {
        get => ResolveSelectedOption(RolesOptions, Roles);
        set => Roles = value?.Type ?? string.Empty;
    }

    public IntOption? SelectedFindPlayTimeTargetOption
    {
        get => ResolveSelectedOption(FindPlayTimeTargetOptions, FindPlayTimeTarget);
        set => FindPlayTimeTarget = value?.Value ?? 1;
    }

    public TaskModuleOption? SelectedCoreCharOption
    {
        get => ResolveSelectedOption(CoreCharOptions, CoreChar);
        set => CoreChar = value?.Type ?? string.Empty;
    }

    public bool IsThemeJieGarden => string.Equals(Theme, "JieGarden", StringComparison.OrdinalIgnoreCase);

    public bool IsThemePhantom => string.Equals(Theme, "Phantom", StringComparison.OrdinalIgnoreCase);

    public bool IsThemeMizuki => string.Equals(Theme, "Mizuki", StringComparison.OrdinalIgnoreCase);

    public bool IsThemeSami => string.Equals(Theme, "Sami", StringComparison.OrdinalIgnoreCase);

    public bool IsThemeSarkaz => string.Equals(Theme, "Sarkaz", StringComparison.OrdinalIgnoreCase);

    public bool IsSquadProfessional => Mode == ModeCollectible && !IsThemePhantom && ProfessionalSquads.Contains(Squad);

    public bool IsSquadFoldartal => Mode == ModeCollectible && IsThemeSami && string.Equals(Squad, "生活至上分队", StringComparison.Ordinal);

    public bool ShowCollectibleModeSquad => Mode == ModeCollectible;

    public bool ShowFindPlayTimeTarget => Mode == ModeFindPlaytime && IsThemeJieGarden;

    public bool ShowJieGardenDifficultyTip => Mode == ModeInvestment && IsThemeJieGarden;

    public bool CanToggleInvestmentEnabled => Mode != ModeInvestment;

    public bool ShowStopWhenInvestmentFull => InvestmentEnabled && Mode != ModeCollectible;

    public bool ShowInvestmentWithMoreScore => Mode == ModeInvestment;

    public bool ShowCollectibleModeShopping => Mode == ModeCollectible;

    public bool ShowInvestmentsCount => InvestmentEnabled;

    public bool ShowRefreshTraderWithDice => IsThemeMizuki;

    public bool ShowStartWithEliteTwo => IsSquadProfessional && (IsThemeMizuki || IsThemeSami);

    public bool EffectiveOnlyStartWithEliteTwo => OnlyStartWithEliteTwo && StartWithEliteTwo && ShowStartWithEliteTwo;

    public bool ShowOnlyStartWithEliteTwoOption => StartWithEliteTwo && ShowStartWithEliteTwo && Mode == ModeCollectible;

    public bool ShowCollectibleStartRewardOptions => Mode == ModeCollectible && !EffectiveOnlyStartWithEliteTwo;

    public bool ShowCollectibleHotWater => true;

    public bool ShowCollectibleShield => true;

    public bool ShowCollectibleIngot => true;

    public bool ShowCollectibleHope => !IsThemeJieGarden;

    public bool ShowCollectibleRandom => true;

    public bool ShowCollectibleKey => IsThemeMizuki;

    public bool ShowCollectibleDice => IsThemeMizuki;

    public bool ShowCollectibleIdeas => IsThemeSarkaz;

    public bool ShowCollectibleTicket => IsThemeJieGarden;

    public bool ShowFirstFloorFoldartalToggle => Mode == ModeCollectible && IsThemeSami;

    public bool ShowFirstFloorFoldartalText => ShowFirstFloorFoldartalToggle && FirstFloorFoldartalEnabled;

    public bool ShowStartFoldartalListToggle => IsSquadFoldartal;

    public bool ShowStartFoldartalListText => ShowStartFoldartalListToggle && StartFoldartalListEnabled;

    public bool ShowExpectedCollapsalParadigms => Mode == ModeCollapse;

    public bool CanUseSupport => !string.IsNullOrWhiteSpace(CoreChar);

    public bool ShowUseNonfriendSupport => UseSupport && CanUseSupport;

    public bool ShowStopAtFinalBoss => Mode == ModeExp && !IsThemePhantom;

    public bool ShowMonthlySquadAutoIterate => Mode == ModeSquad;

    public bool ShowMonthlySquadCheckComms => Mode == ModeSquad && MonthlySquadAutoIterate;

    public bool ShowDeepExplorationAutoIterate => Mode == ModeExploration;

    public bool ShowStopAtMaxLevel => Mode == ModeExp;

    public bool ShowStartWithSeedToggle => IsThemeJieGarden;

    public bool ShowStartWithSeedInput => ShowStartWithSeedToggle && StartWithSeedEnabled;

    public string CollectibleStartRewardSummary => BuildCollectibleStartRewardSummary();

    public bool DelayAbortUntilCombatComplete
    {
        get => _delayAbortUntilCombatComplete;
        set
        {
            if (!SetProperty(ref _delayAbortUntilCombatComplete, value))
            {
                return;
            }

            if (!_suppressDelayAbortPersist)
            {
                _ = PersistDelayAbortUntilCombatCompleteAsync(value);
            }
        }
    }

    public Task ReloadPersistentConfigAsync(CancellationToken cancellationToken = default)
    {
        ApplyPersistentDelayAbortSetting(ResolveDelayAbortUntilCombatComplete());
        return Task.CompletedTask;
    }

    public string CoreCharTipText => Texts.GetOrDefault("Roguelike.CoreCharTip", "根据当前主题选择开局核心干员。");

    public string Theme
    {
        get => _theme;
        set
        {
            var normalized = string.IsNullOrWhiteSpace(value) ? "JieGarden" : value.Trim();
            if (!SetTrackedProperty(ref _theme, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(SelectedThemeOption));
            if (!_suppressOptionRefresh)
            {
                RebuildOptionState();
            }
            else
            {
                RaiseComputedPropertyChanges();
            }
        }
    }

    public int Mode
    {
        get => _mode;
        set
        {
            if (!SetTrackedProperty(ref _mode, value))
            {
                return;
            }

            if (_mode == ModeInvestment && !InvestmentEnabled)
            {
                InvestmentEnabled = true;
            }

            OnPropertyChanged(nameof(SelectedModeOption));
            if (!_suppressOptionRefresh)
            {
                RebuildOptionState();
            }
            else
            {
                RaiseComputedPropertyChanges();
            }
        }
    }

    public int Difficulty
    {
        get => _difficulty;
        set
        {
            if (!SetTrackedProperty(ref _difficulty, value))
            {
                return;
            }

            OnPropertyChanged(nameof(SelectedDifficultyOption));
        }
    }

    public int StartsCount
    {
        get => _startsCount;
        set => SetTrackedProperty(ref _startsCount, Math.Max(0, value));
    }

    public bool InvestmentEnabled
    {
        get => _investmentEnabled;
        set
        {
            var normalized = Mode == ModeInvestment ? true : value;
            if (!SetTrackedProperty(ref _investmentEnabled, normalized))
            {
                return;
            }

            RaiseComputedPropertyChanges();
        }
    }

    public bool InvestmentWithMoreScore
    {
        get => _investmentWithMoreScore;
        set => SetTrackedProperty(ref _investmentWithMoreScore, value);
    }

    public int InvestmentsCount
    {
        get => _investmentsCount;
        set => SetTrackedProperty(ref _investmentsCount, Math.Max(0, value));
    }

    public bool StopWhenInvestmentFull
    {
        get => _stopWhenInvestmentFull;
        set => SetTrackedProperty(ref _stopWhenInvestmentFull, value);
    }

    public string Squad
    {
        get => _squad;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (!SetTrackedProperty(ref _squad, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(SelectedSquadOption));
            RaiseComputedPropertyChanges();
        }
    }

    public string Roles
    {
        get => _roles;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (!SetTrackedProperty(ref _roles, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(SelectedRolesOption));
        }
    }

    public string CoreChar
    {
        get => _coreChar;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (!SetTrackedProperty(ref _coreChar, normalized))
            {
                return;
            }

            if (string.IsNullOrWhiteSpace(normalized) && UseSupport)
            {
                UseSupport = false;
            }

            OnPropertyChanged(nameof(SelectedCoreCharOption));
            RaiseComputedPropertyChanges();
        }
    }

    public bool UseSupport
    {
        get => _useSupport;
        set
        {
            var normalized = value;
            if (normalized && StartWithEliteTwo && IsSquadProfessional)
            {
                StartWithEliteTwo = false;
            }

            if (!SetTrackedProperty(ref _useSupport, normalized))
            {
                return;
            }

            if (!normalized && UseNonfriendSupport)
            {
                UseNonfriendSupport = false;
            }

            RaiseComputedPropertyChanges();
        }
    }

    public bool UseNonfriendSupport
    {
        get => _useNonfriendSupport;
        set => SetTrackedProperty(ref _useNonfriendSupport, value);
    }

    public bool RefreshTraderWithDice
    {
        get => _refreshTraderWithDice;
        set => SetTrackedProperty(ref _refreshTraderWithDice, value);
    }

    public bool StopAtFinalBoss
    {
        get => _stopAtFinalBoss;
        set => SetTrackedProperty(ref _stopAtFinalBoss, value);
    }

    public bool StopAtMaxLevel
    {
        get => _stopAtMaxLevel;
        set => SetTrackedProperty(ref _stopAtMaxLevel, value);
    }

    public bool CollectibleModeShopping
    {
        get => _collectibleModeShopping;
        set => SetTrackedProperty(ref _collectibleModeShopping, value);
    }

    public string CollectibleModeSquad
    {
        get => _collectibleModeSquad;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (!SetTrackedProperty(ref _collectibleModeSquad, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(SelectedCollectibleModeSquadOption));
        }
    }

    public bool StartWithEliteTwo
    {
        get => _startWithEliteTwo;
        set
        {
            if (value && UseSupport)
            {
                UseSupport = false;
            }

            if (!value && OnlyStartWithEliteTwo)
            {
                OnlyStartWithEliteTwo = false;
            }

            if (!SetTrackedProperty(ref _startWithEliteTwo, value))
            {
                return;
            }

            RaiseComputedPropertyChanges();
        }
    }

    public bool OnlyStartWithEliteTwo
    {
        get => _onlyStartWithEliteTwo;
        set
        {
            if (!SetTrackedProperty(ref _onlyStartWithEliteTwo, value))
            {
                return;
            }

            RaiseComputedPropertyChanges();
        }
    }

    public bool CollectibleHotWater
    {
        get => _collectibleHotWater;
        set => SetCollectibleRewardFlag(ref _collectibleHotWater, value);
    }

    public bool CollectibleShield
    {
        get => _collectibleShield;
        set => SetCollectibleRewardFlag(ref _collectibleShield, value);
    }

    public bool CollectibleIngot
    {
        get => _collectibleIngot;
        set => SetCollectibleRewardFlag(ref _collectibleIngot, value);
    }

    public bool CollectibleHope
    {
        get => _collectibleHope;
        set => SetCollectibleRewardFlag(ref _collectibleHope, value);
    }

    public bool CollectibleRandom
    {
        get => _collectibleRandom;
        set => SetCollectibleRewardFlag(ref _collectibleRandom, value);
    }

    public bool CollectibleKey
    {
        get => _collectibleKey;
        set => SetCollectibleRewardFlag(ref _collectibleKey, value);
    }

    public bool CollectibleDice
    {
        get => _collectibleDice;
        set => SetCollectibleRewardFlag(ref _collectibleDice, value);
    }

    public bool CollectibleIdeas
    {
        get => _collectibleIdeas;
        set => SetCollectibleRewardFlag(ref _collectibleIdeas, value);
    }

    public bool CollectibleTicket
    {
        get => _collectibleTicket;
        set => SetCollectibleRewardFlag(ref _collectibleTicket, value);
    }

    public bool MonthlySquadAutoIterate
    {
        get => _monthlySquadAutoIterate;
        set
        {
            if (!SetTrackedProperty(ref _monthlySquadAutoIterate, value))
            {
                return;
            }

            RaiseComputedPropertyChanges();
        }
    }

    public bool MonthlySquadCheckComms
    {
        get => _monthlySquadCheckComms;
        set => SetTrackedProperty(ref _monthlySquadCheckComms, value);
    }

    public bool DeepExplorationAutoIterate
    {
        get => _deepExplorationAutoIterate;
        set => SetTrackedProperty(ref _deepExplorationAutoIterate, value);
    }

    public int FindPlayTimeTarget
    {
        get => _findPlayTimeTarget;
        set
        {
            var normalized = Math.Clamp(value, 1, 3);
            if (!SetTrackedProperty(ref _findPlayTimeTarget, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(SelectedFindPlayTimeTargetOption));
        }
    }

    public bool FirstFloorFoldartalEnabled
    {
        get => _firstFloorFoldartalEnabled;
        set
        {
            if (!SetTrackedProperty(ref _firstFloorFoldartalEnabled, value))
            {
                return;
            }

            RaiseComputedPropertyChanges();
        }
    }

    public string FirstFloorFoldartal
    {
        get => _firstFloorFoldartal;
        set
        {
            var normalized = (value ?? string.Empty)
                .Replace('；', ';')
                .Trim();
            SetTrackedProperty(ref _firstFloorFoldartal, normalized);
        }
    }

    public bool StartFoldartalListEnabled
    {
        get => _startFoldartalListEnabled;
        set
        {
            if (!SetTrackedProperty(ref _startFoldartalListEnabled, value))
            {
                return;
            }

            RaiseComputedPropertyChanges();
        }
    }

    public string StartFoldartalListText
    {
        get => _startFoldartalListText;
        set
        {
            var normalized = (value ?? string.Empty)
                .Replace('；', ';')
                .Trim();
            SetTrackedProperty(ref _startFoldartalListText, normalized);
        }
    }

    public string ExpectedCollapsalParadigmsText
    {
        get => _expectedCollapsalParadigmsText;
        set
        {
            var normalized = (value ?? string.Empty)
                .Replace('；', ';')
                .Trim();
            SetTrackedProperty(ref _expectedCollapsalParadigmsText, normalized);
        }
    }

    public bool StartWithSeedEnabled
    {
        get => _startWithSeedEnabled;
        set
        {
            if (!SetTrackedProperty(ref _startWithSeedEnabled, value))
            {
                return;
            }

            RaiseComputedPropertyChanges();
        }
    }

    public string StartWithSeed
    {
        get => _startWithSeed;
        set
        {
            var normalized = (value ?? string.Empty)
                .Replace('；', ';')
                .Trim();
            SetTrackedProperty(ref _startWithSeed, normalized);
        }
    }

    protected override Task<UiOperationResult<RoguelikeTaskParamsDto>> LoadDtoAsync(int index, CancellationToken cancellationToken)
    {
        return Runtime.TaskQueueFeatureService.GetRoguelikeParamsAsync(index, cancellationToken);
    }

    protected override Task<UiOperationResult> SaveDtoAsync(int index, RoguelikeTaskParamsDto dto, CancellationToken cancellationToken)
    {
        return Runtime.TaskQueueFeatureService.SaveRoguelikeParamsAsync(index, dto, cancellationToken);
    }

    protected override TaskCompileOutput CompileDto(RoguelikeTaskParamsDto dto, UnifiedProfile profile, UnifiedConfig config)
    {
        return TaskParamCompiler.CompileRoguelike(dto, profile, config);
    }

    protected override void ApplyDto(RoguelikeTaskParamsDto dto)
    {
        _suppressOptionRefresh = true;
        try
        {
            Theme = dto.Theme;
            Mode = dto.Mode;
            Difficulty = dto.Difficulty;
            StartsCount = dto.StartsCount;
            InvestmentEnabled = dto.InvestmentEnabled;
            InvestmentWithMoreScore = dto.InvestmentWithMoreScore;
            InvestmentsCount = dto.InvestmentsCount;
            StopWhenInvestmentFull = dto.StopWhenInvestmentFull;
            Squad = dto.Squad;
            Roles = dto.Roles;
            CoreChar = dto.CoreChar;
            UseSupport = dto.UseSupport;
            UseNonfriendSupport = dto.UseNonfriendSupport;
            RefreshTraderWithDice = dto.RefreshTraderWithDice;
            StopAtFinalBoss = dto.StopAtFinalBoss;
            StopAtMaxLevel = dto.StopAtMaxLevel;
            CollectibleModeShopping = dto.CollectibleModeShopping;
            CollectibleModeSquad = dto.CollectibleModeSquad;
            StartWithEliteTwo = dto.StartWithEliteTwo;
            OnlyStartWithEliteTwo = dto.OnlyStartWithEliteTwo;
            CollectibleHotWater = dto.CollectibleModeStartList.HotWater;
            CollectibleShield = dto.CollectibleModeStartList.Shield;
            CollectibleIngot = dto.CollectibleModeStartList.Ingot;
            CollectibleHope = dto.CollectibleModeStartList.Hope;
            CollectibleRandom = dto.CollectibleModeStartList.Random;
            CollectibleKey = dto.CollectibleModeStartList.Key;
            CollectibleDice = dto.CollectibleModeStartList.Dice;
            CollectibleIdeas = dto.CollectibleModeStartList.Ideas;
            CollectibleTicket = dto.CollectibleModeStartList.Ticket;
            MonthlySquadAutoIterate = dto.MonthlySquadAutoIterate;
            MonthlySquadCheckComms = dto.MonthlySquadCheckComms;
            DeepExplorationAutoIterate = dto.DeepExplorationAutoIterate;
            FindPlayTimeTarget = dto.FindPlayTimeTarget;
            FirstFloorFoldartalEnabled = !string.IsNullOrWhiteSpace(dto.FirstFloorFoldartal);
            FirstFloorFoldartal = dto.FirstFloorFoldartal;
            StartFoldartalListEnabled = dto.StartFoldartalList.Count > 0;
            StartFoldartalListText = string.Join(";", dto.StartFoldartalList);
            ExpectedCollapsalParadigmsText = string.Join(";", dto.ExpectedCollapsalParadigms);
            StartWithSeedEnabled = !string.IsNullOrWhiteSpace(dto.StartWithSeed);
            StartWithSeed = dto.StartWithSeed;
        }
        finally
        {
            _suppressOptionRefresh = false;
        }

        RebuildOptionState();
    }

    protected override RoguelikeTaskParamsDto BuildDto()
    {
        var startFoldartalList = ShowStartFoldartalListText
            ? ParseDelimitedLines(StartFoldartalListText)
            : [];
        if (startFoldartalList.Count > 3)
        {
            startFoldartalList = [.. startFoldartalList.Take(3)];
        }

        var expectedCollapsalParadigms = ShowExpectedCollapsalParadigms
            ? ParseDelimitedLines(ExpectedCollapsalParadigmsText)
            : [];

        var effectiveStartWithEliteTwo = StartWithEliteTwo && ShowStartWithEliteTwo;
        var effectiveOnlyStartWithEliteTwo = OnlyStartWithEliteTwo && effectiveStartWithEliteTwo;
        var effectiveSupport = UseSupport && CanUseSupport;

        return new RoguelikeTaskParamsDto
        {
            Mode = Mode,
            Theme = string.IsNullOrWhiteSpace(Theme) ? "JieGarden" : Theme.Trim(),
            Difficulty = Difficulty,
            StartsCount = Math.Max(0, StartsCount),
            InvestmentEnabled = Mode == ModeInvestment ? true : InvestmentEnabled,
            InvestmentWithMoreScore = InvestmentWithMoreScore,
            InvestmentsCount = Math.Max(0, InvestmentsCount),
            StopWhenInvestmentFull = StopWhenInvestmentFull,
            Squad = Squad.Trim(),
            Roles = Roles.Trim(),
            CoreChar = CoreChar.Trim(),
            UseSupport = effectiveSupport,
            UseNonfriendSupport = effectiveSupport && UseNonfriendSupport,
            RefreshTraderWithDice = RefreshTraderWithDice,
            StopAtFinalBoss = StopAtFinalBoss,
            StopAtMaxLevel = StopAtMaxLevel,
            CollectibleModeShopping = CollectibleModeShopping,
            CollectibleModeSquad = CollectibleModeSquad.Trim(),
            StartWithEliteTwo = effectiveStartWithEliteTwo,
            OnlyStartWithEliteTwo = effectiveOnlyStartWithEliteTwo,
            CollectibleModeStartList = new RoguelikeCollectibleStartListDto
            {
                HotWater = ShowCollectibleHotWater && CollectibleHotWater,
                Shield = ShowCollectibleShield && CollectibleShield,
                Ingot = ShowCollectibleIngot && CollectibleIngot,
                Hope = ShowCollectibleHope && CollectibleHope,
                Random = ShowCollectibleRandom && CollectibleRandom,
                Key = ShowCollectibleKey && CollectibleKey,
                Dice = ShowCollectibleDice && CollectibleDice,
                Ideas = ShowCollectibleIdeas && CollectibleIdeas,
                Ticket = ShowCollectibleTicket && CollectibleTicket,
            },
            MonthlySquadAutoIterate = MonthlySquadAutoIterate,
            MonthlySquadCheckComms = MonthlySquadCheckComms,
            DeepExplorationAutoIterate = DeepExplorationAutoIterate,
            FindPlayTimeTarget = Math.Clamp(FindPlayTimeTarget, 1, 3),
            FirstFloorFoldartal = ShowFirstFloorFoldartalText ? FirstFloorFoldartal.Trim() : string.Empty,
            StartFoldartalList = startFoldartalList,
            ExpectedCollapsalParadigms = expectedCollapsalParadigms,
            StartWithSeed = ShowStartWithSeedInput ? StartWithSeed.Trim() : string.Empty,
        };
    }

    protected override IReadOnlyList<TaskValidationIssue> ValidateBeforeSave()
    {
        var issues = new List<TaskValidationIssue>();
        if (ShowStartFoldartalListText && ContainsStructuredMarkers(StartFoldartalListText))
        {
            issues.Add(new TaskValidationIssue(
                "DelimitedInputParseFailed",
                "roguelike.start_foldartal_list",
                "Start foldartal list only supports plain delimiter-separated text."));
        }

        if (ShowExpectedCollapsalParadigms && ContainsStructuredMarkers(ExpectedCollapsalParadigmsText))
        {
            issues.Add(new TaskValidationIssue(
                "DelimitedInputParseFailed",
                "roguelike.expected_collapsal_paradigms",
                "Expected collapsal paradigms only supports plain delimiter-separated text."));
        }

        return issues;
    }

    private void OnTextsPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (!string.IsNullOrEmpty(e.PropertyName)
            && !string.Equals(e.PropertyName, nameof(LocalizedTextMap.Language), StringComparison.Ordinal)
            && !string.Equals(e.PropertyName, "Item[]", StringComparison.Ordinal))
        {
            return;
        }

        RebuildOptionState();
    }

    private void RebuildOptionState()
    {
        _suppressOptionRefresh = true;
        try
        {
            _themeOptions = ThemeOptionSpecs
                .Select(spec => new TaskModuleOption(spec.Value, Texts.GetOrDefault(spec.TextKey, spec.Fallback)))
                .ToArray();
            if (!_themeOptions.Any(option => string.Equals(option.Type, Theme, StringComparison.OrdinalIgnoreCase)))
            {
                SetTrackedProperty(ref _theme, "JieGarden", nameof(Theme));
            }

            _modeOptions = BuildModeOptions();
            if (!_modeOptions.Any(option => option.Value == Mode))
            {
                SetTrackedProperty(ref _mode, _modeOptions[0].Value, nameof(Mode));
            }

            if (_mode == ModeInvestment && !_investmentEnabled)
            {
                SetTrackedProperty(ref _investmentEnabled, true, nameof(InvestmentEnabled));
            }

            _difficultyOptions = BuildDifficultyOptions();
            if (!_difficultyOptions.Any(option => option.Value == Difficulty))
            {
                SetTrackedProperty(ref _difficulty, -1, nameof(Difficulty));
            }

            _rolesOptions = BuildRolesOptions();
            if (!_rolesOptions.Any(option => string.Equals(option.Type, Roles, StringComparison.Ordinal)))
            {
                SetTrackedProperty(ref _roles, "稳扎稳打", nameof(Roles));
            }

            _squadOptions = BuildSquadOptions();
            if (!_squadOptions.Any(option => string.Equals(option.Type, Squad, StringComparison.Ordinal)))
            {
                SetTrackedProperty(ref _squad, "指挥分队", nameof(Squad));
            }

            if (!_squadOptions.Any(option => string.Equals(option.Type, CollectibleModeSquad, StringComparison.Ordinal)))
            {
                SetTrackedProperty(ref _collectibleModeSquad, _squad, nameof(CollectibleModeSquad));
            }

            _findPlayTimeTargetOptions =
            [
                new IntOption(1, Texts.GetOrDefault("RoguelikePlaytimeLing", "令 - 掷地有声")),
                new IntOption(2, Texts.GetOrDefault("RoguelikePlaytimeShu", "黍 - 种因得果")),
                new IntOption(3, Texts.GetOrDefault("RoguelikePlaytimeNian", "年 - 三缺一")),
            ];
            if (_findPlayTimeTarget is < 1 or > 3)
            {
                SetTrackedProperty(ref _findPlayTimeTarget, 1, nameof(FindPlayTimeTarget));
            }

            _coreCharOptions = BuildCoreCharOptions();
            _coreCharNameOptions = _coreCharOptions
                .Select(option => option.DisplayName)
                .Distinct(StringComparer.Ordinal)
                .ToArray();
        }
        finally
        {
            _suppressOptionRefresh = false;
        }

        OnPropertyChanged(nameof(ThemeOptions));
        OnPropertyChanged(nameof(ModeOptions));
        OnPropertyChanged(nameof(DifficultyOptions));
        OnPropertyChanged(nameof(SquadOptions));
        OnPropertyChanged(nameof(RolesOptions));
        OnPropertyChanged(nameof(FindPlayTimeTargetOptions));
        OnPropertyChanged(nameof(CoreCharOptions));
        OnPropertyChanged(nameof(CoreCharNameOptions));
        OnPropertyChanged(nameof(SelectedThemeOption));
        OnPropertyChanged(nameof(SelectedModeOption));
        OnPropertyChanged(nameof(SelectedDifficultyOption));
        OnPropertyChanged(nameof(SelectedSquadOption));
        OnPropertyChanged(nameof(SelectedCollectibleModeSquadOption));
        OnPropertyChanged(nameof(SelectedRolesOption));
        OnPropertyChanged(nameof(SelectedFindPlayTimeTargetOption));
        OnPropertyChanged(nameof(SelectedCoreCharOption));

        RaiseComputedPropertyChanges();
    }

    private IReadOnlyList<IntOption> BuildModeOptions()
    {
        var result = new List<IntOption>();

        if (IsThemeJieGarden)
        {
            result.Add(new IntOption(ModeExp, Texts.GetOrDefault("RoguelikeStrategyExp", "刷等级，尽可能稳定地打更多层数")));
            result.Add(new IntOption(ModeInvestment, Texts.GetOrDefault("RoguelikeStrategyGold", "刷源石锭，投资完成后自动退出")));
            result.Add(new IntOption(ModeCollectible, Texts.GetOrDefault("RoguelikeStrategyLastReward", "刷开局，刷取热水壶或精二干员开局")));
            result.Add(new IntOption(ModeFindPlaytime, Texts.GetOrDefault("RoguelikeStrategyFindPlaytime", "刷常乐节点，第一层进洞，找不到需要的节点就重开")));
            return result;
        }

        result.Add(new IntOption(ModeExp, Texts.GetOrDefault("RoguelikeStrategyExp", "刷等级，尽可能稳定地打更多层数")));
        result.Add(new IntOption(ModeInvestment, Texts.GetOrDefault("RoguelikeStrategyGold", "刷源石锭，投资完成后自动退出")));
        result.Add(new IntOption(ModeCollectible, Texts.GetOrDefault("RoguelikeStrategyLastReward", "刷开局，刷取热水壶或精二干员开局")));
        result.Add(new IntOption(ModeSquad, Texts.GetOrDefault("RoguelikeStrategyMonthlySquad", "刷月度小队，尽可能稳定地打更多层数")));
        result.Add(new IntOption(ModeExploration, Texts.GetOrDefault("RoguelikeStrategyDeepExploration", "刷深入调查，尽可能稳定地打更多层数")));

        if (IsThemeSami)
        {
            result.Add(new IntOption(ModeCollapse, Texts.GetOrDefault("RoguelikeStrategyCollapse", "刷坍缩范式，遇到非稀有坍缩范式后直接重开")));
        }

        return result;
    }

    private IReadOnlyList<IntOption> BuildDifficultyOptions()
    {
        var maxDifficulty = GetMaxDifficultyForTheme(Theme);
        var list = new List<IntOption>(maxDifficulty + 3)
        {
            new IntOption(-1, $"{Texts.GetOrDefault("NotSwitch", "不切换")} (-1)"),
            new IntOption(int.MaxValue, $"MAX ({maxDifficulty})"),
        };

        for (var value = maxDifficulty; value >= 0; value--)
        {
            list.Add(new IntOption(value, value == 0 ? "MIN (0)" : value.ToString()));
        }

        return list;
    }

    private IReadOnlyList<TaskModuleOption> BuildRolesOptions()
    {
        var roles = new List<TaskModuleOption>
        {
            new("先手必胜", Texts.GetOrDefault("FirstMoveAdvantage", "先手必胜（先锋、狙击、特种）")),
            new("稳扎稳打", Texts.GetOrDefault("SlowAndSteadyWinsTheRace", "稳扎稳打（重装、术师、狙击）")),
            new("取长补短", Texts.GetOrDefault("OvercomingYourWeaknesses", "取长补短（近卫、辅助、医疗）")),
        };

        if (IsThemeJieGarden)
        {
            roles.Add(new TaskModuleOption("灵活部署", Texts.GetOrDefault("FlexibleDeployment", "灵活部署（先锋、辅助、特种）")));
            roles.Add(new TaskModuleOption("坚不可摧", Texts.GetOrDefault("Unbreakable", "坚不可摧（重装、术师、医疗）")));
        }

        roles.Add(new TaskModuleOption("随心所欲", Texts.GetOrDefault("AsYourHeartDesires", "随心所欲（三张随机）")));
        return roles;
    }

    private IReadOnlyList<TaskModuleOption> BuildSquadOptions()
    {
        var result = new List<TaskModuleOption>();

        var themeKey = $"{Theme}_{Mode}";
        if (!SquadDictionary.ContainsKey(themeKey))
        {
            themeKey = $"{Theme}_Default";
        }

        if (SquadDictionary.TryGetValue(themeKey, out var themedSquads))
        {
            foreach (var (textKey, value, fallback) in themedSquads)
            {
                result.Add(new TaskModuleOption(value, Texts.GetOrDefault(textKey, fallback)));
            }
        }

        foreach (var (textKey, value, fallback) in CommonSquads)
        {
            result.Add(new TaskModuleOption(value, Texts.GetOrDefault(textKey, fallback)));
        }

        return result;
    }

    private IReadOnlyList<TaskModuleOption> BuildCoreCharOptions()
    {
        var names = new List<string>();
        var clientType = ResolveCurrentClientType();
        var operNameLanguage = ResolveOperatorDisplayLanguage(clientType);
        var battleDataCache = GetBattleDataCache();
        var path = ResolveRoguelikeRecruitmentFilePath(Theme);
        if (path is not null)
        {
            try
            {
                var root = JsonNode.Parse(File.ReadAllText(path)) as JsonObject;
                if (root?[
                    "priority"] is JsonArray priorityArray)
                {
                    foreach (var priority in priorityArray)
                    {
                        if (priority is not JsonObject priorityObject
                            || priorityObject["opers"] is not JsonArray opersArray)
                        {
                            continue;
                        }

                        foreach (var oper in opersArray)
                        {
                            if (oper is not JsonObject operObject)
                            {
                                continue;
                            }

                            if (!(operObject["is_start"]?.GetValue<bool?>() ?? false))
                            {
                                continue;
                            }

                            var name = operObject["name"]?.GetValue<string?>()?.Trim();
                            if (!string.IsNullOrWhiteSpace(name))
                            {
                                var localizedName = ResolveLocalizedCoreCharName(
                                    name,
                                    clientType,
                                    operNameLanguage,
                                    battleDataCache);
                                if (!string.IsNullOrWhiteSpace(localizedName))
                                {
                                    names.Add(localizedName);
                                }
                            }
                        }
                    }
                }
            }
            catch
            {
                // Keep options empty when local resource file cannot be parsed.
            }
        }

        if (!string.IsNullOrWhiteSpace(CoreChar))
        {
            names.Add(CoreChar);
        }

        return names
            .Distinct(StringComparer.Ordinal)
            .Select(name => new TaskModuleOption(name, name))
            .ToArray();
    }

    private string ResolveCurrentClientType()
    {
        if (!Runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
        {
            return "Official";
        }

        if (!profile.Values.TryGetValue("ClientType", out var node) || !TryReadString(node, out var clientType))
        {
            return "Official";
        }

        return string.IsNullOrWhiteSpace(clientType) ? "Official" : clientType.Trim();
    }

    private string ResolveOperatorDisplayLanguage(string clientType)
    {
        var fallbackLanguage = UiLanguageCatalog.Normalize(Texts.Language);
        var globals = Runtime.ConfigurationService.CurrentConfig.GlobalValues;
        if (!globals.TryGetValue(ConfigurationKeys.OperNameLanguage, out var node)
            || !TryReadString(node, out var mode))
        {
            return fallbackLanguage;
        }

        var normalizedMode = mode.Trim();
        if (string.Equals(normalizedMode, OperNameLanguageClientMode, StringComparison.OrdinalIgnoreCase))
        {
            return ResolveClientLanguage(clientType);
        }

        if (normalizedMode.StartsWith(OperNameLanguageForcePrefix, StringComparison.OrdinalIgnoreCase))
        {
            var forcedLanguage = normalizedMode[OperNameLanguageForcePrefix.Length..].Trim();
            if (UiLanguageCatalog.IsSupported(forcedLanguage))
            {
                return UiLanguageCatalog.Normalize(forcedLanguage);
            }
        }

        return fallbackLanguage;
    }

    private static string ResolveClientLanguage(string clientType)
    {
        if (ClientTypeLanguageMap.TryGetValue(clientType.Trim(), out var language)
            && UiLanguageCatalog.IsSupported(language))
        {
            return UiLanguageCatalog.Normalize(language);
        }

        return UiLanguageCatalog.DefaultLanguage;
    }

    private static string? ResolveLocalizedCoreCharName(
        string rawName,
        string clientType,
        string language,
        BattleDataCache cache)
    {
        if (!cache.TryGetCharacterByAlias(rawName, out var character))
        {
            return null;
        }

        if (!IsCharacterAvailableInClient(character, clientType))
        {
            return null;
        }

        return GetLocalizedCharacterName(character, language);
    }

    private static string? ResolveRoguelikeRecruitmentFilePath(string theme)
    {
        var candidates = new[]
        {
            Path.Combine(AppContext.BaseDirectory, "resource", "roguelike", theme, "recruitment.json"),
            Path.Combine(Environment.CurrentDirectory, "resource", "roguelike", theme, "recruitment.json"),
        };

        foreach (var candidate in candidates)
        {
            if (File.Exists(candidate))
            {
                return candidate;
            }
        }

        return null;
    }

    private static string? ResolveBattleDataFilePath()
    {
        var candidates = new[]
        {
            Path.Combine(AppContext.BaseDirectory, "resource", "battle_data.json"),
            Path.Combine(Environment.CurrentDirectory, "resource", "battle_data.json"),
        };

        foreach (var candidate in candidates)
        {
            if (File.Exists(candidate))
            {
                return candidate;
            }
        }

        return null;
    }

    private static BattleDataCache GetBattleDataCache()
    {
        lock (BattleDataCacheLock)
        {
            _battleDataCache ??= LoadBattleDataCache();
            return _battleDataCache;
        }
    }

    private static BattleDataCache LoadBattleDataCache()
    {
        var path = ResolveBattleDataFilePath();
        if (path is null)
        {
            return BattleDataCache.Empty;
        }

        try
        {
            var root = JsonNode.Parse(File.ReadAllText(path)) as JsonObject;
            if (root?["chars"] is not JsonObject chars)
            {
                return BattleDataCache.Empty;
            }

            var aliasIndex = new Dictionary<string, BattleDataCharacter>(StringComparer.OrdinalIgnoreCase);
            foreach (var pair in chars)
            {
                if (pair.Value is not JsonObject characterNode)
                {
                    continue;
                }

                if (!TryReadString(characterNode["name"], out var name) || string.IsNullOrWhiteSpace(name))
                {
                    continue;
                }

                _ = TryReadString(characterNode["name_tw"], out var nameTw);
                _ = TryReadString(characterNode["name_en"], out var nameEn);
                _ = TryReadString(characterNode["name_jp"], out var nameJp);
                _ = TryReadString(characterNode["name_kr"], out var nameKr);

                _ = TryReadBool(characterNode["name_tw_unavailable"], out var nameTwUnavailable);
                _ = TryReadBool(characterNode["name_en_unavailable"], out var nameEnUnavailable);
                _ = TryReadBool(characterNode["name_jp_unavailable"], out var nameJpUnavailable);
                _ = TryReadBool(characterNode["name_kr_unavailable"], out var nameKrUnavailable);

                var character = new BattleDataCharacter(
                    Name: name.Trim(),
                    NameTw: string.IsNullOrWhiteSpace(nameTw) ? null : nameTw.Trim(),
                    NameEn: string.IsNullOrWhiteSpace(nameEn) ? null : nameEn.Trim(),
                    NameJp: string.IsNullOrWhiteSpace(nameJp) ? null : nameJp.Trim(),
                    NameKr: string.IsNullOrWhiteSpace(nameKr) ? null : nameKr.Trim(),
                    NameTwUnavailable: nameTwUnavailable,
                    NameEnUnavailable: nameEnUnavailable,
                    NameJpUnavailable: nameJpUnavailable,
                    NameKrUnavailable: nameKrUnavailable);

                AddAlias(aliasIndex, character.Name, character);
                AddAlias(aliasIndex, character.NameTw, character);
                AddAlias(aliasIndex, character.NameEn, character);
                AddAlias(aliasIndex, character.NameJp, character);
                AddAlias(aliasIndex, character.NameKr, character);
            }

            return new BattleDataCache(aliasIndex);
        }
        catch
        {
            return BattleDataCache.Empty;
        }
    }

    private static void AddAlias(
        IDictionary<string, BattleDataCharacter> aliasIndex,
        string? alias,
        BattleDataCharacter character)
    {
        if (string.IsNullOrWhiteSpace(alias))
        {
            return;
        }

        var normalized = alias.Trim();
        if (!aliasIndex.ContainsKey(normalized))
        {
            aliasIndex[normalized] = character;
        }
    }

    private static bool IsCharacterAvailableInClient(BattleDataCharacter character, string clientType)
    {
        var normalized = clientType.Trim();
        if (string.Equals(normalized, "zh-tw", StringComparison.OrdinalIgnoreCase)
            || string.Equals(normalized, "txwy", StringComparison.OrdinalIgnoreCase))
        {
            return !character.NameTwUnavailable;
        }

        if (string.Equals(normalized, "en-us", StringComparison.OrdinalIgnoreCase)
            || string.Equals(normalized, "YoStarEN", StringComparison.OrdinalIgnoreCase))
        {
            return !character.NameEnUnavailable;
        }

        if (string.Equals(normalized, "ja-jp", StringComparison.OrdinalIgnoreCase)
            || string.Equals(normalized, "YoStarJP", StringComparison.OrdinalIgnoreCase))
        {
            return !character.NameJpUnavailable;
        }

        if (string.Equals(normalized, "ko-kr", StringComparison.OrdinalIgnoreCase)
            || string.Equals(normalized, "YoStarKR", StringComparison.OrdinalIgnoreCase))
        {
            return !character.NameKrUnavailable;
        }

        return true;
    }

    private static string GetLocalizedCharacterName(BattleDataCharacter character, string language)
    {
        var normalized = UiLanguageCatalog.IsSupported(language)
            ? UiLanguageCatalog.Normalize(language)
            : UiLanguageCatalog.DefaultLanguage;

        return normalized switch
        {
            "zh-tw" => string.IsNullOrWhiteSpace(character.NameTw) ? character.Name : character.NameTw,
            "en-us" => string.IsNullOrWhiteSpace(character.NameEn) ? character.Name : character.NameEn,
            "ja-jp" => string.IsNullOrWhiteSpace(character.NameJp) ? character.Name : character.NameJp,
            "ko-kr" => string.IsNullOrWhiteSpace(character.NameKr) ? character.Name : character.NameKr,
            _ => character.Name,
        };
    }

    private static bool TryReadString(JsonNode? node, out string value)
    {
        value = string.Empty;
        if (node is not JsonValue jsonValue)
        {
            return false;
        }

        if (!jsonValue.TryGetValue(out string? parsed) || string.IsNullOrWhiteSpace(parsed))
        {
            return false;
        }

        value = parsed;
        return true;
    }

    private static bool TryReadBool(JsonNode? node, out bool value)
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

        if (jsonValue.TryGetValue(out int parsedInt))
        {
            value = parsedInt != 0;
            return true;
        }

        if (jsonValue.TryGetValue(out string? parsedText) && bool.TryParse(parsedText, out parsedBool))
        {
            value = parsedBool;
            return true;
        }

        return false;
    }

    private bool ResolveDelayAbortUntilCombatComplete()
    {
        if (Runtime.ConfigurationService.TryGetCurrentProfile(out var profile)
            && profile.Values.TryGetValue(DelayAbortUntilCombatCompleteConfigKey, out var profileNode)
            && TryReadBool(profileNode, out var profileValue))
        {
            return profileValue;
        }

        var globals = Runtime.ConfigurationService.CurrentConfig.GlobalValues;
        if (!globals.TryGetValue(DelayAbortUntilCombatCompleteConfigKey, out var node) || node is null)
        {
            return false;
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

        return false;
    }

    private async Task PersistDelayAbortUntilCombatCompleteAsync(bool value)
    {
        try
        {
            if (Runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
            {
                profile.Values[DelayAbortUntilCombatCompleteConfigKey] = JsonValue.Create(value);
            }
            else
            {
                Runtime.ConfigurationService.CurrentConfig.GlobalValues[DelayAbortUntilCombatCompleteConfigKey] = JsonValue.Create(value);
            }

            await Runtime.ConfigurationService.SaveAsync();
        }
        catch (Exception ex)
        {
            await Runtime.DiagnosticsService.RecordErrorAsync(
                "Roguelike.DelayAbortUntilCombatComplete.Save",
                "Failed to persist Roguelike delay-abort setting.",
                ex);
        }
    }

    private void ApplyPersistentDelayAbortSetting(bool value)
    {
        _suppressDelayAbortPersist = true;
        try
        {
            SetProperty(ref _delayAbortUntilCombatComplete, value, nameof(DelayAbortUntilCombatComplete));
        }
        finally
        {
            _suppressDelayAbortPersist = false;
        }
    }

    private void RaiseComputedPropertyChanges()
    {
        OnPropertyChanged(nameof(IsThemeJieGarden));
        OnPropertyChanged(nameof(IsThemePhantom));
        OnPropertyChanged(nameof(IsThemeMizuki));
        OnPropertyChanged(nameof(IsThemeSami));
        OnPropertyChanged(nameof(IsThemeSarkaz));
        OnPropertyChanged(nameof(IsSquadProfessional));
        OnPropertyChanged(nameof(IsSquadFoldartal));
        OnPropertyChanged(nameof(ShowCollectibleModeSquad));
        OnPropertyChanged(nameof(ShowFindPlayTimeTarget));
        OnPropertyChanged(nameof(ShowJieGardenDifficultyTip));
        OnPropertyChanged(nameof(CanToggleInvestmentEnabled));
        OnPropertyChanged(nameof(ShowStopWhenInvestmentFull));
        OnPropertyChanged(nameof(ShowInvestmentWithMoreScore));
        OnPropertyChanged(nameof(ShowCollectibleModeShopping));
        OnPropertyChanged(nameof(ShowInvestmentsCount));
        OnPropertyChanged(nameof(ShowRefreshTraderWithDice));
        OnPropertyChanged(nameof(ShowStartWithEliteTwo));
        OnPropertyChanged(nameof(EffectiveOnlyStartWithEliteTwo));
        OnPropertyChanged(nameof(ShowOnlyStartWithEliteTwoOption));
        OnPropertyChanged(nameof(ShowCollectibleStartRewardOptions));
        OnPropertyChanged(nameof(ShowCollectibleHotWater));
        OnPropertyChanged(nameof(ShowCollectibleShield));
        OnPropertyChanged(nameof(ShowCollectibleIngot));
        OnPropertyChanged(nameof(ShowCollectibleHope));
        OnPropertyChanged(nameof(ShowCollectibleRandom));
        OnPropertyChanged(nameof(ShowCollectibleKey));
        OnPropertyChanged(nameof(ShowCollectibleDice));
        OnPropertyChanged(nameof(ShowCollectibleIdeas));
        OnPropertyChanged(nameof(ShowCollectibleTicket));
        OnPropertyChanged(nameof(ShowFirstFloorFoldartalToggle));
        OnPropertyChanged(nameof(ShowFirstFloorFoldartalText));
        OnPropertyChanged(nameof(ShowStartFoldartalListToggle));
        OnPropertyChanged(nameof(ShowStartFoldartalListText));
        OnPropertyChanged(nameof(ShowExpectedCollapsalParadigms));
        OnPropertyChanged(nameof(CanUseSupport));
        OnPropertyChanged(nameof(ShowUseNonfriendSupport));
        OnPropertyChanged(nameof(ShowStopAtFinalBoss));
        OnPropertyChanged(nameof(ShowMonthlySquadAutoIterate));
        OnPropertyChanged(nameof(ShowMonthlySquadCheckComms));
        OnPropertyChanged(nameof(ShowDeepExplorationAutoIterate));
        OnPropertyChanged(nameof(ShowStopAtMaxLevel));
        OnPropertyChanged(nameof(ShowStartWithSeedToggle));
        OnPropertyChanged(nameof(ShowStartWithSeedInput));
        OnPropertyChanged(nameof(CollectibleStartRewardSummary));
    }

    private void SetCollectibleRewardFlag(ref bool field, bool value)
    {
        if (!SetTrackedProperty(ref field, value))
        {
            return;
        }

        OnPropertyChanged(nameof(CollectibleStartRewardSummary));
    }

    private string BuildCollectibleStartRewardSummary()
    {
        var selected = new List<string>();
        AppendCollectibleSummaryToken(selected, ShowCollectibleHotWater && CollectibleHotWater, "Roguelike.Collectible.HotWater", "Hot water");
        AppendCollectibleSummaryToken(selected, ShowCollectibleShield && CollectibleShield, "Roguelike.Collectible.Shield", "Shield");
        AppendCollectibleSummaryToken(selected, ShowCollectibleIngot && CollectibleIngot, "Roguelike.Collectible.Ingot", "Ingot");
        AppendCollectibleSummaryToken(selected, ShowCollectibleHope && CollectibleHope, "Roguelike.Collectible.Hope", "Hope");
        AppendCollectibleSummaryToken(selected, ShowCollectibleRandom && CollectibleRandom, "Roguelike.Collectible.Random", "Random");
        AppendCollectibleSummaryToken(selected, ShowCollectibleKey && CollectibleKey, "Roguelike.Collectible.Key", "Key");
        AppendCollectibleSummaryToken(selected, ShowCollectibleDice && CollectibleDice, "Roguelike.Collectible.Dice", "Dice");
        AppendCollectibleSummaryToken(selected, ShowCollectibleIdeas && CollectibleIdeas, "Roguelike.Collectible.Ideas", "Ideas");
        AppendCollectibleSummaryToken(selected, ShowCollectibleTicket && CollectibleTicket, "Roguelike.Collectible.Ticket", "Ticket");

        if (selected.Count == 0)
        {
            return Texts.GetOrDefault("Roguelike.StartWithSelectList", "Expected rewards in collectible start mode");
        }

        return string.Join(" / ", selected);
    }

    private void AppendCollectibleSummaryToken(List<string> selected, bool enabled, string textKey, string fallback)
    {
        if (!enabled)
        {
            return;
        }

        selected.Add(Texts.GetOrDefault(textKey, fallback));
    }

    private static int GetMaxDifficultyForTheme(string theme)
    {
        return theme switch
        {
            "Phantom" => 15,
            "Mizuki" => 18,
            "Sami" => 15,
            "Sarkaz" => 18,
            "JieGarden" => 18,
            _ => 18,
        };
    }

    private static List<string> ParseDelimitedLines(string value)
    {
        return value
            .Split(new[] { '\r', '\n', ';', ',', '|' }, StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
            .Where(v => !string.IsNullOrWhiteSpace(v))
            .Distinct(StringComparer.Ordinal)
            .ToList();
    }

    private static bool ContainsStructuredMarkers(string value)
    {
        return !string.IsNullOrWhiteSpace(value) && value.IndexOfAny(['[', ']', '{', '}', ':', '"']) >= 0;
    }

    private static TaskModuleOption? ResolveSelectedOption(
        IReadOnlyList<TaskModuleOption> options,
        string? value)
    {
        var normalized = value?.Trim() ?? string.Empty;
        if (string.IsNullOrEmpty(normalized))
        {
            return options.FirstOrDefault();
        }

        foreach (var option in options)
        {
            if (string.Equals(option.Type, normalized, StringComparison.OrdinalIgnoreCase))
            {
                return option;
            }
        }

        return new TaskModuleOption(normalized, normalized);
    }

    private static IntOption? ResolveSelectedOption(
        IReadOnlyList<IntOption> options,
        int value)
    {
        foreach (var option in options)
        {
            if (option.Value == value)
            {
                return option;
            }
        }

        return options.FirstOrDefault();
    }

    private sealed record BattleDataCharacter(
        string Name,
        string? NameTw,
        string? NameEn,
        string? NameJp,
        string? NameKr,
        bool NameTwUnavailable,
        bool NameEnUnavailable,
        bool NameJpUnavailable,
        bool NameKrUnavailable);

    private sealed class BattleDataCache
    {
        public static readonly BattleDataCache Empty = new(new Dictionary<string, BattleDataCharacter>(StringComparer.OrdinalIgnoreCase));

        public BattleDataCache(IReadOnlyDictionary<string, BattleDataCharacter> aliasIndex)
        {
            AliasIndex = aliasIndex;
        }

        public IReadOnlyDictionary<string, BattleDataCharacter> AliasIndex { get; }

        public bool TryGetCharacterByAlias(string alias, out BattleDataCharacter character)
        {
            if (AliasIndex.TryGetValue(alias, out var matched))
            {
                character = matched;
                return true;
            }

            character = null!;
            return false;
        }
    }
}
