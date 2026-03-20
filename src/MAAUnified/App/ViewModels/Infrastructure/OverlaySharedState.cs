using System.Runtime.CompilerServices;
using MAAUnified.Application.Services;
using MAAUnified.Platform;

namespace MAAUnified.App.ViewModels.Infrastructure;

public sealed class OverlaySharedState : ObservableObject
{
    private string _selectedTargetId = "preview";
    private bool _visible;
    private OverlayRuntimeMode _mode = OverlayRuntimeMode.Hidden;
    private string _statusMessage = string.Empty;

    public string SelectedTargetId
    {
        get => _selectedTargetId;
        set
        {
            var normalized = string.IsNullOrWhiteSpace(value)
                ? "preview"
                : value.Trim();
            SetProperty(ref _selectedTargetId, normalized);
        }
    }

    public bool Visible
    {
        get => _visible;
        set => SetProperty(ref _visible, value);
    }

    public OverlayRuntimeMode Mode
    {
        get => _mode;
        set => SetProperty(ref _mode, value);
    }

    public string StatusMessage
    {
        get => _statusMessage;
        set => SetProperty(ref _statusMessage, value ?? string.Empty);
    }

    public void ApplyRuntimeState(OverlayStateChangedEvent state)
    {
        SelectedTargetId = string.IsNullOrWhiteSpace(state.TargetId)
            ? "preview"
            : state.TargetId;
        Visible = state.Visible;
        Mode = state.Mode;
        StatusMessage = state.Message;
    }
}

internal static class OverlaySharedStateRegistry
{
    private static readonly ConditionalWeakTable<MAAUnifiedRuntime, OverlaySharedState> States = new();

    public static OverlaySharedState Get(MAAUnifiedRuntime runtime)
    {
        return States.GetValue(runtime, static _ => new OverlaySharedState());
    }
}
