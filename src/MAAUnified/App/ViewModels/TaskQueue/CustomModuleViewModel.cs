using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.TaskParams;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class CustomModuleViewModel : TypedTaskModuleViewModelBase<CustomTaskParamsDto>
{
    private string _taskNamesText = string.Empty;

    public CustomModuleViewModel(MAAUnifiedRuntime runtime, LocalizedTextMap texts)
        : base(runtime, texts, "TaskQueue.Custom")
    {
    }

    public string TaskNamesText
    {
        get => _taskNamesText;
        set
        {
            if (!SetTrackedProperty(ref _taskNamesText, value))
            {
                return;
            }

            OnPropertyChanged(nameof(TaskNamesPreview));
        }
    }

    public string TaskNamesPreview
    {
        get
        {
            var values = ParseTaskNames(TaskNamesText);
            return values.Count == 0 ? string.Empty : string.Join(", ", values);
        }
    }

    protected override Task<UiOperationResult<CustomTaskParamsDto>> LoadDtoAsync(int index, CancellationToken cancellationToken)
    {
        return Runtime.TaskQueueFeatureService.GetCustomParamsAsync(index, cancellationToken);
    }

    protected override Task<UiOperationResult> SaveDtoAsync(int index, CustomTaskParamsDto dto, CancellationToken cancellationToken)
    {
        return Runtime.TaskQueueFeatureService.SaveCustomParamsAsync(index, dto, cancellationToken);
    }

    protected override TaskCompileOutput CompileDto(CustomTaskParamsDto dto, UnifiedProfile profile, UnifiedConfig config)
    {
        return TaskParamCompiler.CompileCustom(dto, profile, config);
    }

    protected override void ApplyDto(CustomTaskParamsDto dto)
    {
        TaskNamesText = string.Join(Environment.NewLine, dto.TaskNames);
        OnPropertyChanged(nameof(TaskNamesPreview));
    }

    protected override CustomTaskParamsDto BuildDto()
    {
        return new CustomTaskParamsDto
        {
            TaskNames = ParseTaskNames(TaskNamesText),
        };
    }

    protected override IReadOnlyList<TaskValidationIssue> ValidateBeforeSave()
    {
        if (!ContainsStructuredMarkers(TaskNamesText))
        {
            return [];
        }

        return
        [
            new TaskValidationIssue(
                "DelimitedInputParseFailed",
                "custom.task_names",
                "Custom task names only support plain delimiter-separated text."),
        ];
    }

    private static List<string> ParseTaskNames(string value)
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
