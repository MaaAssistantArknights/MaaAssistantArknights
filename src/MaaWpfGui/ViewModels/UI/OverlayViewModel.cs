// <copyright file="OverlayViewModel.cs" company="MaaAssistantArknights">
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
using System.Diagnostics;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Views.UI;
using Newtonsoft.Json;
using Stylet;
using Windows.Win32;

namespace MaaWpfGui.ViewModels.UI
{
    // ViewModel wrapper for overlay control; exposes methods for ViewModels and binding-friendly properties
    public class OverlayViewModel : PropertyChangedBase
    {
        public OverlayViewModel()
        {
            _logItemsSource = Instances.TaskQueueViewModel.LogItemViewModels;
        }

        private OverlayWindow? _overlay;

        public bool IsCreated => _overlay != null;

        private ObservableCollection<LogItemViewModel> _logItemsSource;

        public ObservableCollection<LogItemViewModel> LogItemsSource
        {
            get => _logItemsSource;
            set => SetAndNotify(ref _logItemsSource, value);
        }

        // 仅允许在窗口未显示的时候设置目标窗口，否则需要先关闭再设置
        public void SetTargetHwnd(IntPtr hwnd, bool persist = true)
        {
            if (hwnd == IntPtr.Zero)
            {
                return;
            }

            Execute.OnUIThread(() => _overlay?.SetTargetHwnd(hwnd));
            if (persist)
            {
                // persist target info (process id, name, title) only if changed
                try
                {
                    var hwndVal = (Windows.Win32.Foundation.HWND)hwnd;
                    PInvoke.GetWindowThreadProcessId(hwndVal, out var pid);
                    var title = GetWindowTitle(hwnd);
                    string name = "(unknown)";
                    try
                    {
                        var p = Process.GetProcessById((int)pid);
                        name = p.ProcessName;
                    }
                    catch
                    {
                    }

                    var info = new OverlayTargetInfo { ProcessId = (int)pid, ProcessName = name, Title = title, Hwnd = hwnd.ToInt64() };
                    var json = JsonConvert.SerializeObject(info);
                    ConfigurationHelper.SetGlobalValue(ConfigurationKeys.OverlayTarget, json);
                }
                catch
                {
                }
            }
        }

        public void EnsureCreated()
        {
            // Ensure creation runs on UI thread; this avoids explicit locks and keeps the code simple
            Execute.OnUIThread(() => {
                if (_overlay != null)
                {
                    return;
                }

                _overlay = new OverlayWindow();
                LoadConfig();
                _ = _overlay.InitializeAndShowAsync();
                NotifyOfPropertyChange(() => IsCreated);
            });
        }

        private void LoadConfig()
        {
            var saved = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.OverlayTarget, string.Empty);
            if (!string.IsNullOrWhiteSpace(saved))
            {
                var info = JsonConvert.DeserializeObject<OverlayTargetInfo>(saved);
                if (info != null)
                {
                    var found = TryFindWindow(info);
                    if (found != IntPtr.Zero)
                    {
                        SetTargetHwnd(found, false);
                    }
                }
            }
        }

        public void Close()
        {
            Execute.OnUIThread(() => {
                try
                {
                    _overlay?.Close();
                }
                catch
                {
                }

                _overlay = null;
                NotifyOfPropertyChange(() => IsCreated);
            });
        }

        private static string GetWindowTitle(IntPtr hWnd)
        {
            try
            {
                var hwndVal = (Windows.Win32.Foundation.HWND)hWnd;
                var len = PInvoke.GetWindowTextLength(hwndVal);
                if (len <= 0)
                {
                    return string.Empty;
                }

                Span<char> buffer = len <= 1024 ? stackalloc char[len + 1] : new char[len + 1];
                int written = PInvoke.GetWindowText(hwndVal, buffer);
                return new string(buffer[..Math.Max(0, Math.Min(written, buffer.Length))]);
            }
            catch
            {
                return string.Empty;
            }
        }

        private static IntPtr TryFindWindow(OverlayTargetInfo info)
        {
            IntPtr found = IntPtr.Zero;
            try
            {
                // 优先尝试使用存储的 hwnd（如果有）并验证其有效性与进程匹配
                if (info.Hwnd != 0)
                {
                    try
                    {
                        var h = (Windows.Win32.Foundation.HWND)new IntPtr(info.Hwnd);
                        _ = PInvoke.GetWindowThreadProcessId(h, out var pidCheck);
                        if (pidCheck == info.ProcessId)
                        {
                            return (IntPtr)h;
                        }
                    }
                    catch
                    {
                    }
                }

                PInvoke.EnumWindows((hWnd, lParam) => {
                    if (!PInvoke.IsWindowVisible(hWnd))
                    {
                        return true;
                    }

                    var len = PInvoke.GetWindowTextLength(hWnd);
                    if (len <= 0)
                    {
                        return true;
                    }

                    Span<char> buffer = len <= 1024 ? stackalloc char[len + 1] : new char[len + 1];
                    int written = PInvoke.GetWindowText(hWnd, buffer);
                    var title = new string(buffer[..Math.Max(0, Math.Min(written, buffer.Length))]);
                    if (string.IsNullOrWhiteSpace(title))
                    {
                        return true;
                    }

                    _ = PInvoke.GetWindowThreadProcessId(hWnd, out var pid);
                    if (pid == info.ProcessId)
                    {
                        found = (IntPtr)hWnd;
                        return false; // stop enumeration
                    }

                    // fallback: match by process name + title contains
                    try
                    {
                        var p = Process.GetProcessById((int)pid);
                        if (!string.IsNullOrEmpty(info.ProcessName) &&
                            p.ProcessName.Equals(info.ProcessName, StringComparison.OrdinalIgnoreCase) &&
                            title.Contains(info.Title ?? string.Empty, StringComparison.OrdinalIgnoreCase))
                        {
                            found = hWnd;
                            return false;
                        }
                    }
                    catch
                    {
                    }

                    return true;
                }, IntPtr.Zero);
            }
            catch
            {
            }

            return found;
        }

        private class OverlayTargetInfo
        {
            public int ProcessId { get; set; }

            public string? ProcessName { get; set; }

            public string? Title { get; set; }

            public long Hwnd { get; set; }
        }
    }
}
