// <copyright file="FightSettingsUserControl.xaml.cs" company="MaaAssistantArknights">
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

using MaaWpfGui.ViewModels.UserControl.TaskQueue;

namespace MaaWpfGui.Views.UserControl.TaskQueue;

/// <summary>
/// ParamSettingUserControl.xaml 的交互逻辑
/// </summary>
public partial class FightSettingsUserControl : System.Windows.Controls.UserControl
{
    /// <summary>
    /// Initializes a new instance of the <see cref="FightSettingsUserControl"/> class.
    /// </summary>
    public FightSettingsUserControl()
    {
        InitializeComponent();
    }

    private FightSettingsUserControlModel? ViewModel => DataContext as FightSettingsUserControlModel;

    private bool _isUpdating = false;

    private void SingleStageComboBox_Loaded(object sender, System.Windows.RoutedEventArgs e)
    {
        if (_isUpdating || ViewModel == null)
        {
            return;
        }

        _isUpdating = true;
        try
        {
            if (ViewModel.StagePlan.Count <= 0)
            {
                var item = new FightSettingsUserControlModel.StagePlanItem();
                ViewModel.StagePlan.Add(item);
            }

            SingleStageComboBox.SelectedValue = ViewModel.StagePlan[0].Value;
        }
        finally
        {
            _isUpdating = false;
        }
    }

    private void SingleStageComboBox_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
    {
        if (_isUpdating || ViewModel == null)
        {
            return;
        }

        _isUpdating = true;
        try
        {
            var selectedValue = SingleStageComboBox.SelectedValue as string ?? string.Empty;

            // 确保 StagePlan 至少有一个元素
            if (ViewModel.StagePlan.Count <= 0)
            {
                var item = new FightSettingsUserControlModel.StagePlanItem();
                ViewModel.StagePlan.Add(item);
            }

            ViewModel.StagePlan[0].Value = selectedValue;
        }
        finally
        {
            _isUpdating = false;
        }
    }

    private void SingleStageTextBox_Loaded(object sender, System.Windows.RoutedEventArgs e)
    {
        if (_isUpdating || ViewModel == null)
        {
            return;
        }

        _isUpdating = true;
        try
        {
            if (ViewModel.StagePlan.Count > 0)
            {
                SingleStageTextBox.Text = ViewModel.StagePlan[0].Value;
            }
        }
        finally
        {
            _isUpdating = false;
        }
    }

    private void SingleStageTextBox_TextChanged(object sender, System.Windows.Controls.TextChangedEventArgs e)
    {
        if (_isUpdating || ViewModel == null)
        {
            return;
        }

        _isUpdating = true;
        try
        {
            var text = SingleStageTextBox.Text ?? string.Empty;

            // 确保 StagePlan 至少有一个元素
            if (ViewModel.StagePlan.Count == 0)
            {
                var item = new FightSettingsUserControlModel.StagePlanItem();
                ViewModel.StagePlan.Add(item);
            }

            ViewModel.StagePlan[0].Value = text;
        }
        finally
        {
            _isUpdating = false;
        }
    }
}
