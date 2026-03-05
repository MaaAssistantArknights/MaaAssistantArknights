using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.TaskParams;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class ReclamationModuleViewModel : TypedTaskModuleViewModelBase<ReclamationTaskParamsDto>
{
    private string _theme = "Tales";
    private int _mode = 1;
    private int _incrementMode;
    private int _numCraftBatches = 1;
    private string _toolsToCraftText = string.Empty;
    private bool _clearStore = true;

    public ReclamationModuleViewModel(MAAUnifiedRuntime runtime, LocalizedTextMap texts)
        : base(runtime, texts, "TaskQueue.Reclamation")
    {
    }

    public IReadOnlyList<string> ThemeOptions { get; } = ["Tales", "Fire"];

    public string Theme
    {
        get => _theme;
        set => SetTrackedProperty(ref _theme, value);
    }

    public int Mode
    {
        get => _mode;
        set => SetTrackedProperty(ref _mode, value);
    }

    public int IncrementMode
    {
        get => _incrementMode;
        set => SetTrackedProperty(ref _incrementMode, value);
    }

    public int NumCraftBatches
    {
        get => _numCraftBatches;
        set => SetTrackedProperty(ref _numCraftBatches, Math.Max(1, value));
    }

    public string ToolsToCraftText
    {
        get => _toolsToCraftText;
        set => SetTrackedProperty(ref _toolsToCraftText, value);
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
        ToolsToCraftText = string.Join(Environment.NewLine, dto.ToolsToCraft);
        ClearStore = dto.ClearStore;
    }

    protected override ReclamationTaskParamsDto BuildDto()
    {
        return new ReclamationTaskParamsDto
        {
            Theme = string.IsNullOrWhiteSpace(Theme) ? "Tales" : Theme.Trim(),
            Mode = Mode,
            IncrementMode = IncrementMode,
            NumCraftBatches = Math.Max(1, NumCraftBatches),
            ToolsToCraft = ParseTextLines(ToolsToCraftText),
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
