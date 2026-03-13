using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Text.Json.Nodes;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Localization;
using MAAUnified.Application.Services.TaskParams;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class RecruitTaskModuleViewModel : TypedTaskModuleViewModelBase<RecruitTaskParamsDto>
{
    private const int MinRecruitTime = 60;
    private const int MaxRecruitTime = 540;

    private static readonly string[] AutoRecruitTagKeys =
    [
        "近战位",
        "远程位",
        "先锋干员",
        "近卫干员",
        "狙击干员",
        "重装干员",
        "医疗干员",
        "辅助干员",
        "术师干员",
        "治疗",
        "费用回复",
        "输出",
        "生存",
        "群攻",
        "防护",
        "减速",
    ];

    private static readonly IReadOnlyDictionary<string, string> DisplayLanguageClientDirectoryMap =
        new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["zh-tw"] = "txwy",
            ["en-us"] = "YoStarEN",
            ["ja-jp"] = "YoStarJP",
            ["ko-kr"] = "YoStarKR",
        };

    private int _times = 4;
    private bool _refresh = true;
    private bool _forceRefresh = true;
    private bool _useExpedited;
    private bool _skipRobot = true;
    private int _extraTagsMode;
    private bool _chooseLevel3 = true;
    private bool _chooseLevel4 = true;
    private bool _chooseLevel5;
    private int _level3Time = 540;
    private int _level4Time = 540;
    private int _level5Time = 540;
    private IReadOnlyList<ExtraTagsModeOption> _extraTagsModeOptions = [];
    private bool _suppressTagSelectionChanged;

    public RecruitTaskModuleViewModel(MAAUnifiedRuntime runtime, LocalizedTextMap texts)
        : base(runtime, texts, "TaskQueue.Recruit")
    {
        Texts.PropertyChanged += OnTextsChanged;
        Runtime.ConfigurationService.ConfigChanged += _ => RebuildRecruitTagOptions();
        RebuildExtraTagsModeOptions();
        RebuildRecruitTagOptions();
    }

    public ObservableCollection<RecruitTagOption> FirstTagOptions { get; } = [];

    public string FirstTagsSummary => BuildFirstTagsSummary();

    public int Times
    {
        get => _times;
        set => SetTrackedProperty(ref _times, Math.Max(0, value));
    }

    public bool Refresh
    {
        get => _refresh;
        set
        {
            if (!SetTrackedProperty(ref _refresh, value))
            {
                return;
            }

            if (!_refresh)
            {
                ForceRefresh = false;
            }
        }
    }

    public bool ForceRefresh
    {
        get => _forceRefresh;
        set => SetTrackedProperty(ref _forceRefresh, Refresh && value);
    }

    public bool UseExpedited
    {
        get => _useExpedited;
        set => SetTrackedProperty(ref _useExpedited, value);
    }

    public bool SkipRobot
    {
        get => _skipRobot;
        set => SetTrackedProperty(ref _skipRobot, value);
    }

    public bool NotChooseLevel1
    {
        get => SkipRobot;
        set => SkipRobot = value;
    }

    public int ExtraTagsMode
    {
        get => _extraTagsMode;
        set
        {
            var normalized = Math.Clamp(value, 0, 2);
            if (!SetTrackedProperty(ref _extraTagsMode, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(SelectedExtraTagsModeOption));
        }
    }

    public IReadOnlyList<ExtraTagsModeOption> ExtraTagsModeOptions => _extraTagsModeOptions;

    public ExtraTagsModeOption? SelectedExtraTagsModeOption
    {
        get => ExtraTagsModeOptions.FirstOrDefault(option => option.Value == ExtraTagsMode);
        set => ExtraTagsMode = value?.Value ?? 0;
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

    public int Level3Time
    {
        get => _level3Time;
        set
        {
            if (!SetTrackedProperty(ref _level3Time, NormalizeRecruitTime(value)))
            {
                return;
            }

            OnPropertyChanged(nameof(Level3Hour));
            OnPropertyChanged(nameof(Level3Minute));
        }
    }

    public int Level4Time
    {
        get => _level4Time;
        set
        {
            if (!SetTrackedProperty(ref _level4Time, NormalizeRecruitTime(value)))
            {
                return;
            }

            OnPropertyChanged(nameof(Level4Hour));
            OnPropertyChanged(nameof(Level4Minute));
        }
    }

    public int Level5Time
    {
        get => _level5Time;
        set
        {
            if (!SetTrackedProperty(ref _level5Time, NormalizeRecruitTime(value)))
            {
                return;
            }

            OnPropertyChanged(nameof(Level5Hour));
            OnPropertyChanged(nameof(Level5Minute));
        }
    }

    public int Level3Hour
    {
        get => Level3Time / 60;
        set => Level3Time = (value * 60) + Level3Minute;
    }

    public int Level3Minute
    {
        get => (Level3Time % 60) / 10 * 10;
        set => Level3Time = (Level3Hour * 60) + value;
    }

    public int Level4Hour
    {
        get => Level4Time / 60;
        set => Level4Time = (value * 60) + Level4Minute;
    }

    public int Level4Minute
    {
        get => (Level4Time % 60) / 10 * 10;
        set => Level4Time = (Level4Hour * 60) + value;
    }

    public int Level5Hour
    {
        get => Level5Time / 60;
        set => Level5Time = (value * 60) + Level5Minute;
    }

    public int Level5Minute
    {
        get => (Level5Time % 60) / 10 * 10;
        set => Level5Time = (Level5Hour * 60) + value;
    }

    protected override Task<UiOperationResult<RecruitTaskParamsDto>> LoadDtoAsync(int index, CancellationToken cancellationToken)
    {
        return Runtime.TaskQueueFeatureService.GetRecruitParamsAsync(index, cancellationToken);
    }

    protected override Task<UiOperationResult> SaveDtoAsync(int index, RecruitTaskParamsDto dto, CancellationToken cancellationToken)
    {
        return Runtime.TaskQueueFeatureService.SaveRecruitParamsAsync(index, dto, cancellationToken);
    }

    protected override TaskCompileOutput CompileDto(RecruitTaskParamsDto dto, UnifiedProfile profile, UnifiedConfig config)
    {
        return TaskParamCompiler.CompileRecruit(dto, profile, config);
    }

    protected override void ApplyDto(RecruitTaskParamsDto dto)
    {
        Times = dto.Times;
        Refresh = dto.Refresh;
        ForceRefresh = dto.ForceRefresh;
        UseExpedited = dto.UseExpedited;
        SkipRobot = dto.SkipRobot;
        ExtraTagsMode = dto.ExtraTagsMode;
        ChooseLevel3 = dto.ChooseLevel3;
        ChooseLevel4 = dto.ChooseLevel4;
        ChooseLevel5 = dto.ChooseLevel5;
        Level3Time = dto.Level3Time;
        Level4Time = dto.Level4Time;
        Level5Time = dto.Level5Time;
        ApplyFirstTags(dto.FirstTags);
    }

    protected override RecruitTaskParamsDto BuildDto()
    {
        var firstTags = FirstTagOptions
            .Where(option => option.IsSelected)
            .Select(option => option.Value)
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToList();

        return new RecruitTaskParamsDto
        {
            Times = Math.Max(0, Times),
            Refresh = Refresh,
            ForceRefresh = Refresh && ForceRefresh,
            UseExpedited = UseExpedited,
            SkipRobot = SkipRobot,
            ExtraTagsMode = ExtraTagsMode,
            FirstTags = firstTags,
            ChooseLevel3 = ChooseLevel3,
            ChooseLevel4 = ChooseLevel4,
            ChooseLevel5 = ChooseLevel5,
            Level3Time = Level3Time,
            Level4Time = Level4Time,
            Level5Time = Level5Time,
            SetTime = true,
        };
    }

    private void OnTextsChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName is not (nameof(LocalizedTextMap.Language) or "Item[]"))
        {
            return;
        }

        RebuildExtraTagsModeOptions();
        RebuildRecruitTagOptions();
    }

    private void RebuildExtraTagsModeOptions()
    {
        _extraTagsModeOptions =
        [
            new ExtraTagsModeOption(
                0,
                Texts.GetOrDefault("Recruit.Option.ExtraTags.Default", "Default no extra tags")),
            new ExtraTagsModeOption(
                1,
                Texts.GetOrDefault("Recruit.Option.ExtraTags.SelectAll", "Select extra tags")),
            new ExtraTagsModeOption(
                2,
                Texts.GetOrDefault("Recruit.Option.ExtraTags.RareOnly", "Select extra tags: rare only")),
        ];

        OnPropertyChanged(nameof(ExtraTagsModeOptions));
        OnPropertyChanged(nameof(SelectedExtraTagsModeOption));
    }

    private void RebuildRecruitTagOptions()
    {
        var selected = FirstTagOptions
            .Where(option => option.IsSelected)
            .Select(option => option.Value)
            .ToHashSet(StringComparer.OrdinalIgnoreCase);

        var recruitTags = ResolveRecruitTags();

        foreach (var option in FirstTagOptions)
        {
            option.PropertyChanged -= OnRecruitTagOptionChanged;
        }

        FirstTagOptions.Clear();
        foreach (var (display, value) in recruitTags)
        {
            var option = new RecruitTagOption(display, value)
            {
                IsSelected = selected.Contains(value),
            };
            option.PropertyChanged += OnRecruitTagOptionChanged;
            FirstTagOptions.Add(option);
        }

        OnPropertyChanged(nameof(FirstTagsSummary));
    }

    private void ApplyFirstTags(IReadOnlyList<string> tags)
    {
        var selected = tags.ToHashSet(StringComparer.OrdinalIgnoreCase);
        _suppressTagSelectionChanged = true;
        try
        {
            foreach (var option in FirstTagOptions)
            {
                option.IsSelected = selected.Contains(option.Value);
            }
        }
        finally
        {
            _suppressTagSelectionChanged = false;
        }

        OnPropertyChanged(nameof(FirstTagsSummary));
    }

    private void OnRecruitTagOptionChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (_suppressTagSelectionChanged || IsApplyingDto || e.PropertyName != nameof(RecruitTagOption.IsSelected))
        {
            return;
        }

        OnPropertyChanged(nameof(FirstTagsSummary));
        MarkDirty();
    }

    private string BuildFirstTagsSummary()
    {
        var selected = FirstTagOptions
            .Where(option => option.IsSelected)
            .Select(option => option.DisplayName)
            .Where(name => !string.IsNullOrWhiteSpace(name))
            .ToList();

        if (selected.Count == 0)
        {
            return Texts.GetOrDefault("Recruit.FirstTags", "First tags");
        }

        return string.Join(" / ", selected);
    }

    private IReadOnlyList<(string Display, string Value)> ResolveRecruitTags()
    {
        var clientType = ResolveCurrentClientType();
        var displayLanguage = UiLanguageCatalog.Normalize(Texts.Language);
        var clientTags = ParseRecruitTags(ResolveRecruitmentPathByClientType(clientType));
        var displayTags = ParseRecruitTags(ResolveRecruitmentPathByDisplayLanguage(displayLanguage));

        var list = new List<(string Display, string Value)>();
        foreach (var key in AutoRecruitTagKeys)
        {
            if (!clientTags.TryGetValue(key, out var clientValue) || string.IsNullOrWhiteSpace(clientValue))
            {
                continue;
            }

            var display = displayTags.TryGetValue(key, out var displayValue) && !string.IsNullOrWhiteSpace(displayValue)
                ? displayValue
                : clientValue;
            list.Add((display, clientValue));
        }

        return list;
    }

    private string ResolveCurrentClientType()
    {
        if (!Runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
        {
            return "Official";
        }

        if (!profile.Values.TryGetValue("ClientType", out var node) || node is null)
        {
            return "Official";
        }

        return TryReadString(node, out var clientType) && !string.IsNullOrWhiteSpace(clientType)
            ? clientType
            : "Official";
    }

    private static string ResolveRecruitmentPathByClientType(string clientType)
    {
        var baseResourceDirectory = Path.Combine(AppContext.BaseDirectory, "resource");
        if (string.IsNullOrWhiteSpace(clientType)
            || string.Equals(clientType, "Official", StringComparison.OrdinalIgnoreCase)
            || string.Equals(clientType, "Bilibili", StringComparison.OrdinalIgnoreCase))
        {
            return Path.Combine(baseResourceDirectory, "recruitment.json");
        }

        return Path.Combine(baseResourceDirectory, "global", clientType, "resource", "recruitment.json");
    }

    private static string ResolveRecruitmentPathByDisplayLanguage(string language)
    {
        if (DisplayLanguageClientDirectoryMap.TryGetValue(language, out var clientDirectory))
        {
            return Path.Combine(AppContext.BaseDirectory, "resource", "global", clientDirectory, "resource", "recruitment.json");
        }

        return Path.Combine(AppContext.BaseDirectory, "resource", "recruitment.json");
    }

    private static Dictionary<string, string> ParseRecruitTags(string path)
    {
        if (!File.Exists(path))
        {
            return new Dictionary<string, string>(StringComparer.Ordinal);
        }

        try
        {
            var root = JsonNode.Parse(File.ReadAllText(path)) as JsonObject;
            if (root?["tags"] is not JsonObject tags)
            {
                return new Dictionary<string, string>(StringComparer.Ordinal);
            }

            var result = new Dictionary<string, string>(StringComparer.Ordinal);
            foreach (var pair in tags)
            {
                if (pair.Value is not JsonValue tagNode || !tagNode.TryGetValue(out string? value) || string.IsNullOrWhiteSpace(value))
                {
                    continue;
                }

                result[pair.Key] = value;
            }

            return result;
        }
        catch
        {
            return new Dictionary<string, string>(StringComparer.Ordinal);
        }
    }

    private static bool TryReadString(JsonNode node, out string value)
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

    private static int NormalizeRecruitTime(int value)
    {
        return value switch
        {
            < MinRecruitTime => MaxRecruitTime,
            > MaxRecruitTime => MinRecruitTime,
            _ => value / 10 * 10,
        };
    }

    public sealed record ExtraTagsModeOption(int Value, string DisplayName);

    public sealed class RecruitTagOption : ObservableObject
    {
        private bool _isSelected;

        public RecruitTagOption(string displayName, string value)
        {
            DisplayName = displayName;
            Value = value;
        }

        public string DisplayName { get; }

        public string Value { get; }

        public bool IsSelected
        {
            get => _isSelected;
            set => SetProperty(ref _isSelected, value);
        }
    }
}
