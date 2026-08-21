// <copyright file="RoguelikeSettingsUserControl.xaml.cs" company="MaaAssistantArknights">
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

#pragma warning disable SA1402

using System;
using System.Globalization;
using System.Windows.Controls;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;

namespace MaaWpfGui.Views.UserControl.TaskQueue;

/// <summary>
/// RoguelikeSettingsUserControl.xaml 的交互逻辑
/// </summary>
public partial class RoguelikeSettingsUserControl : System.Windows.Controls.UserControl
{
    /// <summary>
    /// Initializes a new instance of the <see cref="RoguelikeSettingsUserControl"/> class.
    /// </summary>
    public RoguelikeSettingsUserControl()
    {
        InitializeComponent();
        _current = this;
    }

    private static RoguelikeSettingsUserControl _current;
    private static bool _isValidResult;

    internal static bool IsValidResult
    {
        get => _isValidResult;
        set
        {
            _isValidResult = value;
            if (!IsValidResult)
            {
                // 输入无效时把下拉列表临时扩展为全干员列表，便于从任意干员中选取；
                // 经由可搜索扩展的 override 切换，不直接写 ItemsSource，以保持其维护的独立视图与过滤状态
                _current.StartingCoreCharComboBox.SetSearchableItemsSourceOverride(DataHelper.CharacterNames);
            }
        }
    }

    private void StartingCoreCharComboBox_DropDownClosed(object sender, EventArgs e)
    {
        if (!IsValidResult)
        {
            return;
        }

        // 清除全干员列表 override，回落到绑定的开局干员列表。换源可能因选中项不在新列表
        // 而清空文本，暂停 Text 绑定避免中间空值写回源属性，挂回时从源属性恢复文本
        StartingCoreCharComboBox.WithTextBindingSuspended(StartingCoreCharComboBox.ClearSearchableItemsSourceOverride);
    }
}

public class StartingCoreCharRule : ValidationRule
{
    public override ValidationResult Validate(object value, CultureInfo cultureInfo)
    {
        if (value is not string stringValue)
        {
            return new ValidationResult(false, HandyControl.Properties.Langs.Lang.FormatError);
        }

        if (!string.IsNullOrEmpty(stringValue) && DataHelper.GetCharacterByNameOrAlias(stringValue) is null)
        {
            RoguelikeSettingsUserControl.IsValidResult = false;
            return new ValidationResult(false, LocalizationHelper.GetString("RoguelikeStartingCoreCharNotFound"));
        }

        RoguelikeSettingsUserControl.IsValidResult = true;
        return ValidationResult.ValidResult;
    }
}
