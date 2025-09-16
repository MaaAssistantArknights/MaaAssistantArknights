// <copyright file="NoAutomationDataGrid .cs" company="MaaAssistantArknights">
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

using System.Linq;
using System.Windows.Automation.Peers;
using System.Windows.Controls;
using System.Windows.Input;

namespace MaaWpfGui.Styles.Controls
{
    public class NoAutomationDataGrid : DataGrid
    {
        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return null;
        }

        protected override void OnPreviewKeyDown(KeyEventArgs e)
        {
            base.OnPreviewKeyDown(e);

            // 因为干掉了 AutomationPeer，Tab 键无法正常切换焦点，这里手动实现 Tab 键切换行
            if (e.Key == Key.Tab)
            {
                e.Handled = true;

                var items = ItemsSource?.Cast<object>().ToList();
                if (items == null || items.Count == 0 || SelectedItem == null)
                {
                    return;
                }

                int currentIndex = items.IndexOf(SelectedItem);
                int nextIndex;

                if (Keyboard.Modifiers.HasFlag(ModifierKeys.Shift))
                {
                    // Shift + Tab 往前
                    nextIndex = currentIndex - 1;
                    if (nextIndex < 0)
                    {
                        nextIndex = items.Count - 1;
                    }
                }
                else
                {
                    // 普通 Tab 往后
                    nextIndex = currentIndex + 1;
                    if (nextIndex >= items.Count)
                    {
                        nextIndex = 0;
                    }
                }

                var nextItem = items[nextIndex];

                SelectedItem = nextItem;
                ScrollIntoView(nextItem);
            }
        }
    }
}
