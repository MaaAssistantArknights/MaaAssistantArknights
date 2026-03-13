using System.Collections.ObjectModel;
using Avalonia.Media.Imaging;
using MAAUnified.App.ViewModels.Infrastructure;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class TaskQueueLogCardViewModel : ObservableObject
{
    private Bitmap? _thumbnail;

    public ObservableCollection<TaskQueueLogEntryViewModel> Items { get; } = [];

    public string StartTime => Items.Count == 0 ? string.Empty : Items[0].Time;

    public string EndTime => Items.Count == 0 ? string.Empty : Items[^1].Time;

    public string PrimaryContent => Items.Count == 0 ? string.Empty : Items[^1].Content;

    public Bitmap? Thumbnail
    {
        get => _thumbnail;
        set
        {
            if (ReferenceEquals(_thumbnail, value))
            {
                return;
            }

            var previous = _thumbnail;
            if (!SetProperty(ref _thumbnail, value))
            {
                return;
            }

            previous?.Dispose();
            OnPropertyChanged(nameof(ShowThumbnail));
            OnPropertyChanged(nameof(ShowTimeOnlyLayout));
        }
    }

    public bool ShowThumbnail => Thumbnail is not null;

    public bool ShowTimeOnlyLayout => !ShowThumbnail;

    public bool ShowStartTime => !string.IsNullOrWhiteSpace(StartTime);

    public bool ShowEndTime =>
        Items.Count > 1
        || (!string.IsNullOrWhiteSpace(StartTime)
            && !string.Equals(StartTime, EndTime, StringComparison.Ordinal));

    public void Append(TaskQueueLogEntryViewModel item)
    {
        Items.Add(item);
        OnPropertyChanged(nameof(StartTime));
        OnPropertyChanged(nameof(EndTime));
        OnPropertyChanged(nameof(PrimaryContent));
        OnPropertyChanged(nameof(ShowStartTime));
        OnPropertyChanged(nameof(ShowEndTime));
    }
}
