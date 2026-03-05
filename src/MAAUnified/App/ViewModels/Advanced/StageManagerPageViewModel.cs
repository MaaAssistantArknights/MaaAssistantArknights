using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;

namespace MAAUnified.App.ViewModels.Advanced;

public sealed class StageManagerPageViewModel : PageViewModelBase
{
    private string _stageCodesText = string.Empty;
    private bool _autoIterate;
    private string _lastSelectedStage = string.Empty;

    public StageManagerPageViewModel(MAAUnifiedRuntime runtime)
        : base(runtime)
    {
    }

    public string StageCodesText
    {
        get => _stageCodesText;
        set => SetProperty(ref _stageCodesText, value ?? string.Empty);
    }

    public bool AutoIterate
    {
        get => _autoIterate;
        set => SetProperty(ref _autoIterate, value);
    }

    public string LastSelectedStage
    {
        get => _lastSelectedStage;
        set => SetProperty(ref _lastSelectedStage, value?.Trim() ?? string.Empty);
    }

    public async Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        var config = await ApplyResultAsync(
            await Runtime.StageManagerFeatureService.LoadConfigAsync(cancellationToken),
            "Advanced.StageManager.Load",
            cancellationToken);
        if (config is null)
        {
            return;
        }

        StageCodesText = string.Join(Environment.NewLine, config.StageCodes);
        AutoIterate = config.AutoIterate;
        LastSelectedStage = config.LastSelectedStage;
    }

    public async Task ValidateAsync(CancellationToken cancellationToken = default)
    {
        var result = await Runtime.StageManagerFeatureService.ValidateStageCodesAsync(StageCodesText, cancellationToken);
        var values = await ApplyResultAsync(result, "Advanced.StageManager.Validate", cancellationToken);
        if (values is null)
        {
            return;
        }

        StatusMessage = $"{values.Count} stage code(s) valid.";
    }

    public async Task SaveAsync(CancellationToken cancellationToken = default)
    {
        var parse = await Runtime.StageManagerFeatureService.ValidateStageCodesAsync(StageCodesText, cancellationToken);
        if (!parse.Success || parse.Value is null)
        {
            await ApplyResultAsync(parse, "Advanced.StageManager.ValidateBeforeSave", cancellationToken);
            return;
        }

        var config = new StageManagerConfig(
            StageCodes: parse.Value,
            AutoIterate: AutoIterate,
            LastSelectedStage: LastSelectedStage);
        await ApplyResultAsync(
            await Runtime.StageManagerFeatureService.SaveConfigAsync(config, cancellationToken),
            "Advanced.StageManager.Save",
            cancellationToken);
    }
}
