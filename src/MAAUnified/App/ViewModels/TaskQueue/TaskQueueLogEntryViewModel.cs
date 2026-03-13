using MAAUnified.App.ViewModels.Infrastructure;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class TaskQueueLogEntryViewModel : ObservableObject
{
    private string _time;
    private string _content;
    private string _level;

    public TaskQueueLogEntryViewModel(string time, string content, string level)
    {
        _time = time;
        _content = content;
        _level = level;
    }

    public string Time
    {
        get => _time;
        set
        {
            if (!SetProperty(ref _time, value))
            {
                return;
            }

            OnPropertyChanged(nameof(HasTime));
        }
    }

    public string Content
    {
        get => _content;
        set => SetProperty(ref _content, value);
    }

    public string Level
    {
        get => _level;
        set
        {
            if (!SetProperty(ref _level, value))
            {
                return;
            }

            OnPropertyChanged(nameof(IsError));
            OnPropertyChanged(nameof(IsWarning));
            OnPropertyChanged(nameof(IsInfo));
            OnPropertyChanged(nameof(IsSuccess));
        }
    }

    public bool IsError => IsLevel("ERROR");

    public bool IsWarning => IsLevel("WARN") || IsLevel("WARNING");

    public bool IsInfo => IsLevel("INFO");

    public bool IsSuccess => IsLevel("SUCCESS");

    public bool HasTime => !string.IsNullOrWhiteSpace(Time);

    private bool IsLevel(string level)
    {
        return string.Equals(Level, level, StringComparison.OrdinalIgnoreCase);
    }
}
