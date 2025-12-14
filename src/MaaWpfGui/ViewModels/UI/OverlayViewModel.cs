using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using MaaWpfGui.Helper;
using MaaWpfGui.Constants;
using MaaWpfGui.Views.UI;
using Newtonsoft.Json;
using Stylet;

namespace MaaWpfGui.ViewModels.UI
{
    // ViewModel wrapper for overlay control; exposes methods for ViewModels and binding-friendly properties
    public class OverlayViewModel : Screen
    {
        private OverlayWindow _overlay;
        private Task _initTask;
        private readonly object _lockObj = new();

        public bool IsCreated => _overlay != null;

        public void SetTargetHwnd(IntPtr hwnd, bool persist = true)
        {
            if (hwnd == IntPtr.Zero)
                return;

            Application.Current.Dispatcher.Invoke(() =>
            {
                EnsureCreated();
                _overlay.SetTargetHwnd(hwnd);

            });
            if (persist)
            {
                // persist target info (process id, name, title) only if changed
                try
                {
                    GetWindowThreadProcessId(hwnd, out var pid);
                    var title = GetWindowTitle(hwnd);
                    string pname = "(unknown)";
                    try
                    {
                        var p = Process.GetProcessById((int)pid);
                        pname = p.ProcessName;
                    }
                    catch { }

                    var info = new OverlayTargetInfo { ProcessId = (int)pid, ProcessName = pname, Title = title, Hwnd = hwnd.ToInt64() };
                    var json = JsonConvert.SerializeObject(info);
                    ConfigurationHelper.SetGlobalValue(ConfigurationKeys.OverlayTarget, json);
                }
                catch
                {
                    // ignore persistence errors
                }
            }
        }

        public void EnsureCreated()
        {
            if (_overlay != null)
                return;

            lock (_lockObj)
            {
                if (_overlay != null)
                    return;

                _overlay = new OverlayWindow();
                // 订阅加载完成事件，在窗口完全就绪后恢复配置
                _overlay.LoadedCompleted += OnOverlayLoadedCompleted;
                _initTask = _overlay.InitializeAndShowAsync();
                NotifyOfPropertyChange(() => IsCreated);
            }
        }

        private void OnOverlayLoadedCompleted(object sender, EventArgs e)
        {
            // 在后台线程恢复配置，避免阻塞 UI
            Task.Run(() =>
            {
                try
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
                                Application.Current.Dispatcher.Invoke(() => SetTargetHwnd(found, false));
                            }
                        }
                    }
                }
                catch
                {
                    // ignore restore errors
                }
            });
        }

        public void Close()
        {
            try
            {
                Application.Current.Dispatcher.Invoke(() =>
                {
                    try
                    {
                        if (_overlay != null)
                        {
                            _overlay.LoadedCompleted -= OnOverlayLoadedCompleted;
                            _overlay.Close();
                        }
                    }
                    catch
                    {
                    }

                    _overlay = null;
                    NotifyOfPropertyChange(() => IsCreated);
                });
            }
            catch
            {
            }
        }

        private static string GetWindowTitle(IntPtr hWnd)
        {
            try
            {
                var len = GetWindowTextLength(hWnd);
                var sb = new StringBuilder(len + 1);
                GetWindowText(hWnd, sb, sb.Capacity);
                return sb.ToString();
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
                        var h = new IntPtr(info.Hwnd);
                        GetWindowThreadProcessId(h, out var pidCheck);
                        if (pidCheck == info.ProcessId)
                        {
                            return h;
                        }
                    }
                    catch { }
                }

                EnumWindows((hWnd, lParam) =>
                {
                    if (!IsWindowVisible(hWnd))
                        return true;

                    var len = GetWindowTextLength(hWnd);
                    var sb = new StringBuilder(len + 1);
                    GetWindowText(hWnd, sb, sb.Capacity);
                    var title = sb.ToString();
                    if (string.IsNullOrWhiteSpace(title))
                        return true;

                    GetWindowThreadProcessId(hWnd, out var pid);
                    if (pid == info.ProcessId)
                    {
                        found = hWnd;
                        return false; // stop enumeration
                    }

                    // fallback: match by process name + title contains
                    try
                    {
                        var p = Process.GetProcessById((int)pid);
                        if (!string.IsNullOrEmpty(info.ProcessName) && p.ProcessName.Equals(info.ProcessName, StringComparison.OrdinalIgnoreCase) && title.IndexOf(info.Title ?? string.Empty, StringComparison.OrdinalIgnoreCase) >= 0)
                        {
                            found = hWnd;
                            return false;
                        }
                    }
                    catch { }

                    return true;
                }, IntPtr.Zero);
            }
            catch { }

            return found;
        }

        private class OverlayTargetInfo
        {
            public int ProcessId { get; set; }

            public string ProcessName { get; set; }

            public string Title { get; set; }

            public long Hwnd { get; set; }
        }

        #region Win32
        private delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

        [DllImport("user32.dll")]
        private static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetWindowText(IntPtr hWnd, System.Text.StringBuilder lpString, int nMaxCount);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetWindowTextLength(IntPtr hWnd);

        [DllImport("user32.dll")]
        private static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool IsWindowVisible(IntPtr hWnd);
        #endregion
    }
}
