using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows;

namespace MaaWpfGui.Views.UI;

/// <summary>
/// Represents a window that allows the user to select a running process by choosing one of its top-level windows.
/// </summary>
/// <remarks>This window displays a list of currently open top-level windows, enabling users to pick a process
/// based on its window. The selection is available through the SelectedHwnd property after the dialog is closed with a
/// positive result. This class is typically used as a modal dialog in applications that require the user to select a
/// process window.</remarks>
public partial class ProcessPickerWindow
{
    private readonly ViewModels.UI.ProcessPickerViewModel _vm;

    public IntPtr SelectedHwnd { get; private set; } = IntPtr.Zero;

    public ProcessPickerWindow()
    {
        InitializeComponent();
        _vm = new ViewModels.UI.ProcessPickerViewModel();
        DataContext = _vm;
        _ = _vm.LoadWindows();
    }

    private void BtnRefresh_Click(object sender, RoutedEventArgs e)
    {
        _ = _vm.LoadWindows();
    }

    private void BtnOk_Click(object sender, RoutedEventArgs e)
    {
        if (_vm.Selected != null)
        {
            SelectedHwnd = _vm.Selected.Hwnd;
            DialogResult = true;
            Close();
            return;
        }

        DialogResult = false;
        Close();
    }
}
