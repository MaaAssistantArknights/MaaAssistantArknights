using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.TaskQueue;

namespace MAAUnified.App.Features.TaskQueue;

public partial class PostActionSettingsView : UserControl
{
    public PostActionSettingsView()
    {
        InitializeComponent();
    }

    private PostActionModuleViewModel? VM => DataContext as PostActionModuleViewModel;

    private void OnClearClick(object? sender, RoutedEventArgs e)
    {
        VM?.ClearActions();
    }
}
