using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.TaskQueue;

namespace MAAUnified.App.Features.Root;

public partial class TaskQueueView : UserControl
{
    private const string TaskRowDragFormat = "application/x-maa-task-row-index";
    private const double TaskRowDragStartThreshold = 4d;

    private Point? _taskRowDragStart;
    private TaskQueueItemViewModel? _taskRowDragSource;
    private bool _taskRowDragInProgress;

    public TaskQueueView()
    {
        InitializeComponent();
    }

    private TaskQueuePageViewModel? VM => DataContext as TaskQueuePageViewModel;

    private void OnOpenButtonContextMenuClick(object? sender, RoutedEventArgs e)
    {
        if (sender is not Control control || control.ContextMenu is null)
        {
            return;
        }

        if (VM is not null)
        {
            control.ContextMenu.DataContext = VM;
        }

        control.ContextMenu.Open(control);
    }

    private async void OnAddTaskModuleClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null || sender is not MenuItem { Tag: string taskType })
        {
            return;
        }

        await VM.AddTaskAsync(taskType);
    }

    private async void OnTaskMenuMoveUpClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null || !TryGetTaskParameter(sender, out var task))
        {
            return;
        }

        VM.SelectedTask = task;
        await VM.MoveSelectedTaskAsync(-1);
    }

    private async void OnTaskMenuMoveDownClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null || !TryGetTaskParameter(sender, out var task))
        {
            return;
        }

        VM.SelectedTask = task;
        await VM.MoveSelectedTaskAsync(1);
    }

    private async void OnTaskMenuRenameClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null || !TryGetTaskParameter(sender, out var task))
        {
            return;
        }

        VM.SelectedTask = task;
        await VM.RenameSelectedTaskWithDialogAsync();
    }

    private async void OnTaskMenuRemoveClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null || !TryGetTaskParameter(sender, out var task))
        {
            return;
        }

        VM.SelectedTask = task;
        await VM.RemoveSelectedTaskAsync();
    }

    private void OnTaskGearClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null || sender is not Control { DataContext: TaskQueueItemViewModel task })
        {
            return;
        }

        VM.SelectedTask = task;
    }

    private async void OnSelectAllClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SelectAllAsync(true);
        }
    }

    private async void OnBatchActionClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ExecuteBatchActionAsync();
        }
    }

    private async void OnToggleBatchModeClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ToggleSelectionBatchModeAsync();
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

    private void OnSelectGeneralSettingsClick(object? sender, RoutedEventArgs e)
    {
        VM?.SelectGeneralSettingsMode();
    }

    private void OnSelectAdvancedSettingsClick(object? sender, RoutedEventArgs e)
    {
        VM?.SelectAdvancedSettingsMode();
    }

    private void OnOpenPostActionClick(object? sender, RoutedEventArgs e)
    {
        VM?.OpenPostActionPanel();
    }

    private async void OnToggleOverlayClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ToggleOverlayAsync();
        }
    }

    private async void OnOverlayButtonPointerPressed(object? sender, PointerPressedEventArgs e)
    {
        if (VM is null || sender is not Control control)
        {
            return;
        }

        var point = e.GetCurrentPoint(control);
        if (!point.Properties.IsRightButtonPressed)
        {
            return;
        }

        e.Handled = true;
        await VM.PickOverlayTargetWithDialogAsync();
    }

    private void OnTaskRowPointerPressed(object? sender, PointerPressedEventArgs e)
    {
        if (sender is not Control control || control.DataContext is not TaskQueueItemViewModel source)
        {
            ResetTaskRowDragState();
            return;
        }

        var point = e.GetCurrentPoint(control);
        if (point.Properties.IsRightButtonPressed)
        {
            ResetTaskRowDragState();
            if (VM is not null)
            {
                VM.SelectedTask = source;
                if (!VM.CanEdit)
                {
                    e.Handled = true;
                    return;
                }
            }

            OpenTaskRowContextMenu(control);
            e.Handled = true;
            return;
        }

        if (VM is not null && !VM.CanEdit)
        {
            ResetTaskRowDragState();
            return;
        }

        if (!point.Properties.IsLeftButtonPressed)
        {
            ResetTaskRowDragState();
            return;
        }

        if (e.Source is Button or CheckBox or MenuItem)
        {
            ResetTaskRowDragState();
            return;
        }

        _taskRowDragSource = source;
        _taskRowDragStart = e.GetPosition(control);
    }

    private async void OnTaskRowPointerMoved(object? sender, PointerEventArgs e)
    {
        if (VM is null
            || !VM.CanEdit
            || _taskRowDragInProgress
            || _taskRowDragSource is null
            || _taskRowDragStart is null
            || sender is not Control control)
        {
            return;
        }

        var point = e.GetCurrentPoint(control);
        if (!point.Properties.IsLeftButtonPressed)
        {
            ResetTaskRowDragState();
            return;
        }

        var offset = e.GetPosition(control) - _taskRowDragStart.Value;
        if (Math.Abs(offset.X) < TaskRowDragStartThreshold && Math.Abs(offset.Y) < TaskRowDragStartThreshold)
        {
            return;
        }

        var sourceIndex = VM.Tasks.IndexOf(_taskRowDragSource);
        if (sourceIndex < 0)
        {
            ResetTaskRowDragState();
            return;
        }

        _taskRowDragInProgress = true;
        try
        {
            var data = new DataObject();
            data.Set(TaskRowDragFormat, sourceIndex);
            await DragDrop.DoDragDrop(e, data, DragDropEffects.Move);
        }
        finally
        {
            _taskRowDragInProgress = false;
            ResetTaskRowDragState();
        }
    }

    private void OnTaskRowDragOver(object? sender, DragEventArgs e)
    {
        if (VM is null
            || !VM.CanEdit
            || sender is not Control control
            || control.DataContext is not TaskQueueItemViewModel target
            || !TryGetDragRowIndex(e.Data, out var sourceIndex))
        {
            e.DragEffects = DragDropEffects.None;
            return;
        }

        var targetIndex = VM.Tasks.IndexOf(target);
        if (sourceIndex < 0 || targetIndex < 0 || sourceIndex == targetIndex)
        {
            e.DragEffects = DragDropEffects.None;
            return;
        }

        e.DragEffects = DragDropEffects.Move;
        e.Handled = true;
    }

    private async void OnTaskRowDrop(object? sender, DragEventArgs e)
    {
        if (VM is null
            || !VM.CanEdit
            || sender is not Control control
            || control.DataContext is not TaskQueueItemViewModel target
            || !TryGetDragRowIndex(e.Data, out var sourceIndex)
            || sourceIndex < 0
            || sourceIndex >= VM.Tasks.Count)
        {
            return;
        }

        var targetIndex = VM.Tasks.IndexOf(target);
        if (targetIndex < 0 || sourceIndex == targetIndex)
        {
            return;
        }

        VM.SelectedTask = VM.Tasks[sourceIndex];
        await VM.MoveSelectedTaskToAsync(targetIndex);
        e.Handled = true;
    }

    private static bool TryGetTaskParameter(object? sender, out TaskQueueItemViewModel task)
    {
        if (sender is not MenuItem menuItem)
        {
            task = null!;
            return false;
        }

        if (menuItem.CommandParameter is TaskQueueItemViewModel parameter)
        {
            task = parameter;
            return true;
        }

        var contextMenu = FindContextMenu(menuItem);
        if (contextMenu?.PlacementTarget is Control { Tag: TaskQueueItemViewModel tagged })
        {
            task = tagged;
            return true;
        }

        if (contextMenu?.PlacementTarget is Control { DataContext: TaskQueueItemViewModel fromDataContext })
        {
            task = fromDataContext;
            return true;
        }

        task = null!;
        return false;
    }

    private static ContextMenu? FindContextMenu(StyledElement element)
    {
        StyledElement? current = element;
        while (current is not null)
        {
            if (current is ContextMenu contextMenu)
            {
                return contextMenu;
            }

            current = current.Parent as StyledElement;
        }

        return null;
    }

    private static bool TryGetDragRowIndex(IDataObject data, out int index)
    {
        index = -1;
        if (!data.Contains(TaskRowDragFormat))
        {
            return false;
        }

        var raw = data.Get(TaskRowDragFormat);
        if (raw is int value)
        {
            index = value;
            return true;
        }

        return raw is string text && int.TryParse(text, out index);
    }

    private void ResetTaskRowDragState()
    {
        _taskRowDragStart = null;
        _taskRowDragSource = null;
    }

    private void OpenTaskRowContextMenu(Control rowControl)
    {
        if (VM is null || !VM.CanEdit || rowControl.ContextMenu is null)
        {
            return;
        }

        if (rowControl.Tag is TaskQueueItemViewModel task)
        {
            foreach (var item in rowControl.ContextMenu.Items)
            {
                if (item is MenuItem menuItem)
                {
                    menuItem.CommandParameter = task;
                }
            }
        }

        rowControl.ContextMenu.DataContext = VM;
        rowControl.ContextMenu.Open(rowControl);
    }
}
