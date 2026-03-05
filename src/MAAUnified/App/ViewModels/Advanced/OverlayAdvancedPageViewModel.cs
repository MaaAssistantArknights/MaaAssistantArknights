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

        var snapshot = await Runtime.PlatformCapabilityService.GetSnapshotAsync(cancellationToken);
        if (snapshot.Success && snapshot.Value is not null)
        {
            var capability = snapshot.Value.Overlay;
            CapabilitySummary = $"provider={capability.Provider}; supported={capability.Supported}; fallback={capability.FallbackMode ?? "none"}";
        }
    }

    public async Task ToggleOverlayAsync(CancellationToken cancellationToken = default)
    {
        await ApplyResultAsync(
            await Runtime.OverlayFeatureService.SelectOverlayTargetAsync(SelectedTarget?.Id ?? "preview", cancellationToken),
            "Advanced.Overlay.SelectTarget",
            cancellationToken);

        Visible = !Visible;
        await ApplyResultAsync(
            await Runtime.OverlayFeatureService.ToggleOverlayVisibilityAsync(Visible, cancellationToken),
            "Advanced.Overlay.ToggleVisible",
            cancellationToken);
    }
}
