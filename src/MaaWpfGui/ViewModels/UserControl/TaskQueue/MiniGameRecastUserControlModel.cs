// <copyright file="MiniGameRecastUserControlModel.cs" company="MaaAssistantArknights">
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
using System;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Linq;
using System.Windows;
using System.Windows.Input;
using System.Windows.Threading;
using HandyControl.Tools.Command;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Utilities.ValueType;
using Newtonsoft.Json;
using Serilog;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

/// <summary>
/// View model for mini game recast settings.
/// 小游戏重新投钱设置model
/// </summary>
public class MiniGameRecastUserControlModel : PropertyChangedBase
{
    private static readonly ILogger _logger = Log.ForContext<MiniGameRecastUserControlModel>();

    static MiniGameRecastUserControlModel()
    {
        Instance = new();
    }

    /// <summary>
    /// Gets the singleton instance of MiniGameRecastUserControlModel.
    /// </summary>
    public static MiniGameRecastUserControlModel Instance { get; }

    /// <summary>
    /// Gets the command to add a new recast condition.
    /// This static command is used for bindings in DataGrid headers where normal DataContext is not available.
    /// </summary>
    public static ICommand AddRecastConditionCommand { get; } = new RelayCommand(_ => Instance.AddRecastCondition());

    /// <summary>
    /// Gets the command to remove a recast condition.
    /// </summary>
    public ICommand RemoveRecastConditionCommand { get; }

    /// <summary>
    /// Gets the static command to remove a recast condition.
    /// This static command is used for bindings in DataGrid cell templates.
    /// </summary>
    public static ICommand RemoveRecastConditionStaticCommand { get; } = new RelayCommand(param => Instance.RemoveRecastCondition(param as RecastCondition));

    /// <summary>
    /// Gets the list of recast metrics.
    /// </summary>
    public static ObservableCollection<CombinedData> RecastMetricList { get; } =
    [
        new() { Display = LocalizationHelper.GetString("MiniGame@CoppersRecastMetricCount"), Value = RecastConstants.MetricTypes.RecastCount },
        new() { Display = LocalizationHelper.GetString("MiniGame@CoppersRecastMetricHp"), Value = RecastConstants.MetricTypes.Hp },
        new() { Display = LocalizationHelper.GetString("MiniGame@CoppersRecastMetricHope"), Value = RecastConstants.MetricTypes.Hope },
        new() { Display = LocalizationHelper.GetString("MiniGame@CoppersRecastMetricIngot"), Value = RecastConstants.MetricTypes.Ingot },
        new() { Display = LocalizationHelper.GetString("MiniGame@CoppersRecastMetricTicket"), Value = RecastConstants.MetricTypes.Ticket },
    ];

    /// <summary>
    /// Gets the list of recast comparators.
    /// </summary>
    public static ObservableCollection<CombinedData> RecastComparatorList { get; } =
    [
        new() { Display = "<", Value = RecastConstants.ComparatorTypes.Less },
        new() { Display = "≤", Value = RecastConstants.ComparatorTypes.LessOrEqual },
        new() { Display = "=", Value = RecastConstants.ComparatorTypes.Equal },
        new() { Display = ">", Value = RecastConstants.ComparatorTypes.Greater },
        new() { Display = "≥", Value = RecastConstants.ComparatorTypes.GreaterOrEqual },
    ];

    private ObservableCollection<RecastCondition> _recastConditions = [];
    private DispatcherTimer? _saveDebounceTimer;

    /// <summary>
    /// Gets or sets the collection of recast conditions.
    /// </summary>
    public ObservableCollection<RecastCondition> RecastConditions
    {
        get => _recastConditions;
        set
        {
            // Unsubscribe from old collection
            UnsubscribeFromCollection(_recastConditions);

            SetAndNotify(ref _recastConditions, value);

            // Subscribe to new collection
            SubscribeToCollection(_recastConditions);
        }
    }

    private RecastCondition? _selectedRecastCondition;

    public RecastCondition? SelectedRecastCondition
    {
        get => _selectedRecastCondition;
        set => SetAndNotify(ref _selectedRecastCondition, value);
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="MiniGameRecastUserControlModel"/> class.
    /// </summary>
    public MiniGameRecastUserControlModel()
    {
        RemoveRecastConditionCommand = new RelayCommand(param => RemoveRecastCondition(param as RecastCondition));
        InitializeSaveDebounceTimer();
        InitializeRecastConditions();
    }

    /// <summary>
    /// Initializes the recast conditions collection and loads saved data.
    /// </summary>
    private void InitializeRecastConditions()
    {
        // Initialize empty collection first to set up event handlers
        _recastConditions = new ObservableCollection<RecastCondition>();
        SubscribeToCollection(_recastConditions);

        // Then load saved conditions
        LoadRecastConditions();
    }

    /// <summary>
    /// Initializes the save debounce timer to prevent frequent saves.
    /// </summary>
    private void InitializeSaveDebounceTimer()
    {
        _saveDebounceTimer = new DispatcherTimer
        {
            Interval = TimeSpan.FromMilliseconds(500), // 500ms debounce delay
        };
        _saveDebounceTimer.Tick += (sender, e) =>
        {
            _saveDebounceTimer?.Stop();
            SaveRecastConditions();
        };
    }

    /// <summary>
    /// Loads recast conditions from configuration.
    /// </summary>
    private void LoadRecastConditions()
    {
        var json = ConfigurationHelper.GetValue(ConfigurationKeys.RecastConditions, string.Empty);
        if (string.IsNullOrWhiteSpace(json))
        {
            return;
        }

        try
        {
            var conditions = JsonConvert.DeserializeObject<System.Collections.Generic.List<RecastCondition>>(json);
            if (conditions != null && conditions.Count > 0)
            {
                // Unsubscribe temporarily to avoid duplicate subscriptions during loading
                UnsubscribeFromCollection(_recastConditions);

                _recastConditions.Clear();
                foreach (var condition in conditions)
                {
                    if (condition != null)
                    {
                        _recastConditions.Add(condition);
                        condition.PropertyChanged += RecastCondition_PropertyChanged;
                    }
                }

                // Re-subscribe to CollectionChanged event after loading is complete
                _recastConditions.CollectionChanged += RecastConditions_CollectionChanged;
            }
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to load recast conditions");

            // Ensure CollectionChanged is subscribed even if loading fails
            if (_recastConditions != null)
            {
                _recastConditions.CollectionChanged += RecastConditions_CollectionChanged;
            }
        }
    }

    /// <summary>
    /// Saves the recast conditions to configuration.
    /// This method handles serialization and persistence of the conditions collection.
    /// </summary>
    private void SaveRecastConditions()
    {
        try
        {
            if (_recastConditions == null || _recastConditions.Count == 0)
            {
                // Save empty array if collection is empty
                ConfigurationHelper.SetValue(ConfigurationKeys.RecastConditions, "[]");
                return;
            }

            var json = JsonConvert.SerializeObject(_recastConditions, Formatting.None);
            ConfigurationHelper.SetValue(ConfigurationKeys.RecastConditions, json);
        }
        catch (JsonException ex)
        {
            _logger.Error(ex, "Failed to serialize recast conditions");
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to save recast conditions to configuration");
        }
    }

    /// <summary>
    /// Triggers a debounced save operation.
    /// </summary>
    private void TriggerDebouncedSave()
    {
        if (_saveDebounceTimer == null)
        {
            InitializeSaveDebounceTimer();
        }

        _saveDebounceTimer?.Stop();
        _saveDebounceTimer?.Start();
    }

    /// <summary>
    /// Adds a new recast condition.
    /// </summary>
    public void AddRecastCondition()
    {
        var newCondition = new RecastCondition();
        RecastConditions.Add(newCondition);

        // Save immediately when adding/removing items
        SaveRecastConditions();
    }

    /// <summary>
    /// Removes a recast condition.
    /// </summary>
    /// <param name="condition">The condition to remove. If null, removes the selected condition.</param>
    public void RemoveRecastCondition(RecastCondition? condition = null)
    {
        var toRemove = condition ?? SelectedRecastCondition;
        if (toRemove == null)
        {
            return;
        }

        RecastConditions.Remove(toRemove);

        // Save immediately when adding/removing items
        SaveRecastConditions();
    }

    /// <summary>
    /// Updates the recast conditions after editing.
    /// CellEditEnding can be fired before the edited value is committed to the binding source.
    /// Defer the save so the binding update can complete first.
    /// </summary>
    public void UpdateRecastCondition()
    {
        // Use debounced save for property changes to avoid frequent saves
        Application.Current.Dispatcher.BeginInvoke(
            new Action(TriggerDebouncedSave),
            DispatcherPriority.Background);
    }

    /// <summary>
    /// Handles the collection changed event for recast conditions.
    /// </summary>
    private void RecastConditions_CollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        if (e == null)
        {
            return;
        }

        // Subscribe to new items
        if (e.NewItems != null)
        {
            foreach (RecastCondition? item in e.NewItems)
            {
                if (item != null)
                {
                    item.PropertyChanged += RecastCondition_PropertyChanged;
                }
            }
        }

        // Unsubscribe from old items
        if (e.OldItems != null)
        {
            foreach (RecastCondition? item in e.OldItems)
            {
                if (item != null)
                {
                    item.PropertyChanged -= RecastCondition_PropertyChanged;
                }
            }
        }

        // Save immediately when collection structure changed (add/remove operations)
        SaveRecastConditions();
    }

    /// <summary>
    /// Handles property changed events for individual recast conditions.
    /// Uses debounced save to avoid frequent saves during editing.
    /// </summary>
    private void RecastCondition_PropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        // Use debounced save to avoid frequent saves during property changes
        if (Application.Current?.Dispatcher != null)
        {
            Application.Current.Dispatcher.BeginInvoke(
                new Action(TriggerDebouncedSave),
                DispatcherPriority.Background);
        }
    }

    /// <summary>
    /// Subscribes to events for a collection of recast conditions.
    /// </summary>
    /// <param name="collection">The collection to subscribe to.</param>
    private void SubscribeToCollection(ObservableCollection<RecastCondition>? collection)
    {
        if (collection == null)
        {
            return;
        }

        collection.CollectionChanged += RecastConditions_CollectionChanged;
        foreach (var item in collection)
        {
            if (item != null)
            {
                item.PropertyChanged += RecastCondition_PropertyChanged;
            }
        }
    }

    /// <summary>
    /// Unsubscribes from events for a collection of recast conditions.
    /// </summary>
    /// <param name="collection">The collection to unsubscribe from.</param>
    private void UnsubscribeFromCollection(ObservableCollection<RecastCondition>? collection)
    {
        if (collection == null)
        {
            return;
        }

        collection.CollectionChanged -= RecastConditions_CollectionChanged;
        foreach (var item in collection)
        {
            if (item != null)
            {
                item.PropertyChanged -= RecastCondition_PropertyChanged;
            }
        }
    }
}
