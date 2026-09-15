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

#nullable enable

#pragma warning disable SA1402

using System;
using System.Globalization;
using System.Linq;
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
        AttachCoreCharRuleOwner(StartingCoreCharComboBox);
        AttachCoreCharRuleOwner(StartingCoreChar2ComboBox);
        AttachCoreCharRuleOwner(StartingCoreChar3ComboBox);
    }

    // 三个开局干员下拉框各自持有独立的 StartingCoreCharRule 实例（位于各自 Text 绑定的
    // ValidationRules 中），把规则与其所属下拉框关联，使输入无效时的全干员列表切换
    // 只作用于触发校验的那一个下拉框，互不干扰
    private static void AttachCoreCharRuleOwner(ComboBox comboBox)
    {
        if (comboBox.GetBindingExpression(ComboBox.TextProperty)?.ParentBinding?.ValidationRules
            .OfType<StartingCoreCharRule>().FirstOrDefault() is { } rule)
        {
            rule.Owner = comboBox;
        }
    }

    private void CoreCharComboBox_DropDownClosed(object sender, EventArgs e)
    {
        if (sender is not ComboBox comboBox)
        {
            return;
        }

        var text = comboBox.Text;
        if (!string.IsNullOrEmpty(text) && DataHelper.GetCharacterByNameOrAlias(text) is null)
        {
            // 输入的是无效干员名，保持全干员列表 override，便于继续从任意干员中选取
            return;
        }

        // 清除全干员列表 override，回落到绑定的开局干员列表。换源可能因选中项不在新列表
        // 而清空文本，暂停 Text 绑定避免中间空值写回源属性，挂回时从源属性恢复文本
        comboBox.WithTextBindingSuspended(comboBox.ClearSearchableItemsSourceOverride);
    }
}

public class StartingCoreCharRule : ValidationRule
{
    /// <summary>
    /// Gets or sets the combo box owning this rule; its candidate list is switched to the
    /// full operator list while the current input is invalid.
    /// </summary>
    internal ComboBox? Owner { get; set; }

    public override ValidationResult Validate(object value, CultureInfo cultureInfo)
    {
        if (value is not string stringValue)
        {
            return new ValidationResult(false, HandyControl.Properties.Langs.Lang.FormatError);
        }

        if (!string.IsNullOrEmpty(stringValue) && DataHelper.GetCharacterByNameOrAlias(stringValue) is null)
        {
            // 输入无效时把下拉列表临时扩展为全干员列表，便于从任意干员中选取；
            // 经由可搜索扩展的 override 切换，不直接写 ItemsSource，以保持其维护的独立视图与过滤状态
            Owner?.SetSearchableItemsSourceOverride(DataHelper.CharacterNames);
            return new ValidationResult(false, LocalizationHelper.GetString("RoguelikeStartingCoreCharNotFound"));
        }

        return ValidationResult.ValidResult;
    }
}
