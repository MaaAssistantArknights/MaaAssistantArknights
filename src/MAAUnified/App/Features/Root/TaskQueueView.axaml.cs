using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.TaskQueue;

namespace MAAUnified.App.Features.Root;

public partial class TaskQueueView : UserControl
{
    public TaskQueueView()
    {
        InitializeComponent();
    }

    private TaskQueuePageViewModel? VM => DataContext as TaskQueuePageViewModel;

    private async void OnAddTaskClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.AddTaskAsync();
        }
    }

    private async void OnRemoveClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.RemoveSelectedTaskAsync();
        }
    }

    private async void OnRenameClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.RenameSelectedTaskWithDialogAsync();
        }
    }

    private async void OnMoveUpClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.MoveSelectedTaskAsync(-1);
        }
    }

    private async void OnMoveDownClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.MoveSelectedTaskAsync(1);
        }
    }

    private async void OnSelectAllClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SelectAllAsync(true);
        }
    }

    private async void OnInverseClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.InverseSelectionAsync();
        }
    }

    private async void OnSaveClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SaveAsync();
        }
    }

    private async void OnToggleRunClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ToggleRunAsync();
        }
    }

    private async void OnWaitAndStopClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.WaitAndStopAsync();
        }
    }

    private async void OnToggleOverlayClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ToggleOverlayAsync();
        }
    }

    private async void OnPickOverlayTargetClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.PickOverlayTargetWithDialogAsync();
        }
    }

    private async void OnReloadOverlayTargetsClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ReloadOverlayTargetsAsync();
        }
    }
}
