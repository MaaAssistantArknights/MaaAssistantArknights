using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Services;
using MAAUnified.Platform;

namespace MAAUnified.App.ViewModels.Advanced;

public sealed class TrayIntegrationPageViewModel : PageViewModelBase
{
    private bool _trayVisible = true;
    private string _trayMessageTitle = "MAAUnified";
    private string _trayMessageBody = "Tray integration test.";
    private string _capabilitySummary = string.Empty;

    public TrayIntegrationPageViewModel(MAAUnifiedRuntime runtime)
        : base(runtime)
    {
    }

    public bool TrayVisible
    {
        get => _trayVisible;
        set => SetProperty(ref _trayVisible, value);
    }

    public string TrayMessageTitle
    {
        get => _trayMessageTitle;
        set => SetProperty(ref _trayMessageTitle, value ?? string.Empty);
    }

    public string TrayMessageBody
    {
        get => _trayMessageBody;
        set => SetProperty(ref _trayMessageBody, value ?? string.Empty);
    }

    public string CapabilitySummary
    {
        get => _capabilitySummary;
        private set => SetProperty(ref _capabilitySummary, value);
    }

    public async Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        await RefreshCapabilitySummaryAsync(cancellationToken);
    }

    public async Task RefreshCapabilitySummaryAsync(CancellationToken cancellationToken = default)
    {
        var snapshot = await Runtime.PlatformCapabilityService.GetSnapshotAsync(cancellationToken);
        if (!snapshot.Success || snapshot.Value is null)
        {
            CapabilitySummary = snapshot.Message;
            return;
        }

        var tray = snapshot.Value.Tray;
        CapabilitySummary = $"provider={tray.Provider}; supported={tray.Supported}; fallback={tray.FallbackMode ?? "none"}";
    }

    public async Task ApplyTrayVisibilityAsync(CancellationToken cancellationToken = default)
    {
        await ApplyResultAsync(
            await Runtime.PlatformCapabilityService.SetTrayVisibleAsync(TrayVisible, cancellationToken),
            "Advanced.TrayIntegration.SetVisible",
            cancellationToken);
    }

    public async Task SendTrayMessageAsync(CancellationToken cancellationToken = default)
    {
        await ApplyResultAsync(
            await Runtime.PlatformCapabilityService.ShowTrayMessageAsync(TrayMessageTitle, TrayMessageBody, cancellationToken),
            "Advanced.TrayIntegration.Notify",
            cancellationToken);
    }

    public async Task SyncMenuStateAsync(CancellationToken cancellationToken = default)
    {
        var state = new TrayMenuState(
            StartEnabled: true,
            StopEnabled: true,
            OverlayEnabled: true,
            ForceShowEnabled: true,
            HideTrayEnabled: true);
        await ApplyResultAsync(
            await Runtime.PlatformCapabilityService.SetTrayMenuStateAsync(state, cancellationToken),
            "Advanced.TrayIntegration.SyncMenu",
            cancellationToken);
    }
}
