using System.Runtime.CompilerServices;
using MAAUnified.Application.Services;

namespace MAAUnified.App.ViewModels.Infrastructure;

public sealed class OverlaySharedState : ObservableObject
{
    private string _selectedTargetId = "preview";
    private bool _visible;

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
}

internal static class OverlaySharedStateRegistry
{
    private static readonly ConditionalWeakTable<MAAUnifiedRuntime, OverlaySharedState> States = new();

    public static OverlaySharedState Get(MAAUnifiedRuntime runtime)
    {
        return States.GetValue(runtime, static _ => new OverlaySharedState());
    }
}
