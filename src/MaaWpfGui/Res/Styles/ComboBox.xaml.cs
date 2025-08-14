// <copyright file="ComboBox.xaml.cs" company="MaaAssistantArknights">
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

using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using JetBrains.Annotations;

namespace MaaWpfGui.Res.Styles
{
    /// <summary>
    /// ComboBox 的光标颜色不会跟随主题变化
    /// </summary>
    [UsedImplicitly]
    public partial class ComboBox
    {
        private void ComboBox_Loaded_SetCaretBrush(object sender, RoutedEventArgs e)
        {
            if (sender is not System.Windows.Controls.ComboBox cb)
            {
                return;
            }

            SetCaret();

            // 订阅 IsEditable 变化
            var dpd = DependencyPropertyDescriptor.FromProperty(System.Windows.Controls.ComboBox.IsEditableProperty, typeof(System.Windows.Controls.ComboBox));
            dpd.AddValueChanged(cb, (_, _) => SetCaret());
            return;

            void SetCaret()
            {
                if (!cb.IsEditable)
                {
                    return;
                }

                cb.ApplyTemplate();
                if (cb.Template.FindName("PART_EditableTextBox", cb) is TextBox tb)
                {
                    tb.CaretBrush = (Brush)Application.Current.Resources["PrimaryTextBrush"];
                }
            }
        }
    }
}
