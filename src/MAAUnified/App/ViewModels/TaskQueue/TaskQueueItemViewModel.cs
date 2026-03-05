using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class TaskQueueItemViewModel : ObservableObject
{
    private bool _isEnabled;
    private string _name;
    private string _status = "Idle";

    public TaskQueueItemViewModel(string type, string name, bool isEnabled)
    {
        Type = TaskModuleTypes.Normalize(type);
        _name = name;
        _isEnabled = isEnabled;
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
                OnPropertyChanged(nameof(IsStatusRunning));
                OnPropertyChanged(nameof(IsStatusSuccess));
                OnPropertyChanged(nameof(IsStatusError));
                OnPropertyChanged(nameof(IsStatusSkipped));
                OnPropertyChanged(nameof(IsStatusIdle));
            }
        }
    }

    public bool IsStatusRunning => IsStatus("Running");

    public bool IsStatusSuccess => IsStatus("Success");

    public bool IsStatusError => IsStatus("Error");

    public bool IsStatusSkipped => IsStatus("Skipped");

    public bool IsStatusIdle =>
        !IsStatusRunning
        && !IsStatusSuccess
        && !IsStatusError
        && !IsStatusSkipped;

    public static TaskQueueItemViewModel FromUnifiedTask(UnifiedTaskItem task)
    {
        return new TaskQueueItemViewModel(task.Type, task.Name, task.IsEnabled);
    }

    private bool IsStatus(string expected)
    {
        return string.Equals(Status, expected, StringComparison.OrdinalIgnoreCase);
    }
}
