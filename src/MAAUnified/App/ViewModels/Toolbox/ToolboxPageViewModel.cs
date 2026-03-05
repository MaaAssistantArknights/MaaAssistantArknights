using System.Collections.ObjectModel;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Services;

namespace MAAUnified.App.ViewModels.Toolbox;

public sealed class ToolboxPageViewModel : PageViewModelBase
{
    private readonly Dictionary<int, string> _toolParametersByTab = [];
    private int _selectedTabIndex;
    private string _resultText = "等待执行工具。";
    private bool _disclaimerAccepted;
    private bool _isExecuting;
    private string _currentToolParameters = string.Empty;

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
        ExecutionHistory = new ObservableCollection<ToolExecutionRecord>();
    }

    public IReadOnlyList<string> Tabs { get; }

    public ObservableCollection<ToolExecutionRecord> ExecutionHistory { get; }

    public int SelectedTabIndex
    {
        get => _selectedTabIndex;
        set
        {
            var normalized = Math.Clamp(value, 0, Tabs.Count - 1);
            if (_selectedTabIndex == normalized)
            {
                return;
            }

            _toolParametersByTab[_selectedTabIndex] = CurrentToolParameters;
            if (SetProperty(ref _selectedTabIndex, normalized))
            {
                CurrentToolParameters = _toolParametersByTab.TryGetValue(normalized, out var stored)
                    ? stored
                    : string.Empty;
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

    public bool IsExecuting
    {
        get => _isExecuting;
        private set => SetProperty(ref _isExecuting, value);
    }

    public string CurrentToolParameters
    {
        get => _currentToolParameters;
        set => SetProperty(ref _currentToolParameters, value ?? string.Empty);
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
            ExecutionHistory.Insert(0, ToolExecutionRecord.Failed(MapToolName(), CurrentToolParameters, LastErrorMessage));
            return;
        }

        if (IsExecuting)
        {
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

        IsExecuting = true;
        try
        {
            var result = await Runtime.ToolboxFeatureService.ExecuteToolAsync(
                toolName,
                CurrentToolParameters,
                cancellationToken);
            var payload = await ApplyResultAsync(result, $"Toolbox.{toolName}", cancellationToken);
            if (payload is null)
            {
                ExecutionHistory.Insert(0, ToolExecutionRecord.Failed(toolName, CurrentToolParameters, result.Message));
                return;
            }

            ResultText = payload;
            ExecutionHistory.Insert(0, ToolExecutionRecord.Succeeded(toolName, CurrentToolParameters, payload));
        }
        finally
        {
            IsExecuting = false;
            const int maxHistoryCount = 30;
            while (ExecutionHistory.Count > maxHistoryCount)
            {
                ExecutionHistory.RemoveAt(ExecutionHistory.Count - 1);
            }
        }
    }

    private string MapToolName()
    {
        return SelectedTabIndex switch
        {
            0 => "Recruit",
            1 => "OperBox",
            2 => "Depot",
            3 => "Gacha",
            4 => "VideoRecognition",
            5 => "MiniGame",
            _ => "Unknown",
        };
    }
}

public sealed record ToolExecutionRecord(
    DateTimeOffset ExecutedAt,
    string ToolName,
    string ParameterSummary,
    bool Success,
    string ResultSummary)
{
    public static ToolExecutionRecord Succeeded(string toolName, string parameterSummary, string resultSummary)
        => new(DateTimeOffset.Now, toolName, parameterSummary, true, resultSummary);

    public static ToolExecutionRecord Failed(string toolName, string parameterSummary, string resultSummary)
        => new(DateTimeOffset.Now, toolName, parameterSummary, false, resultSummary);
}
