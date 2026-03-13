using System.ComponentModel;
using System.Text.Json;
using System.Text.Json.Nodes;
using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Localization;
using MAAUnified.Application.Services.TaskParams;
using LegacyConfigurationKeys = MAAUnified.Compat.Constants.ConfigurationKeys;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class FightTaskModuleViewModel : TypedTaskModuleViewModelBase<FightTaskParamsDto>
{
    private static readonly HashSet<string> ExcludedDropIds =
    [
        "3213", "3223", "3233", "3243",
        "3253", "3263", "3273", "3283",
        "7001", "7002", "7003", "7004",
        "4004", "4005",
        "3105", "3131", "3132", "3133",
        "6001",
        "3141", "4002",
        "32001",
        "30115",
        "30125",
        "30135",
        "30145",
        "30155",
        "30165",
    ];

    private static readonly IReadOnlyDictionary<string, string> DisplayLanguageClientDirectoryMap =
        new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["zh-tw"] = "txwy",
            ["en-us"] = "YoStarEN",
            ["ja-jp"] = "YoStarJP",
            ["ko-kr"] = "YoStarKR",
        };

    private static readonly IReadOnlyDictionary<string, IReadOnlySet<DayOfWeek>> WeeklyOpenStageCodes =
        new Dictionary<string, IReadOnlySet<DayOfWeek>>(StringComparer.OrdinalIgnoreCase)
        {
            ["CE-6"] = new HashSet<DayOfWeek> { DayOfWeek.Tuesday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday },
            ["AP-5"] = new HashSet<DayOfWeek> { DayOfWeek.Monday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday },
            ["CA-5"] = new HashSet<DayOfWeek> { DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Friday, DayOfWeek.Sunday },
            ["SK-5"] = new HashSet<DayOfWeek> { DayOfWeek.Monday, DayOfWeek.Wednesday, DayOfWeek.Friday, DayOfWeek.Saturday },
            ["PR-A-1"] = new HashSet<DayOfWeek> { DayOfWeek.Monday, DayOfWeek.Thursday, DayOfWeek.Friday, DayOfWeek.Sunday },
            ["PR-A-2"] = new HashSet<DayOfWeek> { DayOfWeek.Monday, DayOfWeek.Thursday, DayOfWeek.Friday, DayOfWeek.Sunday },
            ["PR-B-1"] = new HashSet<DayOfWeek> { DayOfWeek.Monday, DayOfWeek.Tuesday, DayOfWeek.Friday, DayOfWeek.Saturday },
            ["PR-B-2"] = new HashSet<DayOfWeek> { DayOfWeek.Monday, DayOfWeek.Tuesday, DayOfWeek.Friday, DayOfWeek.Saturday },
            ["PR-C-1"] = new HashSet<DayOfWeek> { DayOfWeek.Wednesday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday },
            ["PR-C-2"] = new HashSet<DayOfWeek> { DayOfWeek.Wednesday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday },
            ["PR-D-1"] = new HashSet<DayOfWeek> { DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Saturday, DayOfWeek.Sunday },
            ["PR-D-2"] = new HashSet<DayOfWeek> { DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Saturday, DayOfWeek.Sunday },
        };

    private static readonly IReadOnlyList<string> FallbackStageCodes =
    [
        "1-7",
        "R8-11",
        "12-17-HARD",
        "CE-6",
        "AP-5",
        "CA-5",
        "LS-6",
        "SK-5",
        "Annihilation",
        "PR-A-1",
        "PR-A-2",
        "PR-B-1",
        "PR-B-2",
        "PR-C-1",
        "PR-C-2",
        "PR-D-1",
        "PR-D-2",
    ];

    private static readonly Dictionary<string, IReadOnlyList<string>> StageCodeCache =
        new(StringComparer.OrdinalIgnoreCase);

    private static readonly IReadOnlyList<(string StageCode, string ZhTip, string EnTip, IReadOnlyList<string[]>? InventoryGroups)> DailyHintSpecs =
    [
        ("CE-6", "CE-6: 龙门币", "CE-6: LMD", null),
        ("AP-5", "AP-5: 红票", "AP-5: Purchase Certificate", null),
        ("CA-5", "CA-5: 技能", "CA-5: Skill Summary", null),
        ("LS-6", "LS-6: 经验", "LS-6: Battle Record", null),
        ("SK-5", "SK-5: 碳", "SK-5: Carbon", null),
        ("PR-A-1", "PR-A-1/2: 奶&盾芯片", "PR-A-1/2: Med&Def Chip", [["3231", "3261"], ["3232", "3262"]]),
        ("PR-B-1", "PR-B-1/2: 术&狙芯片", "PR-B-1/2: Cst&Sni Chip", [["3251", "3241"], ["3252", "3242"]]),
        ("PR-C-1", "PR-C-1/2: 先&辅芯片", "PR-C-1/2: Pio&Sup Chip", [["3211", "3271"], ["3212", "3272"]]),
        ("PR-D-1", "PR-D-1/2: 近&特芯片", "PR-D-1/2: Grd&Spc Chip", [["3221", "3281"], ["3222", "3282"]]),
    ];

    private string _stage = FightStageSelection.CurrentOrLast;
    private bool _useMedicine;
    private int _medicine;
    private bool _useStone;
    private int _stone;
    private bool _enableTimesLimit;
    private int _times = int.MaxValue;
    private int _series = 1;
    private bool _isDrGrandet;
    private bool _useExpiringMedicine;
    private bool _enableTargetDrop;
    private string _dropId = string.Empty;
    private int _dropCount = 1;
    private bool _useCustomAnnihilation;
    private string _annihilationStage = "Annihilation";
    private bool _useAlternateStage;
    private bool _hideUnavailableStage = true;
    private string _stageResetMode = "Current";
    private bool _hideSeries;
    private bool _allowUseStoneSave;
    private IReadOnlyList<IntOption> _seriesOptions = [];
    private IReadOnlyList<StringOption> _stageResetModeOptions = [];
    private IReadOnlyList<StringOption> _annihilationStageOptions = [];
    private IReadOnlyList<DropOption> _dropOptions = [];
    private IReadOnlyList<StageOption> _stageOptions = [];

    public FightTaskModuleViewModel(MAAUnifiedRuntime runtime, LocalizedTextMap texts)
        : base(runtime, texts, "TaskQueue.Fight")
    {
        Texts.PropertyChanged += OnTextsChanged;
        RebuildLocalizedOptions();
        RebuildDropOptions();
        RebuildStageOptions();
    }

    public IReadOnlyList<IntOption> SeriesOptions => _seriesOptions;

    public IntOption? SelectedSeriesOption
    {
        get => SeriesOptions.FirstOrDefault(option => option.Value == Series);
        set => Series = value?.Value ?? 1;
    }

    public IReadOnlyList<StringOption> StageResetModeOptions => _stageResetModeOptions;

    public StringOption? SelectedStageResetModeOption
    {
        get => StageResetModeOptions.FirstOrDefault(
            option => string.Equals(option.Value, StageResetMode, StringComparison.OrdinalIgnoreCase));
        set => StageResetMode = value?.Value ?? "Current";
    }

    public IReadOnlyList<StringOption> AnnihilationStageOptions => _annihilationStageOptions;

    public StringOption? SelectedAnnihilationStageOption
    {
        get => AnnihilationStageOptions.FirstOrDefault(
            option => string.Equals(option.Value, AnnihilationStage, StringComparison.OrdinalIgnoreCase));
        set => AnnihilationStage = value?.Value ?? "Annihilation";
    }

    public IReadOnlyList<StageOption> StageOptions => _stageOptions;

    public StageOption? SelectedStageOption
    {
        get => StageOptions.FirstOrDefault(
            option => string.Equals(option.Value, Stage, StringComparison.OrdinalIgnoreCase));
        set
        {
            if (value is null)
            {
                return;
            }

            Stage = value.Value;
        }
    }

    public IReadOnlyList<DropOption> DropOptions => _dropOptions;

    public DropOption? SelectedDropOption
    {
        get => DropOptions.FirstOrDefault(option => string.Equals(option.Value, DropId, StringComparison.Ordinal));
        set => DropId = value?.Value ?? string.Empty;
    }

    public bool IsMedicineInputEnabled => UseMedicine && !UseStone;

    public bool ShowSeriesSetting => !HideSeries;

    public bool IsDropControlsEnabled => EnableTargetDrop;

    public string UseStoneDisplayName =>
        $"{Texts.GetOrDefault("Fight.UseStoneDisplay", Texts.GetOrDefault("Fight.UseStone", "Use stone"))}{(AllowUseStoneSave ? string.Empty : "*")}";

    public string Stage
    {
        get => _stage;
        set
        {
            var normalized = FightStageSelection.NormalizeStoredValue(value);
            if (!SetTrackedProperty(ref _stage, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(SelectedStageOption));
        }
    }

    public bool UseMedicine
    {
        get => _useMedicine;
        set
        {
            if (!SetTrackedProperty(ref _useMedicine, value))
            {
                return;
            }

            if (!value && UseStone)
            {
                UseStone = false;
            }

            OnPropertyChanged(nameof(IsMedicineInputEnabled));
        }
    }

    public int Medicine
    {
        get => _medicine;
        set => SetTrackedProperty(ref _medicine, Math.Max(0, value));
    }

    public bool UseStone
    {
        get => _useStone;
        set
        {
            if (!SetTrackedProperty(ref _useStone, value))
            {
                return;
            }

            if (value)
            {
                if (!UseMedicine)
                {
                    UseMedicine = true;
                }

                if (Medicine < 999)
                {
                    Medicine = 999;
                }
            }

            OnPropertyChanged(nameof(IsMedicineInputEnabled));
        }
    }

    public int Stone
    {
        get => _stone;
        set => SetTrackedProperty(ref _stone, Math.Max(0, value));
    }

    public bool EnableTimesLimit
    {
        get => _enableTimesLimit;
        set => SetTrackedProperty(ref _enableTimesLimit, value);
    }

    public int Times
    {
        get => _times;
        set => SetTrackedProperty(ref _times, Math.Max(0, value));
    }

    public int Series
    {
        get => _series;
        set
        {
            var normalized = Math.Clamp(value, -1, 6);
            if (!SetTrackedProperty(ref _series, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(SelectedSeriesOption));
        }
    }

    public bool IsDrGrandet
    {
        get => _isDrGrandet;
        set => SetTrackedProperty(ref _isDrGrandet, value);
    }

    public bool UseExpiringMedicine
    {
        get => _useExpiringMedicine;
        set => SetTrackedProperty(ref _useExpiringMedicine, value);
    }

    public bool EnableTargetDrop
    {
        get => _enableTargetDrop;
        set
        {
            if (!SetTrackedProperty(ref _enableTargetDrop, value))
            {
                return;
            }

            OnPropertyChanged(nameof(IsDropControlsEnabled));
        }
    }

    public string DropId
    {
        get => _dropId;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (!SetTrackedProperty(ref _dropId, normalized))
            {
                return;
            }

            NormalizeDropSelectionToKnownOption();
            OnPropertyChanged(nameof(SelectedDropOption));
        }
    }

    public int DropCount
    {
        get => _dropCount;
        set => SetTrackedProperty(ref _dropCount, Math.Max(1, value));
    }

    public bool UseCustomAnnihilation
    {
        get => _useCustomAnnihilation;
        set
        {
            if (!SetTrackedProperty(ref _useCustomAnnihilation, value))
            {
                return;
            }

            RebuildStageOptions();
        }
    }

    public string AnnihilationStage
    {
        get => _annihilationStage;
        set
        {
            var normalized = string.IsNullOrWhiteSpace(value) ? "Annihilation" : value.Trim();
            if (!SetTrackedProperty(ref _annihilationStage, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(SelectedAnnihilationStageOption));
            RebuildStageOptions();
        }
    }

    public bool UseAlternateStage
    {
        get => _useAlternateStage;
        set
        {
            if (!SetTrackedProperty(ref _useAlternateStage, value))
            {
                return;
            }

            if (value)
            {
                if (HideUnavailableStage)
                {
                    HideUnavailableStage = false;
                }

                if (!string.Equals(StageResetMode, "Ignore", StringComparison.OrdinalIgnoreCase))
                {
                    StageResetMode = "Ignore";
                }
            }
        }
    }

    public bool HideUnavailableStage
    {
        get => _hideUnavailableStage;
        set
        {
            if (!SetTrackedProperty(ref _hideUnavailableStage, value))
            {
                return;
            }

            if (value)
            {
                if (UseAlternateStage)
                {
                    UseAlternateStage = false;
                }

                if (!string.Equals(StageResetMode, "Current", StringComparison.OrdinalIgnoreCase))
                {
                    StageResetMode = "Current";
                }
            }

            RebuildStageOptions();
        }
    }

    public string StageResetMode
    {
        get => _stageResetMode;
        set
        {
            var normalized = NormalizeStageResetMode(value);
            if (!SetTrackedProperty(ref _stageResetMode, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(SelectedStageResetModeOption));
        }
    }

    public bool HideSeries
    {
        get => _hideSeries;
        set
        {
            if (!SetTrackedProperty(ref _hideSeries, value))
            {
                return;
            }

            OnPropertyChanged(nameof(ShowSeriesSetting));
        }
    }

    public bool AllowUseStoneSave
    {
        get => _allowUseStoneSave;
        set
        {
            if (!SetTrackedProperty(ref _allowUseStoneSave, value))
            {
                return;
            }

            OnPropertyChanged(nameof(UseStoneDisplayName));
        }
    }

    protected override Task<UiOperationResult<FightTaskParamsDto>> LoadDtoAsync(int index, CancellationToken cancellationToken)
    {
        return Runtime.TaskQueueFeatureService.GetFightParamsAsync(index, cancellationToken);
    }

    protected override Task<UiOperationResult> SaveDtoAsync(int index, FightTaskParamsDto dto, CancellationToken cancellationToken)
    {
        return Runtime.TaskQueueFeatureService.SaveFightParamsAsync(index, dto, cancellationToken);
    }

    protected override TaskCompileOutput CompileDto(FightTaskParamsDto dto, UnifiedProfile profile, UnifiedConfig config)
    {
        return TaskParamCompiler.CompileFight(dto, profile, config);
    }

    protected override void ApplyDto(FightTaskParamsDto dto)
    {
        Stage = dto.Stage;
        UseMedicine = dto.UseMedicine;
        Medicine = dto.Medicine;
        UseStone = dto.UseStone;
        Stone = dto.Stone;
        EnableTimesLimit = dto.EnableTimesLimit;
        Times = dto.Times;
        Series = dto.Series;
        IsDrGrandet = dto.IsDrGrandet;
        UseExpiringMedicine = dto.UseExpiringMedicine;
        EnableTargetDrop = dto.EnableTargetDrop;
        DropId = dto.DropId;
        DropCount = dto.DropCount;
        UseCustomAnnihilation = dto.UseCustomAnnihilation;
        AnnihilationStage = dto.AnnihilationStage;
        UseAlternateStage = dto.UseAlternateStage;
        HideUnavailableStage = dto.HideUnavailableStage;
        StageResetMode = dto.StageResetMode;
        HideSeries = dto.HideSeries;
        AllowUseStoneSave = dto.AllowUseStoneSave;
        NormalizeDropSelectionToKnownOption();
        RebuildStageOptions();
    }

    protected override FightTaskParamsDto BuildDto()
    {
        return new FightTaskParamsDto
        {
            Stage = FightStageSelection.NormalizeStoredValue(Stage),
            UseMedicine = UseMedicine,
            Medicine = Math.Max(0, Medicine),
            UseStone = UseStone,
            Stone = Math.Max(0, Stone),
            EnableTimesLimit = EnableTimesLimit,
            Times = EnableTimesLimit ? Math.Max(0, Times) : int.MaxValue,
            Series = Math.Clamp(Series, -1, 6),
            IsDrGrandet = IsDrGrandet,
            UseExpiringMedicine = UseExpiringMedicine,
            EnableTargetDrop = EnableTargetDrop,
            DropId = DropId.Trim(),
            DropCount = Math.Max(1, DropCount),
            UseCustomAnnihilation = UseCustomAnnihilation,
            AnnihilationStage = string.IsNullOrWhiteSpace(AnnihilationStage) ? "Annihilation" : AnnihilationStage.Trim(),
            UseAlternateStage = UseAlternateStage,
            HideUnavailableStage = HideUnavailableStage,
            StageResetMode = NormalizeStageResetMode(StageResetMode),
            HideSeries = HideSeries,
            AllowUseStoneSave = AllowUseStoneSave,
        };
    }

    private void OnTextsChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName is not (nameof(LocalizedTextMap.Language) or "Item[]"))
        {
            return;
        }

        RebuildLocalizedOptions();
        RebuildDropOptions();
        RebuildStageOptions();
    }

    private void RebuildLocalizedOptions()
    {
        _seriesOptions =
        [
            new IntOption(0, "AUTO"),
            new IntOption(6, "6"),
            new IntOption(5, "5"),
            new IntOption(4, "4"),
            new IntOption(3, "3"),
            new IntOption(2, "2"),
            new IntOption(1, "1"),
            new IntOption(-1, Texts.GetOrDefault("Fight.NotSwitch", "Don't Switch")),
        ];
        _stageResetModeOptions =
        [
            new StringOption("Current", Texts.GetOrDefault("Fight.StageReset.Current", Texts.GetOrDefault("Fight.DefaultStage", "Cur/Last"))),
            new StringOption("Ignore", Texts.GetOrDefault("Fight.StageReset.Ignore", Texts.GetOrDefault("Fight.NotSwitch", "Don't Switch"))),
        ];
        _annihilationStageOptions =
        [
            new StringOption("Annihilation", Texts.GetOrDefault("Fight.Annihilation.Current", "Current Annihilation")),
            new StringOption("Chernobog@Annihilation", Texts.GetOrDefault("Fight.Annihilation.Chernobog", "Chernobog")),
            new StringOption("LungmenOutskirts@Annihilation", Texts.GetOrDefault("Fight.Annihilation.LungmenOutskirts", "Lungmen Outskirts")),
            new StringOption("LungmenDowntown@Annihilation", Texts.GetOrDefault("Fight.Annihilation.LungmenDowntown", "Lungmen Downtown")),
        ];

        OnPropertyChanged(nameof(SeriesOptions));
        OnPropertyChanged(nameof(SelectedSeriesOption));
        OnPropertyChanged(nameof(StageResetModeOptions));
        OnPropertyChanged(nameof(SelectedStageResetModeOption));
        OnPropertyChanged(nameof(AnnihilationStageOptions));
        OnPropertyChanged(nameof(SelectedAnnihilationStageOption));
        OnPropertyChanged(nameof(UseStoneDisplayName));
    }

    private void RebuildDropOptions()
    {
        var list = new List<DropOption>
        {
            new(Texts.GetOrDefault("Fight.Drop.NotSelected", "Not selected"), string.Empty),
        };

        var path = ResolveItemIndexPathByDisplayLanguage(UiLanguageCatalog.Normalize(Texts.Language));
        if (File.Exists(path))
        {
            try
            {
                var root = JsonNode.Parse(File.ReadAllText(path)) as JsonObject;
                if (root is not null)
                {
                    foreach (var pair in root)
                    {
                        if (!int.TryParse(pair.Key, out _))
                        {
                            continue;
                        }

                        if (ExcludedDropIds.Contains(pair.Key))
                        {
                            continue;
                        }

                        if (pair.Value is not JsonObject itemObj
                            || !TryReadString(itemObj["name"], out var name)
                            || string.IsNullOrWhiteSpace(name))
                        {
                            continue;
                        }

                        list.Add(new DropOption(name, pair.Key));
                    }
                }
            }
            catch
            {
                // ignore malformed resource file and keep fallback option.
            }
        }

        list = list
            .GroupBy(option => option.Value, StringComparer.Ordinal)
            .Select(group => group.First())
            .OrderBy(option => option.Value, StringComparer.Ordinal)
            .ToList();

        _dropOptions = list;
        NormalizeDropSelectionToKnownOption();
        OnPropertyChanged(nameof(DropOptions));
        OnPropertyChanged(nameof(SelectedDropOption));
    }

    private void NormalizeDropSelectionToKnownOption()
    {
        if (string.IsNullOrWhiteSpace(DropId))
        {
            return;
        }

        if (_dropOptions.Any(option => string.Equals(option.Value, DropId, StringComparison.Ordinal)))
        {
            return;
        }

        SetTrackedProperty(ref _dropId, string.Empty, nameof(DropId));
    }

    private static string ResolveItemIndexPathByDisplayLanguage(string language)
    {
        if (DisplayLanguageClientDirectoryMap.TryGetValue(language, out var clientDirectory))
        {
            return Path.Combine(AppContext.BaseDirectory, "resource", "global", clientDirectory, "resource", "item_index.json");
        }

        return Path.Combine(AppContext.BaseDirectory, "resource", "item_index.json");
    }

    private static bool TryReadString(JsonNode? node, out string value)
    {
        value = string.Empty;
        if (node is not JsonValue jsonValue || !jsonValue.TryGetValue(out string? parsed) || string.IsNullOrWhiteSpace(parsed))
        {
            return false;
        }

        value = parsed;
        return true;
    }

    public void RefreshStageOptions(string? clientType = null, bool forceReload = false)
    {
        RebuildStageOptions(clientType, forceReload);
    }

    public static string BuildDailyResourceHint(
        string language,
        string? clientType,
        UnifiedConfig? config = null,
        DateTime? nowUtc = null)
    {
        var zhCn = IsChineseLanguage(language);
        var normalizedClientType = NormalizeClientType(clientType);
        var timestamp = nowUtc ?? DateTime.UtcNow;
        var dayOfWeek = MallDailyResetHelper.GetYjDate(timestamp, normalizedClientType).DayOfWeek;
        var depotCounts = ReadDepotCounts(config);
        var lines = new List<string>();
        foreach (var spec in DailyHintSpecs)
        {
            if (!IsStageOpen(spec.StageCode, dayOfWeek))
            {
                continue;
            }

            lines.Add(zhCn ? spec.ZhTip : spec.EnTip);
            if (spec.InventoryGroups is null)
            {
                continue;
            }

            var inventoryHint = BuildInventoryHint(spec.InventoryGroups, depotCounts, zhCn);
            if (!string.IsNullOrWhiteSpace(inventoryHint))
            {
                lines.Add(inventoryHint);
            }
        }

        if (lines.Count == 0)
        {
            lines.Add(zhCn
                ? "今日关卡信息将在资源加载后展示。"
                : "Daily stage hints will be shown after resources are loaded.");
        }

        return string.Join(Environment.NewLine, lines);
    }

    private static string? BuildInventoryHint(
        IReadOnlyList<string[]> inventoryGroups,
        IReadOnlyDictionary<string, int> depotCounts,
        bool zhCn)
    {
        var groupTexts = new List<string>(inventoryGroups.Count);
        foreach (var group in inventoryGroups)
        {
            var itemCounts = new List<string>(group.Length);
            foreach (var itemId in group)
            {
                if (!depotCounts.TryGetValue(itemId, out var count) || count < 0)
                {
                    return null;
                }

                itemCounts.Add(count.ToString());
            }

            groupTexts.Add(string.Join(" & ", itemCounts));
        }

        var prefix = zhCn ? "(库存 " : "(Inventory ";
        return $"{prefix}{string.Join(" / ", groupTexts)})";
    }

    private static IReadOnlyDictionary<string, int> ReadDepotCounts(UnifiedConfig? config)
    {
        var result = new Dictionary<string, int>(StringComparer.Ordinal);
        if (config is null
            || !config.GlobalValues.TryGetValue(LegacyConfigurationKeys.DepotResult, out JsonNode? node)
            || node is null)
        {
            return result;
        }

        var raw = ExtractRawDepotPayload(node);
        if (string.IsNullOrWhiteSpace(raw))
        {
            return result;
        }

        TryReadDepotCountsFromPayload(raw, result);
        return result;
    }

    private static string ExtractRawDepotPayload(JsonNode node)
    {
        if (node is JsonValue value && value.TryGetValue(out string? text))
        {
            return text ?? string.Empty;
        }

        return node.ToJsonString();
    }

    private static void TryReadDepotCountsFromPayload(string payload, IDictionary<string, int> counts)
    {
        JsonNode? parsed;
        try
        {
            parsed = JsonNode.Parse(payload);
        }
        catch
        {
            return;
        }

        if (parsed is not JsonObject root)
        {
            return;
        }

        ReadFlatDepotCountMap(root, counts);

        if (root["data"] is JsonObject dataObject)
        {
            ReadFlatDepotCountMap(dataObject, counts);
        }
        else if (root["data"] is JsonValue dataValue && dataValue.TryGetValue(out string? dataText))
        {
            TryReadDepotCountsFromPayload(dataText ?? string.Empty, counts);
        }

        if (root["items"] is JsonArray itemsArray)
        {
            ReadDepotItemsArray(itemsArray, counts);
        }

        if (root["arkplanner"]?["object"]?["items"] is JsonArray arkPlannerItems)
        {
            ReadDepotItemsArray(arkPlannerItems, counts);
        }
    }

    private static void ReadFlatDepotCountMap(JsonObject source, IDictionary<string, int> counts)
    {
        foreach (var pair in source)
        {
            if (!int.TryParse(pair.Key, out _))
            {
                continue;
            }

            if (TryReadDepotCount(pair.Value, out var count))
            {
                counts[pair.Key] = count;
            }
        }
    }

    private static void ReadDepotItemsArray(JsonArray items, IDictionary<string, int> counts)
    {
        foreach (var node in items)
        {
            if (node is not JsonObject item
                || !TryReadString(item["id"], out var id)
                || string.IsNullOrWhiteSpace(id))
            {
                continue;
            }

            if (TryReadDepotCount(item["have"], out var have))
            {
                counts[id] = have;
                continue;
            }

            if (TryReadDepotCount(item["count"], out var count))
            {
                counts[id] = count;
            }
        }
    }

    private static bool TryReadDepotCount(JsonNode? node, out int count)
    {
        count = 0;
        if (node is JsonValue value)
        {
            if (value.TryGetValue(out int intValue))
            {
                count = intValue;
                return true;
            }

            if (value.TryGetValue(out string? text) && int.TryParse(text, out intValue))
            {
                count = intValue;
                return true;
            }
        }

        return false;
    }

    private void RebuildStageOptions(string? clientTypeOverride = null, bool forceReload = false)
    {
        var normalizedClientType = NormalizeClientType(clientTypeOverride ?? ResolveClientTypeFromConfig());
        var dayOfWeek = MallDailyResetHelper.GetYjDate(DateTime.UtcNow, normalizedClientType).DayOfWeek;
        var stageResourcePath = ResolveStageResourcePath(normalizedClientType);
        var stageCodes = LoadStageCodes(stageResourcePath, forceReload);
        var defaultStageDisplay = Texts.GetOrDefault("Fight.DefaultStage", "Cur/Last");
        var annihilationDisplay = UseCustomAnnihilation
            ? AnnihilationStageOptions.FirstOrDefault(
                  option => string.Equals(option.Value, AnnihilationStage, StringComparison.OrdinalIgnoreCase))
                  ?.DisplayName ?? Texts.GetOrDefault("Fight.Annihilation.Current", "Current Annihilation")
            : Texts.GetOrDefault("Fight.Annihilation.Current", "Current Annihilation");
        var list = new List<StageOption>
        {
            new(defaultStageDisplay, FightStageSelection.CurrentOrLast, IsOpen: true, IsOutdated: false),
        };

        foreach (var stageCode in stageCodes)
        {
            var normalizedStage = stageCode.Trim();
            if (string.IsNullOrWhiteSpace(normalizedStage))
            {
                continue;
            }

            var isOpen = IsStageOpen(normalizedStage, dayOfWeek);
            var isCurrentStage = string.Equals(
                normalizedStage,
                Stage?.Trim(),
                StringComparison.OrdinalIgnoreCase);
            if (HideUnavailableStage && !isOpen && !isCurrentStage)
            {
                continue;
            }

            var display = string.Equals(normalizedStage, "Annihilation", StringComparison.OrdinalIgnoreCase)
                ? annihilationDisplay
                : !isOpen && !isCurrentStage
                    ? $"{normalizedStage} {BuildClosedSuffix()}"
                    : normalizedStage;
            list.Add(new StageOption(display, normalizedStage, isOpen, IsOutdated: false));
        }

        var currentStage = FightStageSelection.NormalizeStoredValue(Stage);
        if (!FightStageSelection.IsCurrentOrLast(currentStage)
            && !list.Any(option => string.Equals(option.Value, currentStage, StringComparison.OrdinalIgnoreCase)))
        {
            list.Add(new StageOption(
                $"{currentStage} {BuildOutdatedSuffix()}",
                currentStage,
                IsOpen: false,
                IsOutdated: true));
        }

        _stageOptions = list;
        OnPropertyChanged(nameof(StageOptions));
        OnPropertyChanged(nameof(SelectedStageOption));
    }

    private string ResolveClientTypeFromConfig()
    {
        if (Runtime.ConfigurationService.TryGetCurrentProfile(out var profile)
            && profile.Values.TryGetValue("ClientType", out var node)
            && node is JsonValue jsonValue
            && jsonValue.TryGetValue(out string? configuredClientType)
            && !string.IsNullOrWhiteSpace(configuredClientType))
        {
            return configuredClientType;
        }

        return "Official";
    }

    private static string ResolveStageResourcePath(string normalizedClientType)
    {
        var officialPath = Path.Combine(AppContext.BaseDirectory, "resource", "stages.json");
        if (string.Equals(normalizedClientType, "Official", StringComparison.OrdinalIgnoreCase))
        {
            return officialPath;
        }

        var globalPath = Path.Combine(
            AppContext.BaseDirectory,
            "resource",
            "global",
            normalizedClientType,
            "resource",
            "stages.json");
        return File.Exists(globalPath) ? globalPath : officialPath;
    }

    private static IReadOnlyList<string> LoadStageCodes(string stageResourcePath, bool forceReload)
    {
        lock (StageCodeCache)
        {
            if (forceReload)
            {
                StageCodeCache.Remove(stageResourcePath);
            }
            else if (StageCodeCache.TryGetValue(stageResourcePath, out var cachedCodes))
            {
                return cachedCodes;
            }
        }

        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var orderedCodes = new List<string>();

        static void AddStageCode(HashSet<string> seenSet, List<string> ordered, string? stageCode)
        {
            if (string.IsNullOrWhiteSpace(stageCode))
            {
                return;
            }

            var normalized = stageCode.Trim();
            if (seenSet.Add(normalized))
            {
                ordered.Add(normalized);
            }
        }

        foreach (var fallbackCode in FallbackStageCodes)
        {
            AddStageCode(seen, orderedCodes, fallbackCode);
        }

        if (!File.Exists(stageResourcePath))
        {
            var fallbackOnly = orderedCodes.ToArray();
            lock (StageCodeCache)
            {
                StageCodeCache[stageResourcePath] = fallbackOnly;
            }

            return fallbackOnly;
        }

        try
        {
            using var stream = File.OpenRead(stageResourcePath);
            using var document = JsonDocument.Parse(stream);
            if (document.RootElement.ValueKind == JsonValueKind.Array)
            {
                foreach (var element in document.RootElement.EnumerateArray())
                {
                    if (!element.TryGetProperty("code", out var codeElement))
                    {
                        continue;
                    }

                    var code = codeElement.GetString();
                    if (string.IsNullOrWhiteSpace(code))
                    {
                        continue;
                    }

                    AddStageCode(seen, orderedCodes, code);
                }
            }
        }
        catch
        {
            // Ignore malformed resource files and fall back to static stage list.
        }

        var loadedCodes = orderedCodes.ToArray();
        lock (StageCodeCache)
        {
            StageCodeCache[stageResourcePath] = loadedCodes;
        }

        return loadedCodes;
    }

    private static string NormalizeClientType(string? clientType)
    {
        if (string.IsNullOrWhiteSpace(clientType)
            || string.Equals(clientType, "Official", StringComparison.OrdinalIgnoreCase)
            || string.Equals(clientType, "Bilibili", StringComparison.OrdinalIgnoreCase))
        {
            return "Official";
        }

        return clientType.Trim();
    }

    private static bool IsStageOpen(string stageCode, DayOfWeek dayOfWeek)
    {
        if (!WeeklyOpenStageCodes.TryGetValue(stageCode, out var openDays))
        {
            return true;
        }

        return openDays.Contains(dayOfWeek);
    }

    private string BuildClosedSuffix()
    {
        return IsChineseLanguage(Texts.Language)
            ? "(未开放)"
            : "(Closed)";
    }

    private string BuildOutdatedSuffix()
    {
        return IsChineseLanguage(Texts.Language)
            ? "(已过期)"
            : "(Outdated)";
    }

    private static IReadOnlyList<string> GetResourceStageCodesForDay(DayOfWeek dayOfWeek)
    {
        var list = new List<string> { "LS-6" };
        foreach (var pair in WeeklyOpenStageCodes)
        {
            if (!pair.Value.Contains(dayOfWeek))
            {
                continue;
            }

            list.Add(pair.Key);
        }

        return list.OrderBy(stageCode => stageCode, StringComparer.OrdinalIgnoreCase).ToArray();
    }

    private static bool IsChineseLanguage(string? language)
    {
        return !string.IsNullOrWhiteSpace(language)
            && language.StartsWith("zh", StringComparison.OrdinalIgnoreCase);
    }

    private static string NormalizeStageResetMode(string? value)
    {
        return string.Equals(value, "Ignore", StringComparison.OrdinalIgnoreCase)
            ? "Ignore"
            : "Current";
    }

    public sealed record IntOption(int Value, string DisplayName);

    public sealed record StringOption(string Value, string DisplayName);

    public sealed record DropOption(string DisplayName, string Value);

    public sealed record StageOption(string DisplayName, string Value, bool IsOpen, bool IsOutdated)
    {
        public bool IsClosed => !IsOpen;
    }
}
