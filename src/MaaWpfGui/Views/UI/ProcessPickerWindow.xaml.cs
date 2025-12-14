using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows;

namespace MaaWpfGui.Views.UI;

public partial class ProcessPickerWindow
{
    public class WindowEntry
    {
        public IntPtr Hwnd { get; set; }

        public int ProcessId { get; set; }

        public string ProcessName { get; set; }

        public string Title { get; set; }

        public string Display => $"{ProcessName} - {Title}";
    }

    public IntPtr SelectedHwnd { get; private set; } = IntPtr.Zero;

    public ProcessPickerWindow()
    {
        InitializeComponent();
        LoadWindows();
    }

    private void BtnRefresh_Click(object sender, RoutedEventArgs e)
    {
        LoadWindows();
    }

    private void BtnOk_Click(object sender, RoutedEventArgs e)
    {
        if (LbWindows.SelectedItem is WindowEntry entry)
        {
            SelectedHwnd = entry.Hwnd;
            DialogResult = true;
            Close();
            return;
        }

        DialogResult = false;
        Close();
    }

    private void LoadWindows()
    {
        var list = new List<WindowEntry>();

        EnumWindows((hWnd, lParam) =>
        {
            if (!IsWindowVisible(hWnd))
            {
                return true;
            }

            var len = GetWindowTextLength(hWnd);
            var sb = new StringBuilder(len + 1);
            GetWindowText(hWnd, sb, sb.Capacity);
            var title = sb.ToString();
            if (string.IsNullOrWhiteSpace(title))
            {
                return true;
            }

            GetWindowThreadProcessId(hWnd, out var pid);
            string pname = "(unknown)";
            try
            {
                var p = Process.GetProcessById((int)pid);
                pname = p.ProcessName;
            }
            catch
            {
                // ignored
            }

            list.Add(new WindowEntry { Hwnd = hWnd, ProcessId = (int)pid, ProcessName = pname, Title = title });
            return true;
        }, IntPtr.Zero);

        LbWindows.ItemsSource = list.OrderBy(x => x.ProcessName).ThenBy(x => x.Title).ToList();
        if (LbWindows.Items.Count > 0)
        {
            LbWindows.SelectedIndex = 0;
        }
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
