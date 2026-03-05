using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.TaskParams;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class RoguelikeModuleViewModel : TypedTaskModuleViewModelBase<RoguelikeTaskParamsDto>
{
    private int _mode;
    private string _theme = "JieGarden";
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
    private string _firstFloorFoldartal = string.Empty;
    private string _startFoldartalListText = string.Empty;
    private string _expectedCollapsalParadigmsText = string.Empty;
    private string _startWithSeed = string.Empty;

    public RoguelikeModuleViewModel(MAAUnifiedRuntime runtime, LocalizedTextMap texts)
        : base(runtime, texts, "TaskQueue.Roguelike")
    {
    }

    public IReadOnlyList<string> ThemeOptions { get; } = ["JieGarden", "Phantom", "Mizuki", "Sami", "Sarkaz"];

    public IReadOnlyList<int> ModeOptions { get; } = [0, 1, 4, 5, 6, 7, 20001];

    public int Mode
    {
        get => _mode;
        set => SetTrackedProperty(ref _mode, value);
    }

    public string Theme
    {
        get => _theme;
        set => SetTrackedProperty(ref _theme, value);
    }

    public int Difficulty
    {
        get => _difficulty;
        set => SetTrackedProperty(ref _difficulty, value);
    }

    public int StartsCount
    {
        get => _startsCount;
        set => SetTrackedProperty(ref _startsCount, Math.Max(0, value));
    }

    public bool InvestmentEnabled
    {
        get => _investmentEnabled;
        set => SetTrackedProperty(ref _investmentEnabled, value);
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
        set => SetTrackedProperty(ref _squad, value);
    }

    public string Roles
    {
        get => _roles;
        set => SetTrackedProperty(ref _roles, value);
    }

    public string CoreChar
    {
        get => _coreChar;
        set => SetTrackedProperty(ref _coreChar, value);
    }

    public bool UseSupport
    {
        get => _useSupport;
        set => SetTrackedProperty(ref _useSupport, value);
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
        set => SetTrackedProperty(ref _collectibleModeSquad, value);
    }

    public bool StartWithEliteTwo
    {
        get => _startWithEliteTwo;
        set => SetTrackedProperty(ref _startWithEliteTwo, value);
    }

    public bool OnlyStartWithEliteTwo
    {
        get => _onlyStartWithEliteTwo;
        set => SetTrackedProperty(ref _onlyStartWithEliteTwo, value);
    }

    public bool CollectibleHotWater
    {
        get => _collectibleHotWater;
        set => SetTrackedProperty(ref _collectibleHotWater, value);
    }

    public bool CollectibleShield
    {
        get => _collectibleShield;
        set => SetTrackedProperty(ref _collectibleShield, value);
    }

    public bool CollectibleIngot
    {
        get => _collectibleIngot;
        set => SetTrackedProperty(ref _collectibleIngot, value);
    }

    public bool CollectibleHope
    {
        get => _collectibleHope;
        set => SetTrackedProperty(ref _collectibleHope, value);
    }

    public bool CollectibleRandom
    {
        get => _collectibleRandom;
        set => SetTrackedProperty(ref _collectibleRandom, value);
    }

    public bool CollectibleKey
    {
        get => _collectibleKey;
        set => SetTrackedProperty(ref _collectibleKey, value);
    }

    public bool CollectibleDice
    {
        get => _collectibleDice;
        set => SetTrackedProperty(ref _collectibleDice, value);
    }

    public bool CollectibleIdeas
    {
        get => _collectibleIdeas;
        set => SetTrackedProperty(ref _collectibleIdeas, value);
    }

    public bool CollectibleTicket
    {
        get => _collectibleTicket;
        set => SetTrackedProperty(ref _collectibleTicket, value);
    }

    public bool MonthlySquadAutoIterate
    {
        get => _monthlySquadAutoIterate;
        set => SetTrackedProperty(ref _monthlySquadAutoIterate, value);
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
        set => SetTrackedProperty(ref _findPlayTimeTarget, Math.Max(1, value));
    }

    public string FirstFloorFoldartal
    {
        get => _firstFloorFoldartal;
        set => SetTrackedProperty(ref _firstFloorFoldartal, value);
    }

    public string StartFoldartalListText
    {
        get => _startFoldartalListText;
        set => SetTrackedProperty(ref _startFoldartalListText, value);
    }

    public string ExpectedCollapsalParadigmsText
    {
        get => _expectedCollapsalParadigmsText;
        set => SetTrackedProperty(ref _expectedCollapsalParadigmsText, value);
    }

    public string StartWithSeed
    {
        get => _startWithSeed;
        set => SetTrackedProperty(ref _startWithSeed, value);
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
        Mode = dto.Mode;
        Theme = dto.Theme;
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
        FirstFloorFoldartal = dto.FirstFloorFoldartal;
        StartFoldartalListText = string.Join(Environment.NewLine, dto.StartFoldartalList);
        ExpectedCollapsalParadigmsText = string.Join(Environment.NewLine, dto.ExpectedCollapsalParadigms);
        StartWithSeed = dto.StartWithSeed;
    }

    protected override RoguelikeTaskParamsDto BuildDto()
    {
        return new RoguelikeTaskParamsDto
        {
            Mode = Mode,
            Theme = string.IsNullOrWhiteSpace(Theme) ? "JieGarden" : Theme.Trim(),
            Difficulty = Difficulty,
            StartsCount = Math.Max(0, StartsCount),
            InvestmentEnabled = InvestmentEnabled,
            InvestmentWithMoreScore = InvestmentWithMoreScore,
            InvestmentsCount = Math.Max(0, InvestmentsCount),
            StopWhenInvestmentFull = StopWhenInvestmentFull,
            Squad = Squad.Trim(),
            Roles = Roles.Trim(),
            CoreChar = CoreChar.Trim(),
            UseSupport = UseSupport,
            UseNonfriendSupport = UseNonfriendSupport,
            RefreshTraderWithDice = RefreshTraderWithDice,
            StopAtFinalBoss = StopAtFinalBoss,
            StopAtMaxLevel = StopAtMaxLevel,
            CollectibleModeShopping = CollectibleModeShopping,
            CollectibleModeSquad = CollectibleModeSquad.Trim(),
            StartWithEliteTwo = StartWithEliteTwo,
            OnlyStartWithEliteTwo = OnlyStartWithEliteTwo,
            CollectibleModeStartList = new RoguelikeCollectibleStartListDto
            {
                HotWater = CollectibleHotWater,
                Shield = CollectibleShield,
                Ingot = CollectibleIngot,
                Hope = CollectibleHope,
                Random = CollectibleRandom,
                Key = CollectibleKey,
                Dice = CollectibleDice,
                Ideas = CollectibleIdeas,
                Ticket = CollectibleTicket,
            },
            MonthlySquadAutoIterate = MonthlySquadAutoIterate,
            MonthlySquadCheckComms = MonthlySquadCheckComms,
            DeepExplorationAutoIterate = DeepExplorationAutoIterate,
            FindPlayTimeTarget = Math.Max(1, FindPlayTimeTarget),
            FirstFloorFoldartal = FirstFloorFoldartal.Trim(),
            StartFoldartalList = ParseTextLines(StartFoldartalListText),
            ExpectedCollapsalParadigms = ParseTextLines(ExpectedCollapsalParadigmsText),
            StartWithSeed = StartWithSeed.Trim(),
        };
    }

    protected override IReadOnlyList<TaskValidationIssue> ValidateBeforeSave()
    {
        var issues = new List<TaskValidationIssue>();
        if (ContainsStructuredMarkers(StartFoldartalListText))
        {
            issues.Add(new TaskValidationIssue(
                "DelimitedInputParseFailed",
                "roguelike.start_foldartal_list",
                "Start foldartal list only supports plain delimiter-separated text."));
        }

        if (ContainsStructuredMarkers(ExpectedCollapsalParadigmsText))
        {
            issues.Add(new TaskValidationIssue(
                "DelimitedInputParseFailed",
                "roguelike.expected_collapsal_paradigms",
                "Expected collapsal paradigms only supports plain delimiter-separated text."));
        }

        return issues;
    }

    private static List<string> ParseTextLines(string value)
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
}
