// <copyright file="LogCardViewModel.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2025 MaaAssistantArknights Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

#nullable enable

using System.Collections.ObjectModel;
using System.Windows.Media;
using MaaWpfGui.Utilities;
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
            set => SetAndNotify(ref _thumbnail, value);
        }

        [PropertyDependsOn(nameof(Thumbnail))]
        public bool ShowThumbnail => Thumbnail is not null;

        public LogCardViewModel()
        {
            PropertyDependsOnUtility.InitializePropertyDependencies(this);

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
