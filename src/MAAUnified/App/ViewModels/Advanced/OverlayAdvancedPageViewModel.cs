using System.Collections.ObjectModel;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Services;
using MAAUnified.Platform;

namespace MAAUnified.App.ViewModels.Advanced;

public sealed class OverlayAdvancedPageViewModel : PageViewModelBase
{
    private OverlayTarget? _selectedTarget;
    private bool _visible;
    private string _capabilitySummary = string.Empty;

    public OverlayAdvancedPageViewModel(MAAUnifiedRuntime runtime)
        : base(runtime)
    {
        Targets = new ObservableCollection<OverlayTarget>();
    }

    public ObservableCollection<OverlayTarget> Targets { get; }

    public OverlayTarget? SelectedTarget
    {
        get => _selectedTarget;
        set => SetProperty(ref _selectedTarget, value);
    }

    public bool Visible
    {
        get => _visible;
        set => SetProperty(ref _visible, value);
    }

    public string CapabilitySummary
    {
        get => _capabilitySummary;
        private set => SetProperty(ref _capabilitySummary, value);
    }

    public async Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        await ReloadTargetsAsync(cancellationToken);
    }

    public async Task ReloadTargetsAsync(CancellationToken cancellationToken = default)
    {
        var targets = await ApplyResultAsync(
            await Runtime.OverlayFeatureService.GetOverlayTargetsAsync(cancellationToken),
            "Advanced.Overlay.QueryTargets",
            cancellationToken);
        if (targets is null)
        {
            return;
        }

        Targets.Clear();
        foreach (var target in targets)
        {
            Targets.Add(target);
        }

        SelectedTarget = Targets.FirstOrDefault(t => t.IsPrimary) ?? Targets.FirstOrDefault();

        var snapshot = await ApplyResultAsync(
            await Runtime.PlatformCapabilityService.GetSnapshotAsync(cancellationToken),
            "Advanced.Overlay.QueryCapability",
            cancellationToken);
        if (snapshot is not null)
        {
            var capability = snapshot.Overlay;
            CapabilitySummary = $"provider={capability.Provider}; supported={capability.Supported}; fallback={capability.FallbackMode ?? "none"}";
        }
    }

    public async Task ToggleOverlayAsync(CancellationToken cancellationToken = default)
    {
        var selectResult = await ApplyResultAsync(
            await Runtime.OverlayFeatureService.SelectOverlayTargetAsync(SelectedTarget?.Id ?? "preview", cancellationToken),
            "Advanced.Overlay.SelectTarget",
            cancellationToken);
        if (!selectResult)
        {
            return;
        }

        var requestedVisible = !Visible;
        var toggleResult = await Runtime.OverlayFeatureService.ToggleOverlayVisibilityAsync(requestedVisible, cancellationToken);
        if (!await ApplyResultAsync(
            toggleResult,
            "Advanced.Overlay.ToggleVisible",
            cancellationToken))
        {
            return;
        }

        Visible = requestedVisible;
    }
}
