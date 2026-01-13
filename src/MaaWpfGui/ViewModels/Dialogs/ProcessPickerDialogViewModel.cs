// <copyright file="ProcessPickerDialogViewModel.cs" company="MaaAssistantArknights">
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

using System;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Stylet;
using Windows.Win32;

#nullable enable

namespace MaaWpfGui.ViewModels.Dialogs
{
    /// <summary>
    /// Represents a view model that provides a list of open windows and their associated processes for selection.
    /// </summary>
    /// <remarks>This view model is typically used to present a list of currently open windows, allowing the
    /// user to select a specific window and its process. The list is populated asynchronously by calling the
    /// LoadWindows method, which retrieves visible windows and their process information from the operating system. The
    /// Selected property reflects the currently chosen window entry. This class is intended for use in UI scenarios
    /// where process or window selection is required.</remarks>
    public partial class ProcessPickerDialogViewModel : Screen
    {
        public class WindowEntry
        {
            public IntPtr Hwnd { get; set; }

            public int ProcessId { get; set; }

            public required string ProcessName { get; set; }

            public required string Title { get; set; }

            public string Display => $"{Title} - {ProcessName} - {ProcessId}";
        }

        public ObservableCollection<WindowEntry> Items { get; } = [];

        private WindowEntry? _selected;

        public WindowEntry? Selected
        {
            get => _selected;
            set => SetAndNotify(ref _selected, value);
        }

        public ProcessPickerDialogViewModel()
        {
        }

        public Task LoadWindows()
        {
            return Task.Run(() =>
            {
                var list = new System.Collections.Generic.List<WindowEntry>();
                try
                {
                    PInvoke.EnumWindows((hWnd, lParam) =>
                    {
                        if (!PInvoke.IsWindowVisible(hWnd))
                        {
                            return true;
                        }

                        var len = PInvoke.GetWindowTextLength(hWnd);
                        if (len <= 0)
                        {
                            return true;
                        }

                        // Use Span<char> as CsWin32 exposes GetWindowText that accepts a span
                        Span<char> buffer = len <= 1024 ? stackalloc char[len + 1] : new char[len + 1];
                        int written = PInvoke.GetWindowText(hWnd, buffer);
                        var title = new string(buffer[..Math.Max(0, Math.Min(written, buffer.Length))]);
                        if (string.IsNullOrWhiteSpace(title))
                        {
                            return true;
                        }

                        _ = PInvoke.GetWindowThreadProcessId(hWnd, out var pid);
                        string name = "(unknown)";
                        try
                        {
                            var p = Process.GetProcessById((int)pid);
                            name = p.ProcessName;
                        }
                        catch
                        {
                        }

                        if (pid == Environment.ProcessId)
                        {
                           return true;
                        }

                        list.Add(new WindowEntry { Hwnd = (IntPtr)hWnd, ProcessId = (int)pid, ProcessName = name, Title = title });
                        return true;
                    }, IntPtr.Zero);
                }
                catch
                {
                }

                Execute.OnUIThread(() =>
                {
                    Items.Clear();
                    foreach (var it in list.OrderBy(x => x.ProcessName).ThenBy(x => x.Title))
                    {
                        Items.Add(it);
                    }

                    if (Items.Count > 0)
                    {
                        Selected = Items[0];
                    }
                });
            });
        }
    }
}
