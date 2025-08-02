// <copyright file="BadModules.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Helper;
using Windows.Win32;

namespace MaaWpfGui.Utilities
{
    internal class BadModules
    {
        private static readonly string[] _names = ["NahimicOSD.dll", "AudioDevProps2.dll"];

        public static unsafe string[] GetBadInjectedModules()
        {
            var result = new List<string>();
            char[]? buffer = null;
            foreach (var name in _names)
            {
                var hmod = PInvoke.GetModuleHandle(name);
                if (!hmod.IsInvalid)
                {
                    buffer ??= new char[65536];
                    fixed (char* ptr = buffer)
                    {
                        if (PInvoke.GetModuleFileName(hmod, ptr, 65536) > 0)
                        {
                            result.Add(new string(ptr));
                        }
                    }
                }
            }

            return result.ToArray();
        }

        private class WpfWin32Window(System.Windows.Window w) : IWin32Window, System.Windows.Interop.IWin32Window
        {
            public IntPtr Handle => _helper.Handle;

            private readonly System.Windows.Interop.WindowInteropHelper _helper = new(w);
        }

        public static void CheckAndWarnBadInjectedModules()
        {
            if (System.Windows.Application.Current.MainWindow is null)
            {
                return;
            }

            var allBadModules = GetBadInjectedModules();
            var prevFound = ConfigFactory.Root.GUI.FoundBadModules.Split(";", StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
            var suppressed = ConfigFactory.Root.GUI.SuppressedBadModules.Split(";", StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
            var newFoundBadModules = allBadModules.Where(x => !prevFound.Contains(x, StringComparer.InvariantCultureIgnoreCase)).ToArray();
            var notSuppressedBadModules = allBadModules.Where(x => !suppressed.Contains(x, StringComparer.InvariantCultureIgnoreCase)).ToArray();
            ConfigFactory.Root.GUI.FoundBadModules = string.Join(";", [.. prevFound, .. newFoundBadModules]);

            if (notSuppressedBadModules.Length <= 0)
            {
                return;
            }

            var sb = new StringBuilder();
            sb.AppendLine(LocalizationHelper.GetString("BadModules.Warning.Prolog"));
            sb.AppendLine();
            foreach (var module in allBadModules)
            {
                sb.AppendLine(BreakLongPath(module, 70));
            }

            sb.AppendLine();
            sb.Append(LocalizationHelper.GetString("BadModules.Warning.Epilog"));

            var page = new TaskDialogPage
            {
                Caption = "MAA",
                Heading = LocalizationHelper.GetString("BadModules.Warning.Heading"),
                Text = sb.ToString(),
                Icon = TaskDialogIcon.Warning,
                Buttons = { TaskDialogButton.OK },
                SizeToContent = true,
            };

            if (newFoundBadModules.Length == 0)
            {
                // only show the "Do not show again" checkbox on the second time
                page.Verification = new()
                {
                    Text = LocalizationHelper.GetString("BadModules.Warning.DoNotShowAgain"),
                    Checked = false,
                };
            }

            TaskDialog.ShowDialog(new WpfWin32Window(System.Windows.Application.Current.MainWindow), page);

            if (page.Verification?.Checked ?? false)
            {
                ConfigFactory.Root.GUI.SuppressedBadModules = string.Join(";", allBadModules);
            }

            return;

            string BreakLongPath(string path, int maxLen)
            {
                if (path.Length <= maxLen)
                {
                    return path;
                }

                var sb = new StringBuilder();
                int start = 0;

                while (start < path.Length)
                {
                    if (start + maxLen >= path.Length)
                    {
                        sb.AppendLine(path[start..]);
                        break;
                    }

                    int breakPos = path.LastIndexOf('\\', start + maxLen);

                    if (breakPos <= start)
                    {
                        breakPos = Math.Min(start + maxLen, path.Length);
                        sb.AppendLine(path.Substring(start, breakPos - start));
                        start = breakPos;
                    }
                    else
                    {
                        int length = breakPos - start + 1;
                        sb.AppendLine(path.Substring(start, length));
                        start = breakPos + 1;
                    }
                }

                return sb.ToString();
            }
        }
    }
}
