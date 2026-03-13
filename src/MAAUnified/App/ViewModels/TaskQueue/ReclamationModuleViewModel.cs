using System.ComponentModel;
using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.TaskParams;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class ReclamationModuleViewModel : TypedTaskModuleViewModelBase<ReclamationTaskParamsDto>
{
    private static readonly (string Value, string TextKey, string Fallback)[] ThemeOptionSpecs =
    [
        ("Fire", "Reclamation.Option.Theme.FireClosed", "沙中之火（活动未开放）"),
        ("Tales", "Reclamation.Option.Theme.Tales", "沙洲遗闻"),
    ];

    private static readonly (int Value, string TextKey, string Fallback)[] ModeOptionSpecs =
    [
        (0, "Reclamation.Option.Mode.NoArchive", "无存档，通过进出关卡刷生息点数"),
        (1, "Reclamation.Option.Mode.Archive", "有存档，通过组装支援道具刷生息点数"),
    ];

    private static readonly (int Value, string TextKey, string Fallback)[] IncrementModeOptionSpecs =
    [
        (0, "Reclamation.Option.IncrementMode.Click", "连点"),
        (1, "Reclamation.Option.IncrementMode.Hold", "长按"),
    ];

    private string _theme = "Tales";
    private int _mode = 1;
    private int _incrementMode;
    private int _numCraftBatches = 16;
    private string _toolsToCraftText = string.Empty;
    private bool _clearStore = true;
    private IReadOnlyList<TaskModuleOption> _themeOptions = [];
    private IReadOnlyList<IntOption> _modeOptions = [];
    private IReadOnlyList<IntOption> _incrementModeOptions = [];

    public ReclamationModuleViewModel(MAAUnifiedRuntime runtime, LocalizedTextMap texts)
        : base(runtime, texts, "TaskQueue.Reclamation")
    {
        Texts.PropertyChanged += OnTextsPropertyChanged;
        RebuildOptions();
    }

    public sealed record IntOption(int Value, string DisplayName);

    public IReadOnlyList<TaskModuleOption> ThemeOptions => _themeOptions;

    public IReadOnlyList<IntOption> ModeOptions => _modeOptions;

    public IReadOnlyList<IntOption> IncrementModeOptions => _incrementModeOptions;

    public TaskModuleOption? SelectedThemeOption
    {
        get => ResolveSelectedOption(ThemeOptions, Theme);
        set => Theme = value?.Type ?? "Tales";
    }

    public IntOption? SelectedModeOption
    {
        get => ResolveSelectedOption(ModeOptions, Mode);
        set => Mode = value?.Value ?? 1;
    }

    public IntOption? SelectedIncrementModeOption
    {
        get => ResolveSelectedOption(IncrementModeOptions, IncrementMode);
        set => IncrementMode = value?.Value ?? 0;
    }

    public bool IsArchiveMode => Mode == 1;

    public bool IsArchiveSettingsEnabled => IsArchiveMode;

    public bool ShowClearStore => Mode == 0;

    public string Theme
    {
        get => _theme;
        set
        {
            var normalized = string.IsNullOrWhiteSpace(value) ? "Tales" : value.Trim();
            if (!SetTrackedProperty(ref _theme, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(SelectedThemeOption));
        }
    }

    public int Mode
    {
        get => _mode;
        set
        {
            var normalized = value is 0 or 1 ? value : 1;
            if (!SetTrackedProperty(ref _mode, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(SelectedModeOption));
            OnPropertyChanged(nameof(IsArchiveMode));
            OnPropertyChanged(nameof(IsArchiveSettingsEnabled));
            OnPropertyChanged(nameof(ShowClearStore));
        }
    }

    public int IncrementMode
    {
        get => _incrementMode;
        set
        {
            var normalized = value is 0 or 1 ? value : 0;
            if (!SetTrackedProperty(ref _incrementMode, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(SelectedIncrementModeOption));
        }
    }

    public int NumCraftBatches
    {
        get => _numCraftBatches;
        set => SetTrackedProperty(ref _numCraftBatches, Math.Clamp(value, 0, 99999));
    }

    public string ToolsToCraftText
    {
        get => _toolsToCraftText;
        set
        {
            var normalized = (value ?? string.Empty)
                .Replace('；', ';')
                .Trim();
            SetTrackedProperty(ref _toolsToCraftText, normalized);
        }
    }

    public bool ClearStore
    {
        get => _clearStore;
        set => SetTrackedProperty(ref _clearStore, value);
    }

    protected override Task<UiOperationResult<ReclamationTaskParamsDto>> LoadDtoAsync(int index, CancellationToken cancellationToken)
    {
        return Runtime.TaskQueueFeatureService.GetReclamationParamsAsync(index, cancellationToken);
    }

    protected override Task<UiOperationResult> SaveDtoAsync(int index, ReclamationTaskParamsDto dto, CancellationToken cancellationToken)
    {
        return Runtime.TaskQueueFeatureService.SaveReclamationParamsAsync(index, dto, cancellationToken);
    }

    protected override TaskCompileOutput CompileDto(ReclamationTaskParamsDto dto, UnifiedProfile profile, UnifiedConfig config)
    {
        return TaskParamCompiler.CompileReclamation(dto, profile, config);
    }

    protected override void ApplyDto(ReclamationTaskParamsDto dto)
    {
        Theme = dto.Theme;
        Mode = dto.Mode;
        IncrementMode = dto.IncrementMode;
        NumCraftBatches = dto.NumCraftBatches;
        ToolsToCraftText = string.Join(";", dto.ToolsToCraft);
        ClearStore = dto.ClearStore;
    }

    protected override ReclamationTaskParamsDto BuildDto()
    {
        var toolsToCraft = ParseTextLines(ToolsToCraftText);
        if (Mode == 1 && toolsToCraft.Count == 0)
        {
            toolsToCraft = [Texts.GetOrDefault("Reclamation.ToolToCraftPlaceholder", "荧光棒")];
        }

        return new ReclamationTaskParamsDto
        {
            Theme = string.IsNullOrWhiteSpace(Theme) ? "Tales" : Theme.Trim(),
            Mode = Mode,
            IncrementMode = IncrementMode,
            NumCraftBatches = Math.Clamp(NumCraftBatches, 0, 99999),
            ToolsToCraft = toolsToCraft,
            ClearStore = ClearStore,
        };
    }

    protected override IReadOnlyList<TaskValidationIssue> ValidateBeforeSave()
    {
        if (!ContainsStructuredMarkers(ToolsToCraftText))
        {
            return [];
        }

        return
        [
            new TaskValidationIssue(
                "DelimitedInputParseFailed",
                "reclamation.tools_to_craft",
                "ToolsToCraft only supports plain delimiter-separated text."),
        ];
    }

    private void OnTextsPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (!string.IsNullOrEmpty(e.PropertyName)
            && !string.Equals(e.PropertyName, nameof(LocalizedTextMap.Language), StringComparison.Ordinal)
            && !string.Equals(e.PropertyName, "Item[]", StringComparison.Ordinal))
        {
            return;
        }

        RebuildOptions();
    }

    private void RebuildOptions()
    {
        _themeOptions = ThemeOptionSpecs
            .Select(spec => new TaskModuleOption(spec.Value, Texts.GetOrDefault(spec.TextKey, spec.Fallback)))
            .ToArray();
        _modeOptions = ModeOptionSpecs
            .Select(spec => new IntOption(spec.Value, Texts.GetOrDefault(spec.TextKey, spec.Fallback)))
            .ToArray();
        _incrementModeOptions = IncrementModeOptionSpecs
            .Select(spec => new IntOption(spec.Value, Texts.GetOrDefault(spec.TextKey, spec.Fallback)))
            .ToArray();

        OnPropertyChanged(nameof(ThemeOptions));
        OnPropertyChanged(nameof(ModeOptions));
        OnPropertyChanged(nameof(IncrementModeOptions));
        OnPropertyChanged(nameof(SelectedThemeOption));
        OnPropertyChanged(nameof(SelectedModeOption));
        OnPropertyChanged(nameof(SelectedIncrementModeOption));
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
}
