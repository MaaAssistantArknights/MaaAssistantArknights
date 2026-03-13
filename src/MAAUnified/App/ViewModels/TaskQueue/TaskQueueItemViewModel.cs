using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;

namespace MAAUnified.App.ViewModels.TaskQueue;

public static class TaskQueueItemStatus
{
    public const string Idle = "Idle";
    public const string Running = "Running";
    public const string Success = "Success";
    public const string Error = "Error";
    public const string Skipped = "Skipped";
}

public sealed class TaskQueueItemViewModel : ObservableObject
{
    private bool _isEnabled;
    private string _name;
    private string _status = TaskQueueItemStatus.Idle;
    private string _moduleDisplayName;
    private string _statusDisplayName = TaskQueueItemStatus.Idle;
    private string _displayName;
    private string _toolTipText;

    public TaskQueueItemViewModel(string type, string name, bool isEnabled)
    {
        Type = TaskModuleTypes.Normalize(type);
        _name = name;
        _isEnabled = isEnabled;
        _moduleDisplayName = Type;
        _displayName = name;
        _toolTipText = name;
        RefreshToolTipText();
    }

    public string Type { get; }

    public string Name
    {
        get => _name;
        set => SetProperty(ref _name, value);
    }

    public bool IsEnabled
    {
        get => _isEnabled;
        set => SetProperty(ref _isEnabled, value);
    }

    public string Status
    {
        get => _status;
        set
        {
            if (SetProperty(ref _status, value))
            {
                StatusDisplayName = value;
                OnPropertyChanged(nameof(IsStatusRunning));
                OnPropertyChanged(nameof(IsStatusSuccess));
                OnPropertyChanged(nameof(IsStatusError));
                OnPropertyChanged(nameof(IsStatusSkipped));
                OnPropertyChanged(nameof(IsStatusIdle));
            }
        }
    }

    public string ModuleDisplayName
    {
        get => _moduleDisplayName;
        set => SetProperty(ref _moduleDisplayName, value);
    }

    public string StatusDisplayName
    {
        get => _statusDisplayName;
        set
        {
            if (SetProperty(ref _statusDisplayName, value))
            {
                RefreshToolTipText();
            }
        }
    }

    public string DisplayName
    {
        get => _displayName;
        set
        {
            if (SetProperty(ref _displayName, value))
            {
                RefreshToolTipText();
            }
        }
    }

    public string ToolTipText
    {
        get => _toolTipText;
        private set => SetProperty(ref _toolTipText, value);
    }

    public bool IsStatusRunning => IsStatus(TaskQueueItemStatus.Running);

    public bool IsStatusSuccess => IsStatus(TaskQueueItemStatus.Success);

    public bool IsStatusError => IsStatus(TaskQueueItemStatus.Error);

    public bool IsStatusSkipped => IsStatus(TaskQueueItemStatus.Skipped);

    public bool IsStatusIdle =>
        !IsStatusRunning
        && !IsStatusSuccess
        && !IsStatusError
        && !IsStatusSkipped;

    public static TaskQueueItemViewModel FromUnifiedTask(UnifiedTaskItem task)
    {
        return new TaskQueueItemViewModel(task.Type, task.Name, task.IsEnabled);
    }

    public void RefreshLocalizedText(Func<string, string> resolveModuleDisplayName, Func<string, string> resolveStatusDisplayName)
    {
        ModuleDisplayName = resolveModuleDisplayName(Type);
        StatusDisplayName = resolveStatusDisplayName(Status);
        RefreshToolTipText();
    }

    public void RefreshToolTipText()
    {
        var title = string.IsNullOrWhiteSpace(DisplayName) ? Name : DisplayName;
        if (string.IsNullOrWhiteSpace(title))
        {
            title = ModuleDisplayName;
        }

        ToolTipText = string.IsNullOrWhiteSpace(StatusDisplayName)
            ? title
            : $"{title} ({StatusDisplayName})";
    }

    private bool IsStatus(string expected)
    {
        return string.Equals(Status, expected, StringComparison.OrdinalIgnoreCase);
    }
}
