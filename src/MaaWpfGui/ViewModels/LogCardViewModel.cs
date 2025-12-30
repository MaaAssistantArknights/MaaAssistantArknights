#nullable enable
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Media;
using Stylet;

namespace MaaWpfGui.ViewModels
{
    /// <summary>
    /// Represents a grouped log card that contains several <see cref="LogItemViewModel"/>.
    /// </summary>
    public class LogCardViewModel : PropertyChangedBase
    {
        public ObservableCollection<LogItemViewModel> Items { get; } = new();

        private ImageSource? _thumbnail;

        public ImageSource? Thumbnail
        {
            get => _thumbnail;
            set
            {
                if (SetAndNotify(ref _thumbnail, value))
                {
                    NotifyOfPropertyChange(nameof(ShowThumbnail));
                }
            }
        }

        public bool ShowThumbnail => Thumbnail is not null;

        public LogCardViewModel()
        {
            // Keep StartTime/EndTime in sync when Items changes or an item's Time updates.
            Items.CollectionChanged += Items_CollectionChanged;
            // Attach to existing items if any (defensive).
            for (int i = 0; i < Items.Count; i++)
            {
                Items[i].PropertyChanged += LogItem_PropertyChanged;
            }
        }

        private void Items_CollectionChanged(object? sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            if (e.OldItems != null)
            {
                foreach (var obj in e.OldItems)
                {
                    if (obj is LogItemViewModel oldItem)
                    {
                        oldItem.PropertyChanged -= LogItem_PropertyChanged;
                    }
                }
            }

            if (e.NewItems != null)
            {
                foreach (var obj in e.NewItems)
                {
                    if (obj is LogItemViewModel newItem)
                    {
                        newItem.PropertyChanged += LogItem_PropertyChanged;
                    }
                }
            }

            NotifyOfPropertyChange(nameof(StartTime));
            NotifyOfPropertyChange(nameof(EndTime));
        }

        private void LogItem_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName == nameof(LogItemViewModel.Time))
            {
                NotifyOfPropertyChange(nameof(StartTime));
                NotifyOfPropertyChange(nameof(EndTime));
            }
        }

        public string StartTime => Items.Count > 0 ? Items[0].Time : string.Empty;

        public string EndTime => Items.Count > 0 ? Items[^1].Time : string.Empty;
    }
}
