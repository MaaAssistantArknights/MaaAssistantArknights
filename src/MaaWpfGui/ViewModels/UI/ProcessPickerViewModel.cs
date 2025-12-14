using System;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Stylet;

#nullable enable

namespace MaaWpfGui.ViewModels.UI
{
    /// <summary>
    /// Represents a view model that provides a list of open windows and their associated processes for selection.
    /// </summary>
    /// <remarks>This view model is typically used to present a list of currently open windows, allowing the
    /// user to select a specific window and its process. The list is populated asynchronously by calling the
    /// LoadWindows method, which retrieves visible windows and their process information from the operating system. The
    /// Selected property reflects the currently chosen window entry. This class is intended for use in UI scenarios
    /// where process or window selection is required.</remarks>
    public partial class ProcessPickerViewModel : Screen
    {
        public class WindowEntry
        {
            public IntPtr Hwnd { get; set; }

            public int ProcessId { get; set; }

            public required string ProcessName { get; set; }

            public required string Title { get; set; }

            public string Display => $"{ProcessName} - {Title}";
        }

        public ObservableCollection<WindowEntry> Items { get; } = [];

        private WindowEntry? _selected;

        public WindowEntry? Selected
        {
            get => _selected;
            set => SetAndNotify(ref _selected, value);
        }

        public ProcessPickerViewModel()
        {
        }

        public Task LoadWindows()
        {
            return Task.Run(() =>
            {
                var list = new System.Collections.Generic.List<WindowEntry>();
                try
                {
                    EnumWindows((hWnd, lParam) =>
                    {
                        if (!IsWindowVisible(hWnd))
                        {
                            return true;
                        }

                        var len = GetWindowTextLength(hWnd);
                        var sb = new StringBuilder(len + 1);
                        _ = GetWindowText(hWnd, sb, sb.Capacity);
                        var title = sb.ToString();
                        if (string.IsNullOrWhiteSpace(title))
                        {
                            return true;
                        }

                        _ = GetWindowThreadProcessId(hWnd, out var pid);
                        string name = "(unknown)";
                        try
                        {
                            var p = Process.GetProcessById((int)pid);
                            name = p.ProcessName;
                        }
                        catch
                        {
                            // ignored
                        }

                        list.Add(new WindowEntry { Hwnd = hWnd, ProcessId = (int)pid, ProcessName = name, Title = title });
                        return true;
                    }, IntPtr.Zero);
                }
                catch
                {
                    // ignored
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

        #region Win32
        private delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

        [LibraryImport("user32.dll", EntryPoint = "GetWindowTextLengthW")]
        private static partial int GetWindowTextLength(IntPtr hWnd);

        [LibraryImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static partial bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);

        [LibraryImport("user32.dll")]
        private static partial uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

        [LibraryImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static partial bool IsWindowVisible(IntPtr hWnd);
        #endregion
    }
}
