// <copyright file="ClipboardInterceptor.cs" company="MaaAssistantArknights">
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
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Input;

namespace MaaWpfGui.Extensions.UIBehaviors
{
    public static class ClipboardInterceptor
    {
        public static readonly DependencyProperty EnableSafeClipboardProperty =
            DependencyProperty.RegisterAttached(
                "EnableSafeClipboard",
                typeof(bool),
                typeof(ClipboardInterceptor),
                new(false, OnEnableSafeClipboardChanged));

        public static void SetEnableSafeClipboard(DependencyObject element, bool value)
            => element.SetValue(EnableSafeClipboardProperty, value);

        public static bool GetEnableSafeClipboard(DependencyObject element)
            => (bool)element.GetValue(EnableSafeClipboardProperty);

        private static void OnEnableSafeClipboardChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            switch (d)
            {
                case TextBox tb when (bool)e.NewValue:
                    AddCommandBindingsToTextBox(tb);
                    break;
                case RichTextBox rtb when (bool)e.NewValue:
                    AddCommandBindingsToRichTextBox(rtb);
                    break;
                case DataGrid dg when (bool)e.NewValue:
                    AddCommandBindingsToDataGrid(dg);
                    break;
                case ComboBox cb when (bool)e.NewValue && cb.IsEditable:
                    cb.Loaded += OnComboBoxLoaded;
                    break;
            }
        }

        private static void AddCommandBindingsToTextBox(TextBox tb)
        {
            tb.CommandBindings.Add(new(ApplicationCommands.Copy, OnCopyTextBox));
            tb.CommandBindings.Add(new(ApplicationCommands.Cut, OnCutTextBox));
            tb.CommandBindings.Add(new(ApplicationCommands.Paste, OnPasteTextBox));
        }

        private static void AddCommandBindingsToRichTextBox(RichTextBox rtb)
        {
            rtb.CommandBindings.Add(new(ApplicationCommands.Copy, OnCopyRichTextBox));
            rtb.CommandBindings.Add(new(ApplicationCommands.Cut, OnCutRichTextBox));
            rtb.CommandBindings.Add(new(ApplicationCommands.Paste, OnPasteRichTextBox));
        }

        private static void AddCommandBindingsToDataGrid(DataGrid dg)
        {
            dg.CommandBindings.Add(new(ApplicationCommands.Copy, OnCopyDataGrid));
        }

        private static void OnComboBoxLoaded(object sender, RoutedEventArgs e)
        {
            if (sender is not ComboBox cb)
            {
                return;
            }

            if (cb.Template.FindName("PART_EditableTextBox", cb) is TextBox textBox)
            {
                AddCommandBindingsToTextBox(textBox);
            }
        }

        private static void OnCopyTextBox(object sender, ExecutedRoutedEventArgs e)
        {
            if (sender is not TextBox { SelectionLength: > 0 } tb)
            {
                return;
            }

            try
            {
                System.Windows.Forms.Clipboard.Clear();
                System.Windows.Forms.Clipboard.SetDataObject(tb.SelectedText, true);
            }
            catch
            {
                // ignored
            }

            e.Handled = true;
        }

        private static void OnCutTextBox(object sender, ExecutedRoutedEventArgs e)
        {
            if (sender is not TextBox { SelectionLength: > 0 } tb)
            {
                return;
            }

            try
            {
                System.Windows.Forms.Clipboard.Clear();
                System.Windows.Forms.Clipboard.SetDataObject(tb.SelectedText, true);
            }
            catch
            {
                // ignored
            }

            tb.SelectedText = string.Empty;
            e.Handled = true;
        }

        private static void OnPasteTextBox(object sender, ExecutedRoutedEventArgs e)
        {
            if (sender is not TextBox tb)
            {
                return;
            }

            if (System.Windows.Forms.Clipboard.ContainsText())
            {
                var pasteText = System.Windows.Forms.Clipboard.GetText();

                var start = tb.SelectionStart;

                tb.SelectedText = pasteText;
                tb.CaretIndex = start + pasteText.Length;
                tb.SelectionLength = 0;
            }

            e.Handled = true;
        }

        private static void OnCopyRichTextBox(object sender, ExecutedRoutedEventArgs e)
        {
            if (sender is not RichTextBox rtb)
            {
                return;
            }

            var textRange = new TextRange(rtb.Selection.Start, rtb.Selection.End);
            if (string.IsNullOrEmpty(textRange.Text))
            {
                return;
            }

            try
            {
                System.Windows.Forms.Clipboard.Clear();
                System.Windows.Forms.Clipboard.SetDataObject(textRange.Text, true);
            }
            catch
            {
                // ignored
            }

            e.Handled = true;
        }

        private static void OnCutRichTextBox(object sender, ExecutedRoutedEventArgs e)
        {
            if (sender is not RichTextBox rtb)
            {
                return;
            }

            var selection = new TextRange(rtb.Selection.Start, rtb.Selection.End);
            if (string.IsNullOrEmpty(selection.Text))
            {
                return;
            }

            try
            {
                System.Windows.Forms.Clipboard.Clear();
                System.Windows.Forms.Clipboard.SetDataObject(selection.Text, true);
            }
            catch
            {
                // ignored
            }

            selection.Text = string.Empty;
            e.Handled = true;
        }

        private static void OnPasteRichTextBox(object sender, ExecutedRoutedEventArgs e)
        {
            if (sender is not RichTextBox rtb)
            {
                return;
            }

            if (!System.Windows.Forms.Clipboard.ContainsText())
            {
                return;
            }

            var pasteText = System.Windows.Forms.Clipboard.GetText();

            var selection = rtb.Selection;
            selection.Text = pasteText;

            var caretPos = selection.End;
            rtb.CaretPosition = caretPos;
            rtb.Selection.Select(caretPos, caretPos);

            e.Handled = true;
        }

        private static void OnCopyDataGrid(object sender, ExecutedRoutedEventArgs e)
        {
            if (sender is not DataGrid dg)
            {
                return;
            }

            // 获取选中单元格内容，拼成制表符分隔的文本
            var selectedCells = dg.SelectedCells;
            if (selectedCells == null || selectedCells.Count == 0)
            {
                return;
            }

            var sb = new System.Text.StringBuilder();
            var rowGroups = selectedCells.GroupBy(c => c.Item);

            foreach (var row in rowGroups)
            {
                var rowText = string.Join("\t", row.Select(cell =>
                {
                    if (cell.Column.GetCellContent(cell.Item) is TextBlock tb)
                    {
                        return tb.Text;
                    }

                    return string.Empty;
                }));
                sb.AppendLine(rowText);
            }

            var sbStr = sb.ToString().TrimEnd('\r', '\n');

            try
            {
                System.Windows.Forms.Clipboard.Clear();
                System.Windows.Forms.Clipboard.SetDataObject(sbStr, true);
            }
            catch
            {
                // ignored
            }

            e.Handled = true;
        }
    }
}
