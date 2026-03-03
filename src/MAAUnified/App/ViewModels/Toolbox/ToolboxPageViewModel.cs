using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Services;

namespace MAAUnified.App.ViewModels.Toolbox;

public sealed class ToolboxPageViewModel : PageViewModelBase
{
    private int _selectedTabIndex;
    private string _resultText = "等待执行工具。";
    private bool _disclaimerAccepted;

    public ToolboxPageViewModel(MAAUnifiedRuntime runtime)
        : base(runtime)
    {
        Tabs = new[]
        {
            "招募识别",
            "干员识别",
            "仓库识别",
            "抽卡",
            "窥屏",
            "小游戏",
        };
    }

    public IReadOnlyList<string> Tabs { get; }

    public int SelectedTabIndex
    {
        get => _selectedTabIndex;
        set => SetProperty(ref _selectedTabIndex, Math.Clamp(value, 0, Tabs.Count - 1));
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

    public Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return Runtime.DiagnosticsService.RecordEventAsync("Toolbox", "Toolbox page initialized.", cancellationToken);
    }

    public async Task ExecuteCurrentToolAsync(CancellationToken cancellationToken = default)
    {
        if (!DisclaimerAccepted)
        {
            LastErrorMessage = "请先确认免责声明。";
            return;
        }

        var toolName = SelectedTabIndex switch
        {
            0 => "Recruit",
            1 => "OperBox",
            2 => "Depot",
            3 => "Gacha",
            4 => "VideoRecognition",
            5 => "MiniGame",
            _ => "Unknown",
        };

        var result = await Runtime.ToolboxFeatureService.ExecuteToolAsync(toolName, cancellationToken);
        var payload = await ApplyResultAsync(result, $"Toolbox.{toolName}", cancellationToken);
        if (payload is null)
        {
            return;
        }

        ResultText = payload;
    }
}
