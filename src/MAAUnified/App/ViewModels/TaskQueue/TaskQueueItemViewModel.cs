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
                OnPropertyChanged(nameof(StatusBrush));
            }
        }
    }

    public string StatusBrush => Status switch
    {
        "Running" => "#4A9EFF",
        "Success" => "#2FA66A",
        "Error" => "#E45757",
        "Skipped" => "#8F9099",
        _ => "#C8CCD3",
    };

    public static TaskQueueItemViewModel FromUnifiedTask(UnifiedTaskItem task)
    {
        return new TaskQueueItemViewModel(task.Type, task.Name, task.IsEnabled);
    }
}
