using Avalonia.Controls;
using MAAUnified.App.Infrastructure;

namespace MAAUnified.App.Views;

public partial class RuntimeLogWindow : Window
{
    public RuntimeLogWindow()
    {
        InitializeComponent();
        WindowVisuals.ApplyDefaultIcon(this);
    }
}
