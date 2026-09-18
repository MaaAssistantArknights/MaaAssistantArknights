// <copyright file="LogCardItemViewModel.cs" company="MaaAssistantArknights">
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
using System.Linq;
using System.Windows.Media;
using MaaWpfGui.Utilities;
using Stylet;

namespace MaaWpfGui.ViewModels.Items
{
    /// <summary>
    /// Represents a grouped log card that contains several <see cref="LogItemViewModel"/>.
    /// </summary>
    public class LogCardItemViewModel : PropertyChangedBase
    {
        public LogCardItemViewModel()
        {
            PropertyDependsOnUtility.InitializePropertyDependencies(this);
            Items.CollectionChanged += Items_CollectionChanged;
        }

        public ObservableCollection<LogItemViewModel> Items { get; } = new();

        /// <summary>
        /// Gets or sets a value indicating whether this card is a section divider
        /// (rendered as <c>hc:Divider</c> instead of a normal log card).
        /// </summary>
        public bool IsDivider { get; set => SetAndNotify(ref field, value); }

        /// <summary>
        /// Gets or sets the optional header text shown inside the divider.
        /// </summary>
        public string? Header { get; set => SetAndNotify(ref field, value); }

        /// <summary>
        /// Gets or sets a value indicating whether subsequent logs must start a new card.
        /// </summary>
        public bool Sealed { get; set; }

        private ImageSource? _thumbnail;

        public ImageSource? Thumbnail
        {
            get => _thumbnail;
            set => SetAndNotify(ref _thumbnail, value);
        }

        [PropertyDependsOn(nameof(Thumbnail))]
        public bool ShowThumbnail => Thumbnail is not null;

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
            NotifyOfPropertyChange(nameof(ShowTime));
            NotifyOfPropertyChange(nameof(ShowMetadata));
        }

        private void LogItem_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName == nameof(LogItemViewModel.Time))
            {
                NotifyOfPropertyChange(nameof(StartTime));
                NotifyOfPropertyChange(nameof(EndTime));
            }

            if (e.PropertyName == nameof(LogItemViewModel.ShowTime))
            {
                NotifyOfPropertyChange(nameof(ShowTime));
                NotifyOfPropertyChange(nameof(ShowMetadata));
                NotifyOfPropertyChange(nameof(StartTime));
                NotifyOfPropertyChange(nameof(EndTime));
            }
        }

        public bool ShowTime => Items.Any(item => item.ShowTime);

        [PropertyDependsOn(nameof(Thumbnail))]
        public bool ShowMetadata => ShowTime || ShowThumbnail;

        public string StartTime => Items.FirstOrDefault(item => item.ShowTime)?.Time ?? string.Empty;

        public string EndTime => Items.LastOrDefault(item => item.ShowTime)?.Time ?? string.Empty;
    }
}
